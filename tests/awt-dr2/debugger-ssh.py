"""Run the existing debugger protocol assertions with both processes in DR2.

Use an SSH config/agent/askpass for authentication; this script stores no password.
Compile tests/debugger/{Debuggee115,Exception115}.java with -g in --classes first.
"""
import argparse
import importlib.util
from pathlib import Path
import shlex
import subprocess
import sys
import threading
import uuid


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", required=True)
    parser.add_argument("--port", default="22")
    parser.add_argument("--ssh-config")
    parser.add_argument("--home", required=True)
    parser.add_argument("--classes", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--mode", choices=["attach", "caught", "uncaught", "stopped", "launch"], default="attach")
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location(
        "session115", Path(__file__).resolve().parents[1] / "debugger/session115.py")
    session = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(session)
    ssh = ["ssh", "-T", "-p", args.port]
    if args.ssh_config:
        ssh += ["-F", args.ssh_config]
    ssh += [args.host]
    home = args.home.rstrip("/")
    cp = args.classes + ":" + home + "/lib/classes.zip"

    def remote(command):
        # The guest login shell is csh; explicitly select Bourne-shell syntax.
        return ssh + ["/bin/sh -c " + shlex.quote(command)]

    class RemoteProcess(session.Process):
        def __init__(self, command, env, tree=False, wrapper=None):
            self.text = ""
            self.condition = threading.Condition()
            self.tree = False
            self.pidfile = home + "/tests/debugger-" + uuid.uuid4().hex + ".pid"
            script = "mkdir -p " + shlex.quote(home + "/tests")
            script += " && echo $$ > " + shlex.quote(self.pidfile) + " && exec "
            script += shlex.join(["env", "DYLD_LIBRARY_PATH=" + home + "/lib",
                                  "CLASSPATH=" + cp, "JAVA_HOME=" + home] + command)
            self.process = subprocess.Popen(remote(script), stdin=subprocess.PIPE,
                                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            self.reader = threading.Thread(target=self.read, daemon=True)
            self.reader.start()

        def close(self):
            try:
                if self.process.poll() is None:
                    # Only the PID belonging to this still-connected SSH command.
                    script = "test ! -f " + shlex.quote(self.pidfile)
                    script += " || kill `cat " + shlex.quote(self.pidfile) + "` 2>/dev/null || :"
                    subprocess.run(remote(script), timeout=15, check=False)
            finally:
                super().close()

    session.Process = RemoteProcess
    sys.argv = ["session115.py", "--java", home + "/bin/java", "--jdb", home + "/bin/jdb",
                "--classpath", cp, "--libdir", home + "/lib", "--out", args.out]
    if args.mode in ("caught", "uncaught"):
        sys.argv += ["--exception-mode", args.mode]
    elif args.mode == "stopped":
        sys.argv += ["--stop-after-breakpoint"]
    elif args.mode == "launch":
        sys.argv += ["--launch"]
    session.main()


if __name__ == "__main__":
    main()
