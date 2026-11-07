# Phase 5 debugger integration evidence

## Windows thread suspension

The first rebuilt debugger session reached the breakpoint and produced a correct
stack trace, but `locals` returned `Thread isn't suspended.` The Windows HPI
called SuspendThread without updating sys_thread.state; agent.getThreadStatus
therefore reported its monitor-wait state instead of suspension.

Original javai_g.dll 0x1004CDD9 saves the previous state, publishes SUSPENDED before
self-suspension, restores the previous state on failure, and writes SUSPENDED
after a successful call. The reconstructed HPI now follows that behavior.
Original sysThreadResume at 0x1004CE42 writes RUNNABLE and calls ResumeThread until
the native suspend count is at most one. It returns an error on the API's -1
result. The former reconstruction resumed only once and left nested suspension
active. Suspend115 observed that difference against Sun before the fix and now
passes: two Java suspends are cleared by one Java resume.

The original writes SUSPENDED even after a self-suspended thread resumes; this
inherited state behavior is retained, not claimed corrected. The debugging
fixture exercises external suspension at a breakpoint.

## Session oracle

tests/debugger/session115.py starts an owned Java EmptyApp server and attaches
an owned jdb client through 127.0.0.1. All waits are bounded and both processes
are reaped on failure. Each mode uses the same original classes.zip.

- Normal: load Debuggee115, enumerate its methods, stop at target line 4, inspect
  caller main line 9 and argument 10, observe local out of scope, step to line 5,
  inspect local 17 and static field 7, clear the original breakpoint, continue to
  value 18, observe application exit, then quit the client.
- Stopped: quit the client at the first breakpoint. The attached EmptyApp server
  remains harness-owned and is terminated; this proves client disconnect while
  paused, not normal server shutdown.
- Release: the original Agent.initAgent rejects a non-debugging VM with
  IllegalAccessError, `not a debugging VM`, after a connection is made. The test
  requires the same result; it does not expect breakpoint support in Release.
- Automatic launch: the unchanged RemoteAgent invokes java.home/bin/java_g.
  A Debug-only installed launcher alias makes that path available. This mode
  installs a fresh image and starts jdb directly. On Windows a native helper
  assigns jdb to a kill-on-close job before it runs, containing java_g as well;
  Unix uses an owned process group. Failure cannot leave the child server running.
- Caught/uncaught exceptions: Exception115.fail throws IllegalArgumentException.
  The client checks the exception message and native exceptionStackTrace, inspects
  the stopped frame, and continues. Sun reports the uncaught exception twice
  while unwinding through runMain; the test checks that count and application
  exit. The caught variant continues into its Java catch block.

The harness overrides JAVA_HOME only in its child environment, preventing an
unrelated modern installed JDK from redirecting the original jdb child launch.
It preserves the user's environment. Config-specific tests use add_test's
CONFIGURATIONS option; a property named CONFIGURATIONS does not filter CTest.

Original Sun Windows Debug normal, stopped, automatic launch, caught and uncaught
modes have passed. Sun Release rejection passed. Rebuilt Windows Debug normal,
stopped and installed automatic launch passed after the thread fix; rebuilt
Release rejection passed. Remaining platform and full-suite results are recorded
in the Phase 5 completion section when verified.

## Native exception returns

Original javai_g.dll legacy invokers at 0x10012FAC and 0x100130AA notify the
debugger when a native call returns with an exception. The synchronized invoker
releases its monitor first. The rebuilt uncaught session initially stopped only
once; adding these callbacks produces the original two stops across runMain.
Rebuilt Windows caught and uncaught sessions then passed. JNI invokers have the
same ordering requirement, verified independently by the callback probe described
in debugger115-jni.md, including exception identity and released monitor ownership.

Repeated-session verification uses fresh owned servers and clients. The stopped
case proves debugger-client exit while the target is paused, matching Sun; it
does not claim target shutdown or reconnecting to the same VM after client exit.

TTY.breakpointEvent prints its prefix before a remote stack fetch, then prints
the frame. Its command thread can print a prompt during that fetch, as observed
on Linux. The harness permits that one interleaved prompt and still checks the
exact frame, source line and final prompt. Three repetitions each of normal and
stopped sessions passed on Linux and on both reconstructed/Sun Windows VMs.

Unix installed-image tests use a fresh directory on the host temporary filesystem
and record the path in installed-image.txt. This avoids WSL mounted-drive metadata
latency without changing the real cmake installation or runtime checks.

## Integration decisions and limits

- Launcher recovery was committed separately from agent recovery; that first
  checkpoint alone did not claim working sessions. Subsequent checks cover them.
- Full debugging is a Debug-configuration feature, as in Sun's libraries.
  Release tests require the original non-debugging-VM rejection. CTest uses
  add_test CONFIGURATIONS to enforce that distinction.
- Paused exit means client quit with a paused target. The remaining target is
  harness-owned and terminated; same-VM reconnection and target shutdown are not
  claimed. Repetitions use fresh processes.
- ODBC preserves the original bound-buffer lifetime, including the documented
  forced-GC hazard between bind and execute. See ../odbc/PROVENANCE.md.
- Native equivalence claims cover the documented probes and real local driver,
  not every malformed argument, optional catalog/output-parameter path or driver.
- Unix installation probes use temporary host directories and record their
  locations; Windows probes use fresh build-output directories.
- High-address heap-profiler hashes intentionally use unsigned arithmetic. This
  changes the original broken high-address case while preserving low-address
  behavior; profiler115-evidence.md records the failing and passing evidence.

A fresh whole-branch review found no additional concrete implementation defect.
Its pending test gates are closed only by the final matrix recorded below. There
were no deferred minor code findings. Regression execution additionally found
and corrected three AWT fixture synchronization/performance issues and the heap
profiler high-address defect; none was waived to obtain a passing run.

## Phase 5 completion matrix

Final full builds and serial CTest runs on 2026-11-07, after the profiler fix:

| Platform | Configuration | Passed | Skipped | Failed |
| --- | --- | ---: | ---: | ---: |
| Windows | Debug | 196 | 0 | 0 |
| Windows | Release | 191 | 0 | 0 |
| Linux (WSL Ubuntu 26.04) | Debug | 124 | 1 | 0 |
| Linux (WSL Ubuntu 26.04) | Release | 119 | 1 | 0 |

The Linux skip is mmedia115-device because no audio device is available. The
full suites include exact 47-entry agent export checks, Windows 69-entry ODBC
export checks, native parity probes, debugger sessions, JNI callbacks, VM and
self-hosted javac regressions, and the applicable AWT/tool suites. Local logs are
out/phase5-final-{debug,release,linux-debug,linux-release}-tests.log.

Fresh installed Windows Debug/Release images each contain ten DLLs whose hashes
match the build and pass jdb fixtures and the real original/rebuilt ODBC tests.
Fresh installed Linux images each contain eight shared libraries matching the
build bytes after only CMake RUNPATH relocation. Installed Debug performs a full
automatic-launch debugger session; installed Release reproduces Sun's rejection.
The final full Debug suites also exercise a newly installed image on each host.

Normal and paused-client-exit sessions were repeated three times per platform,
including the Sun Windows counterparts. The profiler regression separately passed
ten repetitions per Linux configuration. Phase 5 has no remaining unchecked gate.
