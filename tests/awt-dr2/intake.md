# Rhapsody DR2 Phase 7 intake

Phase 6 was fast-forwarded to master at 2264e00. Phase 7 starts on
build/jdk115-rhapsody using the approved reconstruction spec; no Rhapsody build
or runtime compatibility is inferred from the completed Linux Motif port.

## Donor boundary

Read-only input: vendor/rhapsody-dr2/System/Library/Frameworks/JavaVM.framework.
The shipped awt.jar contains 51 classes: 38 additional AppKit classes and 13
classes already supplied by baseline classes.zip. The proposed separate archive
excludes all 13 duplicates rather than overriding baseline behavior silently.
Its dependency closure contains 38 classes, 168 native methods, zero structurally
missing classes/members and zero changed referenced instance-field declarations.
This does not verify inherited native offsets, static initialization, access
rules, reflection, bytecode behavior or JNI/runtime compatibility.

SHA256 provenance:

- baseline classes.zip: 750757205976f91cf54017dd4193af0f2abab7e9c829c6a9761389fe090fbb2d
- awt.jar: 3d252bd54aa02ec6f7a0e291eddbf558afdf94d4d8a638edbdcef272b007057e
- classes.jar: 4beeba1d9d1a86ecc4378a376dfd5ad82bcb34570be1286e861cdcb31b0798cd
- libawt.A.dylib: 0e1b5ea4f25f85d9870ed2ac279e7356ac3eba42d280a161933e03f1e5b6a846

IDA inventory contains 178 _Java_ symbols with addresses. All 168 selected peer
native methods match, including REventQueue.peekEvent's overloaded __ and __I
symbols. The remaining entries require shared-class/native ownership review;
an entry match is not an implementation claim.

RButtonPeer.setLabel at 0x5401121c enters NativeMethodEnter, installs an
Objective-C exception handler, converts Java text through javaToNSString,
looks up viewFromPeer, sends awtSetTitle:, translates exceptions through
AwtException.throwToJava:exception:, and leaves through NativeMethodExit.
These are JNI/Objective-C bridge entries, not Motif-style legacy stack stubs.
The bridge and AppKit dispatch rules must be recovered and tested on DR2.

Reproduction from the repository root:

```powershell
python tests/awt-dr2/audit-test.py
python tools/rhapsody115-audit.py vendor/jdk1.1.5/lib/classes.zip vendor/rhapsody-dr2 --output out/rhapsody115/audit.json
```

The audit test first failed because the tool was absent, then caught the two
overloaded peekEvent entry names. After JNI long-name matching, it passes with
all archive hashes pinned. The existing four Motif audit tests also pass.

## Guest selection

The owner identified D:/RhapsodiOS's SSH-ready v2/v3 images. Both exist under
vm/work; v3 retains DR2 libSystem and includes a newer reconstructed kernel,
OpenSSL 0.9.8zh and OpenSSH 7.4p1 (per that project's recorded verification).
Use a private snapshot of rhap-i386-bootstrapped-v3.img, distinct name
jdk115-phase7 and ports SSH 2637 / QMP 4637 / GDB 1637. Do not alter the base
image or the guest's system JDK. The modified kernel must be disclosed when
interpreting runtime results; it is not an untouched DR2 kernel oracle.

Verified over SSH in the private v3 snapshot:

- uname: Rhapsody rhap1 5.6, i386, kernel-155.0.0-7 (October 2026 rebuild).
- compiler: Apple cc-771.4, based on GCC 2.7.2.1; GNU Make 3.74 at /bin/gnumake.
- CMake is absent. Mach/cthreads headers live in System.framework's Headers tree,
  not /usr/include/mach; Apple framework include lookup works.
- A compiled Objective-C/Foundation/cthreads probe prints
  `pointer=4 cthread=4 Foundation=ok` and exits zero.
- The original javac compiles HelloDR2; original java prints
  `DR2 donor Java 1.1.5 x86` and exits zero.
- The graphical desktop was reached with the image's stored login. An original
  Frame/Button smoke test launched over SSH timed out after 25 seconds, both at
  the login screen and after desktop login. This oracle-environment failure was
  subsequently diagnosed and resolved as described below.

The v3 SSH server uses modern algorithms; the neighboring legacy harness's
forced group1/DSS options fail negotiation. Reuse its credential handling with
modern defaults and an isolated known_hosts file. Send multiline scripts to
`exec /bin/sh` on stdin: root's csh rejects multiline quoted command arguments.
All probe sources/binaries are private under /private/tmp/jdk115-phase7.
Host logs and launch/probe scripts are in out/rhapsody115/guest and its parent.

The post-merge checkout exposed CRLF conversion in tests/awt118/with-wm.sh.
Commit 5340bd8 on master pins that script to LF. All three affected WM checks
passed after the fix; full Linux regression verification follows separately.

The initial AWT probe also timed out with awt.jar before classes.jar and after successful
desktop login; its last marker is `before Frame`. Do not assume archive order or
console login fixes the stall. Next investigation: donor Frame/toolkit initialization,
AppKit bootstrap namespace and thread/Objective-C bridge state. No reconstructed
native code was loaded in these probes. The owned snapshot was stopped through
QMP after query-name verification; base image remains untouched. Reboot and
resync the small private probes for the next session.

Final merged-code Linux Debug verification: 161 passed, one unavailable audio-device
skip, 224.35 seconds (out/rhapsody115/merged-linux-debug.log). An earlier run
hit an intermittent Choice popup-selection failure during guest activity; it
then passed three consecutive isolated repeats and this full run. No native
change was made to suppress that failure. The Phase 7 commit adds only audit
tooling and documentation; Windows/Linux native implementation is unchanged.

## Donor AWT startup resolved

SIGQUIT located the wait in RToolkit's constructor, reached from Frame.show,
waiting for the AppKit startup callback. A read-only Mach thread stack walk
located the AppKit thread in Foundation's distributed-notification registration.
The SSH task cannot find `Apple DNC Server (Local Host)` (bootstrap lookup 1102);
the logged-in Workspace task can (lookup 0). Foundation's failure log formats a
date, initializes the system timezone, and recursively registers a notification
observer while the notification lock is held. Logging therefore deadlocks before
AppKit can notify RToolkit that it has started.

Evidence in the Intel donor: RToolkit.run at 0x5401babc,
AwtApplication.finishLaunching at 0x5400323c; Foundation's
NSConcreteMachDNC.sendMessageToServer at 0x425503d0, NSLog at 0x42547b40,
NSTimeZone.systemTimeZone at 0x42558a89, and
NSNotificationCenter.addObserver at 0x4251ad1d.

The test-only desktop-launch.c adopts the selected desktop task's bootstrap
port in its own process, verifies the notification service and execs the requested
command. It does not modify the desktop task or any system framework/JDK file.
Run as root in the private guest after graphical login. Find the current Workspace
PID with `ps ax`; do not reuse a PID from an earlier boot. Stage these test sources
with LF endings in /private/tmp/jdk115-phase7, then run:

```sh
cd /private/tmp/jdk115-phase7
cc desktop-launch.c -o desktop-launch
javac Smoke115.java
# Substitute the current Workspace PID for DESKTOP_PID.
./desktop-launch DESKTOP_PID /bin/sh /usr/bin/java -classpath .:/System/Library/Frameworks/JavaVM.framework/Classes/awt.jar:/System/Library/Frameworks/JavaVM.framework/Classes/classes.jar Smoke115
```

Verified with the original guest JDK: `DR2 AppKit startup and three window
lifetimes passed`, exit zero. The test checks toolkit selection and peer
creation/disposal for three Frame/Button lifetimes. A separate longer-lived
Frame was visually inspected in a QMP capture. Missing arguments and a nonexistent
desktop PID both return failure. This closes the donor preflight gate, not the
reconstructed AWT gate.

## Native compiler preflight

The first actual compile of src/unix/java/runtime/thread_cthreads.c failed on
CRLF preprocessor continuations and missing stdint.h. A regular Windows
`git archive` also applied core.autocrlf; use `git -c core.autocrlf=false archive`
or explicitly normalize staged text. With LF input the remaining errors are
missing stdint.h in typedefs_md.h and jni_md.h, and undefined uint32_t. There is
also an ALIGN redefinition warning. No platform correction is implemented yet.

Reproduction from the staged repository root with the native DR2 compiler:

```sh
cc -c -DIN_JRT -DCOMPILER -DXP_UNIX -DJDK115_CTHREADS \
  -DJNI115_ENABLED -DJAVA_INTERNAL_BUILD \
  -Isrc/share/java/include -Isrc/share/java/include/_gen \
  -Isrc/unix/java/include -Isrc/share/java/runtime \
  src/unix/java/runtime/thread_cthreads.c \
  -o /private/tmp/jdk115-phase7/thread.o
```
