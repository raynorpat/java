"""Bounded loopback jdb session; owns and reaps both Java processes."""
import argparse
import os
import re
import signal
import subprocess
import threading
import time
from pathlib import Path


class Process:
    def __init__(self, args, env, tree=False, wrapper=None):
        self.text = ""
        self.condition = threading.Condition()
        self.tree = tree and os.name != "nt"
        if tree and os.name == "nt":
            if not wrapper:
                raise ValueError("Windows autolaunch requires the owned-job wrapper")
            args = [wrapper] + args
        self.process = subprocess.Popen(args, stdin=subprocess.PIPE,
                                        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                        env=env, start_new_session=self.tree)
        self.reader = threading.Thread(target=self.read, daemon=True)
        self.reader.start()

    def read(self):
        while True:
            data = os.read(self.process.stdout.fileno(), 4096)
            if not data:
                break
            with self.condition:
                self.text = (self.text + data.decode("latin1")).replace("\r\n", "\n")
                self.condition.notify_all()

    def expect(self, pattern, start=0, timeout=20):
        deadline = time.monotonic() + timeout
        with self.condition:
            while True:
                match = re.search(pattern, self.text[start:])
                if match:
                    return match
                remaining = deadline - time.monotonic()
                if remaining <= 0 or (self.process.poll() is not None and not self.reader.is_alive()):
                    raise AssertionError("Expected %r; output:\n%s" % (pattern, self.text))
                self.condition.wait(min(remaining, 0.1))

    def command(self, command):
        start = len(self.text)
        self.process.stdin.write((command + "\n").encode("ascii"))
        self.process.stdin.flush()
        return start

    def close(self):
        if self.tree:
            try:
                os.killpg(self.process.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
        if self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=5)
        self.reader.join(timeout=5)
        if self.tree:
            try:
                os.killpg(self.process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--java", required=True)
    parser.add_argument("--jdb", required=True)
    parser.add_argument("--classpath", required=True)
    parser.add_argument("--libdir", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--stop-after-breakpoint", action="store_true")
    parser.add_argument("--expect-release-rejection", action="store_true")
    parser.add_argument("--launch", action="store_true")
    parser.add_argument("--process-wrapper")
    parser.add_argument("--exception-mode", choices=["caught", "uncaught"])
    args = parser.parse_args()
    env = os.environ.copy()
    variable = "PATH" if os.name == "nt" else "LD_LIBRARY_PATH"
    env[variable] = args.libdir + os.pathsep + env.get(variable, "")
    env["CLASSPATH"] = args.classpath
    # Isolate the legacy launcher from the user's modern JAVA_HOME.
    env["JAVA_HOME"] = str(Path(args.jdb).resolve().parent.parent)
    processes = []
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    try:
        if args.launch:
            client = Process([args.jdb], env, tree=True, wrapper=args.process_wrapper)
        else:
            server = Process([args.java, "-debug", "-debugport", "0", "-classpath",
                              args.classpath, "sun.tools.debug.EmptyApp"], env)
            processes.append(("server", server))
            password = server.expect(r"[Pp]assword=([^\r\n]+)").group(1).strip()
            client = Process([args.jdb, "-host", "127.0.0.1", "-password", password], env)
        processes.append(("client", client))
        if args.expect_release_rejection:
            server.expect(r"java.lang.IllegalAccessError: not a debugging VM")
            print("release agent rejects non-debugging VM as Sun does")
            return
        client.expect(r"> ")
        mark = client.command("help")
        client.expect(r"> ", mark)
        if args.exception_mode:
            mark = client.command("load Exception115")
            client.expect(r"> ", mark)
            mark = client.command("catch java.lang.IllegalArgumentException")
            client.expect(r"> ", mark)
            suffix = " caught" if args.exception_mode == "caught" else ""
            mark = client.command("run Exception115" + suffix)
            prefix = "Exception" if args.exception_mode == "caught" else "Uncaught exception"
            client.expect(prefix + r": java.lang.IllegalArgumentException debugger exception\n"
                          r"\tat Exception115.fail\(Exception115.java:3\)", mark)
            client.expect(r"\n\nmain\[1\] ", mark)
            mark = client.command("where")
            client.expect(r"Exception115.fail \(Exception115:3\)", mark)
            client.expect(r"main\[1\] ", mark)
            mark = client.command("cont")
            if args.exception_mode == "caught":
                client.expect(r"debugger exception caught", mark)
                client.expect(r"The application exited", mark)
            else:
                # Sun reports the uncaught exception again as it unwinds
                # through the native runMain boundary; keep every stop bounded.
                for continuation in range(4):
                    event = client.expect(r"The application exited|Uncaught exception:", mark)
                    if event.group(0) == "The application exited":
                        break
                    client.expect(r"\n\nmain\[1\] ", mark)
                    mark = client.command("cont")
                else:
                    raise AssertionError("Uncaught exception did not finish unwinding")
                if continuation != 1:
                    raise AssertionError("Uncaught exception stop count differs from Sun")
                print("uncaught continuations=%d" % (continuation + 1))
            client.command("quit")
            client.process.wait(timeout=20)
            if client.process.returncode != 0:
                raise AssertionError("jdb exception session did not exit cleanly")
            print("jdb %s exception session passed" % args.exception_mode)
            return
        mark = client.command("load Debuggee115")
        client.expect(r"> ", mark)
        mark = client.command("methods Debuggee115")
        client.expect(r"void target\(int\)\nvoid main\(String\[\]\)", mark)
        client.expect(r"> ", mark)
        mark = client.command("stop in Debuggee115.target")
        client.expect(r"> ", mark)
        mark = client.command("run Debuggee115")
        # TTY prints the event prefix, fetches the stack, then prints the frame.
        # Its command thread can print a prompt during that remote stack fetch.
        breakpoint_prefix = r"Breakpoint hit: (?:(?:main\[1\]|>) )?"
        client.expect(breakpoint_prefix + r"Debuggee115.target \(Debuggee115:4\)\nmain\[1\] ", mark)
        if args.stop_after_breakpoint:
            client.command("quit")
            client.process.wait(timeout=20)
            if client.process.returncode != 0:
                raise AssertionError("jdb failed to disconnect while stopped")
            print("jdb disconnect while stopped passed")
            return
        mark = client.command("where")
        client.expect(r"Debuggee115.main \(Debuggee115:9\)", mark)
        client.expect(r"main\[1\] ", mark)
        mark = client.command("locals")
        client.expect(r"argument = 10\nLocal variables:\n  local is not in scope\nmain\[1\] ", mark)
        mark = client.command("step")
        client.expect(breakpoint_prefix + r"Debuggee115.target \(Debuggee115:5\)\nmain\[1\] ", mark)
        mark = client.command("locals")
        client.expect(r"local = 17\nmain\[1\] ", mark)
        mark = client.command("print Debuggee115.value")
        client.expect(r"Debuggee115.value = 7\nmain\[1\] ", mark)
        mark = client.command("clear Debuggee115:4")
        client.expect(r"Breakpoint cleared at Debuggee115: 4\nmain\[1\] ", mark)
        mark = client.command("cont")
        client.expect(r"debuggee value=18", mark)
        client.expect(r"The application exited", mark)
        mark = client.command("quit")
        client.process.wait(timeout=20)
        if client.process.returncode != 0:
            raise AssertionError("jdb exited with %s" % client.process.returncode)
    finally:
        for name, process in reversed(processes):
            process.close()
            (out / (name + ".txt")).write_text(process.text, encoding="utf-8")
    print("jdb loopback attach/breakpoint/stack/locals/step/continue/quit passed")


if __name__ == "__main__":
    main()
