"""Bounded installed-image demo capture; consumes the shared demo inventory.

Run under a dedicated Xvfb. Screenshots and logs require review; launching a
window alone is not an assertion that its interaction/rendering is correct.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import time


def command(*args):
    result = subprocess.run(args, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, timeout=10)
    if args[0] == "xdotool" and result.returncode:
        raise RuntimeError("input command failed: " + result.stderr)
    return result


def animation_sample(client, path):
    capture = command("import", "-window", client, str(path))
    if capture.returncode:
        raise RuntimeError("animation capture failed: " + capture.stderr)
    digest = command("convert", str(path), "-format", "%#", "info:")
    if digest.returncode or not digest.stdout:
        raise RuntimeError("animation pixel digest failed")
    return digest.stdout


def windows():
    found = command("xprop", "-root", "_NET_CLIENT_LIST")
    return re.findall(r"0x[0-9a-f]+", found.stdout)


def controls(path):
    result = []
    if path.exists():
        for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
            parts = line.split("\t")
            if len(parts) == 7:
                entry = dict(type=parts[0], x=int(parts[1]), y=int(parts[2]),
                             width=int(parts[3]), height=int(parts[4]), label=parts[5])
                entry["owner"] = parts[6]
                result.append(entry)
    return result


def interact(case, entries, clients):
    profile = case["input"]["profile"]
    def find(kind, label=None):
        return next((c for c in entries if c["type"].endswith(kind) and
                     (label is None or c["label"].lower() == label.lower())), None)
    def click(control):
        if control is None:
            raise RuntimeError("interaction target missing: " + profile)
        command("xdotool", "mousemove", str(control["x"]+control["width"]//2),
                str(control["y"]+control["height"]//2), "mousedown", "1", "sleep", "0.1", "mouseup", "1")
        time.sleep(.4)
    if profile.startswith("button:"):
        click(find("Button", profile.split(":", 1)[1]))
    elif profile in ("choice", "dither"):
        click(find("Choice"))
        command("xdotool", "key", "--delay", "350", "Down", "Return")
        time.sleep(.4)
        if profile == "dither":
            click(find("Button", "New Image"))
    elif profile == "i18n-applet":
        click(find("Button", "Demo"))
    elif profile == "symbol":
        click(find("TextField"))
        command("xdotool", "key", "Home", "shift+End")
        command("xdotool", "type", "2200")
        command("xdotool", "key", "Return")
    elif profile in ("round", "openlook"):
        click(find("RoundButton") or find("OpenlookButton"))
    elif profile == "resize":
        for window in clients:
            geometry = command("xdotool", "getwindowgeometry", "--shell", window).stdout
            values = dict(re.findall(r"(WIDTH|HEIGHT)=(\d+)", geometry))
            command("xdotool", "windowsize", window, str(int(values["WIDTH"])+50),
                    str(int(values["HEIGHT"])+30))
    else:
        targets = [c for c in entries if c["type"].split(".")[-1] in
                       ("Animator", "DrawPanel", "GraphPanel", "XYZApp", "ThreeD", "NervousText",
                        "SortItem", "SpreadSheet", "TicTacToe", "JumpingBox", "ImageMap")]
        if not targets:
            targets = [c for c in entries if c["width"] > 80 and c["height"] > 80
                       and not c["type"].startswith(("java.", "sun."))]
        if not targets:
            raise RuntimeError("interaction target missing: " + profile)
        activated = set()
        for target in targets:
            client = target["owner"]
            if client not in clients:
                raise RuntimeError("observed window is absent from WM client list")
            command("xdotool", "windowactivate", "--sync", client)
            activated.add(client)
            x, y = target["x"]+target["width"]//2, target["y"]+target["height"]//2
            if profile == "imagemap":
                x, y = target["x"]+280, target["y"]+190
            elif profile == "move":
                x -= (x-target["x"]) % 3
                y -= (y-target["y"]) % 3
            command("xdotool", "mousemove", str(x), str(y))
            if profile == "drag":
                command("xdotool", "mousedown", "1", "mousemove", str(x+30), str(y+20), "mouseup", "1")
            elif profile not in ("move", "hover", "imagemap"):
                command("xdotool", "mousedown", "1", "sleep", "0.1", "mouseup", "1")
                if profile == "sheet":
                    command("xdotool", "type", "1")
        if len(activated) < case.get("expectedWindows", 1):
            raise RuntimeError("not every applet window received input")
        return dict(profile=profile, targets=len(targets), windows=len(activated))
    return profile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("demo", type=Path)
    parser.add_argument("image", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--probe", type=Path, required=True,
                        help="test-only libmotif118xprobe.so for exact window ownership")
    parser.add_argument("--select", default=".*")
    args = parser.parse_args()
    args.manifest = args.manifest.resolve()
    args.demo = args.demo.resolve()
    args.image = args.image.resolve()
    args.output = args.output.resolve()
    args.probe = args.probe.resolve()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8-sig"))
    args.output.mkdir(parents=True, exist_ok=True)
    helpers = args.image.parent / ("demo-helpers-" + args.output.name)
    helpers.mkdir(exist_ok=True)
    env = os.environ.copy()
    for name in ("CLASSPATH", "JAVA_HOME", "LD_LIBRARY_PATH"):
        env.pop(name, None)
    subprocess.run([str(args.image / "bin/javac"), "-d", str(helpers),
                    str(Path(__file__).resolve().parents[1] / "tools/WinAwtDemoApplet115.java"),
                    str(Path(__file__).with_name("DemoObserver115.java"))],
                   env=env, check=True, timeout=30)
    results = []
    for case in manifest["cases"]:
        if not re.search(args.select, case["id"]):
            continue
        output = args.output / case["id"]
        output.mkdir(exist_ok=True)
        if case["kind"] == "applet":
            # Same direct viewer entry used by the Windows demo gate. Do not
            # automate acceptance of the launcher's copyright notice.
            launch = ["WinAwtDemoApplet115",
                      (args.demo / case["path"]).as_uri()]
            if case["parserLimit"]:
                launch.append("observe-live-parser")
            cwd = helpers
        else:
            launch = [case["main"]]
            cwd = args.demo / case["classRoot"]
        snapshot = output / "controls.tsv"
        env["CLASSPATH"] = str(helpers) + ":" + str(cwd)
        launch = [str(args.image / "bin/java"), "-Ddemo.snapshot="+str(snapshot), "-Ddemo.probe="+str(args.probe),
                  "sun.awt.DemoObserver115"] + launch
        before = set(windows())
        with (output / "process.log").open("w") as log:
            process = subprocess.Popen(launch, cwd=cwd, env=env, stdout=log, stderr=log)
            try:
                time.sleep(4)
                clients = sorted(set(windows()) - before)
                deadline = time.monotonic()+20
                while len(clients) < case["expectedWindows"] and process.poll() is None and time.monotonic() < deadline:
                    time.sleep(.5)
                    clients = sorted(set(windows()) - before)
                time.sleep(1)
                entries = controls(snapshot)
                deadline = time.monotonic()+10
                while process.poll() is None and time.monotonic() < deadline:
                    needs_choice = case["input"]["profile"] in ("choice", "dither")
                    if entries and (not needs_choice or any(c["type"].endswith("Choice") for c in entries)):
                        break
                    time.sleep(.2)
                    entries = controls(snapshot)
                capture = command("import", "-window", "root", str(output / "initial.png"))
                record = dict(id=case["id"], windows=len(clients),
                              expectedWindows=case["expectedWindows"],
                              exit=process.poll(), capture=capture.returncode,
                              profile=case["input"]["profile"], review="pending")
                record["artifactDirectory"] = str(output)
                (output / "before-controls.json").write_text(json.dumps(entries, indent=2))
                try:
                    if record["profile"] == "pause":
                        first = animation_sample(clients[0], output / "motion-0.png")
                        record["movingBefore"] = False
                        for attempt in range(3):
                            time.sleep(.25)
                            sample = animation_sample(clients[0], output / ("motion-%d.png" % (attempt+1)))
                            if sample != first:
                                record["movingBefore"] = True
                                break
                    record["interaction"] = interact(case, entries, clients)
                except (RuntimeError, KeyError, IndexError, subprocess.TimeoutExpired) as failure:
                    record["interactionError"] = str(failure)
                time.sleep(10 if case["input"]["profile"] in ("dither", "i18n-applet") else 3)
                if record["profile"] == "i18n-applet":
                    nested = controls(snapshot)
                    before_choices = [c["label"] for c in nested if c["type"] == "java.awt.Choice"]
                    try:
                        interact(dict(input=dict(profile="choice")), nested, clients)
                    except (RuntimeError, KeyError, subprocess.TimeoutExpired) as failure:
                        record["interactionError"] = str(failure)
                    time.sleep(2)
                    after_choices = [c["label"] for c in controls(snapshot) if c["type"] == "java.awt.Choice"]
                    record["nestedDemoChanged"] = len(set(windows())-before)>len(clients) and before_choices != after_choices
                if record["profile"] == "pause" and clients:
                    first = animation_sample(clients[0], output / "paused.png")
                    time.sleep(.4)
                    record["pauseStable"] = first == animation_sample(clients[0], output / "paused-late.png")
                command("import", "-window", "root", str(output / "after.png"))
                after_entries = controls(snapshot)
                (output / "after-controls.json").write_text(json.dumps(after_entries, indent=2))
                record["clientCaptures"] = []
                for index, client in enumerate(sorted(set(windows()) - before)):
                    command("xdotool", "windowactivate", client)
                    time.sleep(.2)
                    name = "client-%d.png" % index
                    captured = command("import", "-window", client, str(output / name))
                    record["clientCaptures"].append(dict(file=name, result=captured.returncode))
                if case["input"]["profile"] in ("choice", "dither"):
                    before_choices = [c["label"] for c in entries if c["type"] == "java.awt.Choice"]
                    after_choices = [c["label"] for c in after_entries if c["type"] == "java.awt.Choice"]
                    record["choiceChanged"] = before_choices != after_choices
                if case["input"]["profile"] == "symbol":
                    record["textChanged"] = any(c["type"] == "java.awt.TextField" and
                                                c["label"] == "2200" for c in after_entries)
                record["exitAfterInput"] = process.poll()
                record["cleanupPolicy"] = case["cleanup"]
                for client in reversed(sorted(set(windows()) - before)):
                    if client not in windows() or process.poll() is not None:
                        continue
                    command("xdotool", "windowactivate", client)
                    command("xdotool", "key", "--delay", "100", "alt+F4")
                    time.sleep(.5)
                deadline = time.monotonic()+5
                while process.poll() is None and time.monotonic() < deadline:
                    time.sleep(.1)
                record["windowsAfterClose"] = len(set(windows()) - before)
                record["exitAfterClose"] = process.poll()
                record["executionPassed"] = (
                    record["windows"] >= record["expectedWindows"] and
                    record["exitAfterInput"] is None and not record.get("interactionError") and
                    record.get("choiceChanged", True) and record.get("textChanged", True) and record["capture"] == 0 and
                    record.get("movingBefore", True) and record.get("pauseStable", True) and record.get("nestedDemoChanged", True) and
                    all(c["result"] == 0 for c in record["clientCaptures"]) and
                    ((record["windowsAfterClose"] == 0 and record["exitAfterClose"] == 0) or
                     case["cleanup"] == "plain-frame-bounded-termination"))
                results.append(record)
                print(json.dumps(record), flush=True)
            finally:
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=3)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait()
        (args.output / "results.json").write_text(json.dumps(results, indent=2) + "\n")


if __name__ == "__main__":
    main()
