"""Exercise unchanged demo inventory through installed DR2 and AppKit events.

Requires the test observer/probe compiled in --helpers, normal SSH authentication,
and the private guest's QMP socket. Captures require separate visual review.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shlex
import socket
import subprocess
import time
from PIL import Image


class Qmp:
    def __init__(self, port, name):
        self.socket = socket.create_connection(("127.0.0.1", port), timeout=10)
        self.stream = self.socket.makefile("rwb")
        self.stream.readline()
        self.call("qmp_capabilities")
        if self.call("query-name")["name"] != name:
            raise RuntimeError("QMP guest identity mismatch")

    def call(self, name, **arguments):
        self.stream.write(json.dumps(dict(execute=name, arguments=arguments)).encode() + b"\n")
        self.stream.flush()
        while True:
            result = json.loads(self.stream.readline())
            if "error" in result:
                raise RuntimeError(str(result["error"]))
            if "return" in result:
                return result["return"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("host", "home", "helpers", "demo", "desktop-launch", "desktop-pid", "guest-output", "vm-name"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--port", default="22")
    parser.add_argument("--ssh-config")
    parser.add_argument("--qmp-port", type=int, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--select", default=".*")
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    args.guest_output = args.guest_output.rstrip("/") + "/run-" + str(time.time_ns())
    spec = importlib.util.spec_from_file_location("x11demos", Path(__file__).parents[1] / "awt118/demos.py")
    shared = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(shared)
    ssh = ["ssh", "-T", "-p", args.port]
    if args.ssh_config:
        ssh += ["-F", args.ssh_config]
    ssh.append(args.host)
    qmp = Qmp(args.qmp_port, args.vm_name)
    quote = shlex.quote

    def remote(script, check=True):
        result = subprocess.run(ssh + ["/bin/sh -c 'exec /bin/sh'"], input=(script + "\n").encode(),
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30)
        result.stdout = result.stdout.decode(errors="replace")
        if check and result.returncode:
            raise RuntimeError(result.stdout)
        return result

    def snapshot(path):
        remote_text = remote("cat " + quote(guest + "/controls.tsv"), False).stdout
        path.write_text(remote_text, encoding="utf-8")
        return shared.controls(path)

    def control(window, action, x=0, y=0):
        line = "%d %s %d %d\n" % (int(window, 16), action, x, y)
        target = guest + "/controls.tsv.command"
        remote("printf %s " + quote(line) + " > " + quote(target + ".tmp") +
               "\nmv " + quote(target + ".tmp") + " " + quote(target))
        deadline = time.monotonic() + 30
        while remote("test -f " + quote(target), False).returncode == 0:
            if action == "close" and remote("kill -0 " + str(pid), False).returncode:
                return
            if time.monotonic() > deadline:
                raise RuntimeError("AppKit command timed out: " + action)
            time.sleep(.1)

    def capture(path):
        # Native test events do not reset the guest's idle display dimmer.
        qmp.call("send-key", keys=[dict(type="qcode", data="shift")])
        time.sleep(.15)
        ppm = path.with_suffix(".ppm")
        qmp.call("screendump", filename=str(ppm).replace("\\", "/"))
        with Image.open(ppm) as image:
            image.save(path)
        ppm.unlink()

    def animation_sample(path, entries):
        capture(path)
        target = next((c for c in entries if c["type"] in ("Animator", "NervousText")), None)
        if target is None:
            raise RuntimeError("Animated applet is missing from the snapshot")
        # Java's DR2 screen origin excludes the 20-pixel menu bar.
        box = (target["x"], target["y"] + 20,
               target["x"] + target["width"], target["y"] + 20 + target["height"])
        with Image.open(path) as image:
            return hashlib.sha256(image.crop(box).tobytes()).hexdigest()

    def key(name):
        names = {"Down": "down", "Return": "ret", "Home": "home", "End": "end"}
        keys = [names.get(part, part) for part in name.split("+")]
        qmp.call("send-key", keys=[dict(type="qcode", data=part) for part in keys])
        time.sleep(.35)

    def command(*values):
        nonlocal active, mouse, pressed
        tokens = list(values)
        if tokens.pop(0) != "xdotool":
            raise RuntimeError("Unsupported demo input")
        while tokens:
            operation = tokens.pop(0)
            if operation == "windowactivate":
                if tokens[0] == "--sync":
                    tokens.pop(0)
                active = tokens.pop(0)
                control(active, "activate")
            elif operation == "mousemove":
                mouse = (int(tokens.pop(0)), int(tokens.pop(0)))
                if pressed is None:
                    control(active, "move", *mouse)
            elif operation in ("mousedown", "mouseup"):
                tokens.pop(0)
                if operation == "mousedown":
                    pressed = mouse
                else:
                    control(active, "click" if mouse == pressed else "draggesture", *mouse)
                    pressed = None
            elif operation == "sleep":
                time.sleep(float(tokens.pop(0)))
            elif operation == "key":
                if tokens and tokens[0] == "--delay":
                    del tokens[:2]
                for value in tokens:
                    key(value)
                tokens.clear()
            elif operation == "type":
                for value in tokens.pop(0):
                    key(value)
            elif operation == "getwindowgeometry":
                tokens.pop(0)
                window = tokens.pop(0)
                frame = next(c for c in entries if c["owner"] == window)
                return subprocess.CompletedProcess(values, 0, "WIDTH=%d\nHEIGHT=%d" % (frame["width"], frame["height"]), "")
            elif operation == "windowsize":
                window, width, height = tokens.pop(0), int(tokens.pop(0)), int(tokens.pop(0))
                frame = next(c for c in entries if c["owner"] == window)
                control(window, "resize", width-frame["width"], height-frame["height"])
            else:
                raise RuntimeError("Unsupported input operation: " + operation)
        return subprocess.CompletedProcess(values, 0, "", "")

    shared.command = command
    results = []
    for case in json.loads(args.manifest.read_text(encoding="utf-8-sig"))["cases"]:
        if not re.search(args.select, case["id"]):
            continue
        output = args.output / case["id"]
        output.mkdir(exist_ok=True)
        guest = args.guest_output.rstrip("/") + "/" + case["id"]
        launch = ["WinAwtDemoApplet115", "file:" + args.demo + "/" + case["path"]]
        cwd = args.helpers
        if case["kind"] != "applet":
            launch = [case["main"]]
            cwd = args.demo + "/" + case["classRoot"]
        elif case["parserLimit"]:
            launch.append("observe-live-parser")
        launch = [args.desktop_launch, args.desktop_pid, args.home + "/bin/java",
                  "-Ddemo.snapshot=" + guest + "/controls.tsv",
                  "-Ddemo.probe=" + args.helpers + "/libdemoprobe115.dylib",
                  "sun.awt.DR2DemoObserver115"] + launch
        script = "mkdir -p " + quote(guest) + "\ncd " + quote(cwd)
        script += "\nunset JAVA_HOME DYLD_LIBRARY_PATH\nCLASSPATH=" + quote(args.helpers + ":" + cwd)
        script += "\nexport CLASSPATH\n" + shlex.join(launch)
        script += " > " + quote(guest + "/process.log") + " 2>&1 < /dev/null &\necho $!"
        pid = int(remote(script).stdout.strip())
        record = dict(id=case["id"], expectedWindows=case["expectedWindows"], profile=case["input"]["profile"], review="pending")
        try:
            entries = []
            started_waiting = time.monotonic()
            deadline = started_waiting + 120
            while time.monotonic() < deadline:
                entries = snapshot(output / "controls.tsv")
                clients = sorted(set(c["owner"] for c in entries))
                if len(clients) >= case["expectedWindows"] and any(not c["type"].startswith(("java.", "sun.")) for c in entries):
                    break
                time.sleep(.5)
            record["windows"] = len(clients)
            record["startupSeconds"] = round(time.monotonic() - started_waiting, 2)
            if len(clients) < case["expectedWindows"]:
                raise RuntimeError("Missing demo windows")
            time.sleep(3)
            entries = snapshot(output / "before-controls.tsv")
            deadline = time.monotonic() + 30
            while any("loading" in c["label"].lower() for c in entries) and time.monotonic() < deadline:
                time.sleep(.5)
                entries = snapshot(output / "before-controls.tsv")
            active, mouse, pressed = clients[0], (0, 0), None
            control(active, "activate")
            capture(output / "initial.png")
            if record["profile"] == "pause":
                first = animation_sample(output / "motion-0.png", entries)
                record["movingBefore"] = False
                for attempt in range(6):
                    # Avoid sampling every 400 ms: the two-frame original demo
                    # also has a 400 ms cycle, which can hide real motion.
                    time.sleep(.23 + attempt * .07)
                    if animation_sample(output / ("motion-%d.png" % (attempt + 1)), entries) != first:
                        record["movingBefore"] = True
                        break
            record["interaction"] = shared.interact(case, entries, clients)
            time.sleep(3)
            after = snapshot(output / "after-controls.tsv")
            if record["profile"] == "resize":
                record["resizeAnchored"] = all(
                    next(c for c in entries if c["owner"] == window)["y"] ==
                    next(c for c in after if c["owner"] == window)["y"]
                    for window in clients)
            if record["profile"] == "pause":
                first = animation_sample(output / "paused.png", after)
                time.sleep(.7)
                record["pauseStable"] = first == animation_sample(output / "paused-late.png", after)
            if record["profile"] in ("choice", "dither"):
                record["choiceChanged"] = ([c["label"] for c in entries if c["type"] == "java.awt.Choice"] !=
                                           [c["label"] for c in after if c["type"] == "java.awt.Choice"])
            if record["profile"] == "symbol":
                record["textChanged"] = any(c["type"] == "java.awt.TextField" and c["label"] == "2200" for c in after)
            if record["profile"] == "i18n-applet":
                nested_clients = sorted(set(c["owner"] for c in after))
                started_waiting = time.monotonic()
                deadline = started_waiting + 120
                while not set(nested_clients) - set(clients) and time.monotonic() < deadline:
                    time.sleep(.5)
                    after = snapshot(output / "after-controls.tsv")
                    nested_clients = sorted(set(c["owner"] for c in after))
                if not set(nested_clients) - set(clients):
                    raise RuntimeError("Demo button did not open its application window")
                record["nestedWindowWaitSeconds"] = round(time.monotonic() - started_waiting, 2)
                active = next(window for window in nested_clients if window not in clients)
                control(active, "activate")
                before_choices = [c["label"] for c in after if c["type"] == "java.awt.Choice"]
                shared.interact(dict(input=dict(profile="choice")), after, nested_clients)
                time.sleep(2)
                after = snapshot(output / "after-controls.tsv")
                record["nestedDemoChanged"] = before_choices != [c["label"] for c in after if c["type"] == "java.awt.Choice"]
            capture(output / "after.png")
            record["clientCaptures"] = []
            for index, window in enumerate(sorted(set(c["owner"] for c in after))):
                control(window, "activate")
                time.sleep(.3)
                name = "client-%d.png" % index
                capture(output / name)
                record["clientCaptures"].append(dict(window=window, file=name))
            record["interactionCompleted"] = True
            for window in reversed(sorted(set(c["owner"] for c in after))):
                control(window, "close")
            time.sleep(1)
            record["aliveAfterClose"] = remote("kill -0 " + str(pid), False).returncode == 0
            deadline = time.monotonic() + 10
            while (record["aliveAfterClose"] and case["cleanup"] != "plain-frame-bounded-termination"
                   and time.monotonic() < deadline):
                time.sleep(.5)
                record["aliveAfterClose"] = remote("kill -0 " + str(pid), False).returncode == 0
            record["executionPassed"] = all(record.get(name, True) for name in
                ("movingBefore", "pauseStable", "choiceChanged", "textChanged", "nestedDemoChanged", "resizeAnchored")) and (
                not record["aliveAfterClose"] or case["cleanup"] == "plain-frame-bounded-termination")
            if not record["executionPassed"] and record["aliveAfterClose"]:
                remote("kill -3 " + str(pid) + " 2>/dev/null || :", False)
                time.sleep(.5)
        except (RuntimeError, subprocess.TimeoutExpired, ValueError, IndexError) as failure:
            record["error"] = str(failure)
            record["executionPassed"] = False
            remote("kill -3 " + str(pid) + " 2>/dev/null || :", False)
            time.sleep(.5)
        finally:
            record["processLog"] = remote("cat " + quote(guest + "/process.log"), False).stdout
            if "Exception occurred during event dispatching:" in record["processLog"]:
                record["executionPassed"] = False
                record["error"] = "Demo logged an event-dispatch exception"
            remote("kill " + str(pid) + " 2>/dev/null || :")
            results.append(record)
            (args.output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
            print(json.dumps({key: value for key, value in record.items() if key != "processLog"}), flush=True)

    if not all(record.get("executionPassed", False) for record in results):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
