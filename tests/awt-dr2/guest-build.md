# Native DR2 build and verification

Use the private Intel snapshot described in intake.md. Stage the source tree with
LF line endings, preserving binary fixtures and dependency bytes. The old Apple
preprocessor treats CRLF continuations incorrectly. Stage the original baseline
vendor/jdk1.1.5/lib resources separately; do not substitute the guest's classes.jar.

From the staged repository root:

```sh
/bin/gnumake -f build/rhapsody.mk check-types check-core
/bin/gnumake -f build/rhapsody.mk check-libraries
JAVA_HOME=`pwd`/out/dr2
DYLD_LIBRARY_PATH=$JAVA_HOME/lib
export JAVA_HOME DYLD_LIBRARY_PATH
mkdir -p "$JAVA_HOME/tests/selfhost"
"$JAVA_HOME/bin/javac" -d "$JAVA_HOME/tests/selfhost" tests/vm/Hello115.java
"$JAVA_HOME/bin/java" -classpath "$JAVA_HOME/tests/selfhost:$JAVA_HOME/lib/classes.zip" Hello115
```

This builds the reconstructed core dylib, java, javah and the Java-class tool launchers
with Apple cc-771.4/GCC 2.7.2.1 and GNU Make 3.74. Outputs remain under out/dr2;
nothing is installed in JavaVM.framework. JRE, AppKit and SoundKit audio are now
included in the private image. Phase 7 acceptance and final review are complete
within the donor limitations recorded in demo-results.json and bridge-evidence.md.

The Makefile uses `-traditional-cpp` to avoid cpp-precomp's crash on jni.c and
incorrect parsing of check_code.c, and `-fno-common` because the DR2 dylib linker
rejects common symbols. Its awk native registry was compared byte-for-byte against
the current CMake-generated registry on the actual guest. Do not reuse a stale
registry from a different build. Preserve source modification times when staging
incremental builds; an archive with every mtime set to zero will hide edits.

Verified native results:

- The VM/JNI integer width and signedness test passes. The SDK has signed BSD
  fixed-width types but no stdint.h; the adapter supplies the two unsigned names.
- All VM core and Unix HPI sources compile and the dylib/launcher link.
- Six exact fixtures pass through check-core: Hello115, NumericConversions115,
  ReflectValues, SerialValues, MonitorCache115 and Interrupt.
- The reconstructed javac compiles Hello115, whose output matches the fixture.
- Threads, Monitors, Exec, FinalizeQueue115, Longs, Dates, Maths and the unchanged
  six-worker Stress fixture pass, including `stress ok finalizers=true`.
- JNI hello and ABI tests pass (callbacks, arrays/fields, wide arguments/results,
  overloaded names and registration). JNI invocation passes defaults, VM creation,
  host-thread attachment/detachment and hooks; DestroyJavaVM retains its expected
  1.1 return value of -1.
- The HPI readiness/processor/barrier probe passes. Four cthreads perform 80,000
  UTC/local time conversions successfully.

Observed corrections include Mach-O JNI symbol spelling/directives, native Mach
CPU queries and i386 fences, BSD socket-length arguments, complete monitor types,
and avoiding duplicate executable/VM OpenCode exports. The math regression was a
header-guard collision: jmath.h suppressed DR2's math.h, so floor/ceil had implicit
integer returns. Renaming the private guard restores the prototypes; the original
Hello115/ReflectValues/SerialValues failures then pass unchanged.

DR2 sysGmtime (0x5c135ca8) and sysLocaltime (0x5c135c7c) copy libc's static tm;
the reconstructed port serializes its callers around that copy because DR2 lacks
gmtime_r/localtime_r. This lock does not serialize unrelated libc callers outside
the VM. Native process creation uses execv for inherited environments, avoiding
the executable-owned environ symbol which cannot be referenced by this dylib.

Remaining diagnostics include ALIGN macro redefinitions and a local relocation
warning in globals.o. Native runtime tests above pass despite those warnings;
they are not treated as proof of complete runtime or AppKit compatibility.

Platform recheck for this checkpoint: Linux Debug passes 161 tests with one
unavailable audio-device skip (261.37 seconds). Windows Debug passes 203 of 204;
awt-Validation115 fails because the unchanged Sun oracle reports `natural label
paint missing: Validation footer`, while the reconstructed implementation passes.
Two isolated repeats, including after guest stress ended, reproduce that oracle
failure. It remains an open acceptance issue, not a passing Windows suite.

## Networking

The native Makefile also builds libnet.dylib. It omits the legacy 1.0 socket_md.c
from the core: those unused native implementations collide with the reconstructed
1.1 net dylib in DR2's flat dyld namespace. Net115, Net115Close and Net115PeekClose
pass with exact fixture output on DR2, covering TCP/UDP, timeouts, options, and
closing blocked operations.

DR2 uses int socket lengths and select rather than poll. Its kernel rejects
select nfds above FD_SETSIZE (256), even after raising RLIMIT_NOFILE and supplying
a dynamically sized mask. Timed waits on such descriptors report SocketException
before accessing the fixed mask. tests/native/mach115-net-wait.c verifies normal
timeout/readiness and safe rejection at descriptor 300. The Linux poll path is
unchanged; Net115HighFD's 1,100-descriptor expectation is not a DR2 capability.

## Support libraries and native tools

The Makefile builds ZIP, BigInteger/bnlib, systemresource, JPEG and agent dylibs.
check-libraries verifies ZIP and BigInteger output, both file and ZIP resource
lookup, all JPEG output/error fixtures, and javah JNI headers against the saved
Sun results. The guest's reconstructed javah also generates the agent headers and
legacy stubs. Dependency license/provenance files are copied into the private image.
The only codec/arithmetic source correction is restricting the visibility attribute
to GCC 4 or newer; DR2's GCC 2.7 rejects that syntax.

GNU Make 3.74 selects the first matching pattern rule rather than the shortest
stem, so specialized library rules precede the generic object rule. The actual
guest check-libraries target passes, including JPEG malformed input, cancellation
and callback exception fixtures. Targeted library checks pass 6/6 on both Linux
and Windows Debug.

The agent's cthreads status mapping follows libagent.A.dylib at 0x5bd0118c,
including sleeping on the thread's own monitor, pending suspension and breakpoint
suspension. mach115-agent-status covers all eight native state values and both
pending-suspend flags. Linux agent checks pass 6/6; Windows checks pass 10/10.
This does not yet establish the full DR2 debugger protocol or suspend/GC acceptance.

The native loader no longer searches JavaVM.framework by default. The guest has
donor libawt/libmmedia symlinks there, so retaining that fallback could conceal a
missing reconstruction. A failing-then-passing native path check enforces this
boundary; core fixtures still pass with the fallback removed.

Dependency ruling: the DR2 mmedia donor uses SoundKit objects through AppKit's
NativeMethodEnter/Exit bridge. Recover audio after the Task 3 bridge rather than
forcing it into the console-only portion of Task 2. Audio remains required for
Phase 7 acceptance; the recovered backend and its checks are described below.

## Private installation and JRE

Run `/bin/gnumake -f build/rhapsody.mk check-install INSTALL_DIR=/private/tmp/jdk115-phase7/installed`
from the guest source root. Installation requires an explicit destination. It
copies native binaries into libexec and shell launchers into bin; the launchers
select the adjacent private libraries for DR2 dyld. The VM and JRE discover their
home through the SDK's `_NSGetArgv`, and JRE uses NSAddLibrary and Mach-O symbol
lookup instead of unavailable dlfcn APIs. Loaded images remain process-lifetime.

The check copies the installed image to a fresh directory containing a space,
unsets JAVA_HOME/DYLD_LIBRARY_PATH/CLASSPATH, changes directory, compiles Hello115,
compares java and jre output with the exact fixture, exercises absolute, relative
and PATH invocation, and checks installed javah output against the Sun JNI header.
The real DR2 run passes all checks. Private relocated images are retained beside
the installation for inspection. External executable symlinks are not covered.

## Native Debug image and debugger

`DEBUG=1` selects out/dr2-debug and enables DEBUG/TRACING/JCOV, including java_g
for the unchanged RemoteAgent launcher. Keep Debug and release outputs separate.
A fresh Debug image builds successfully on GNU Make 3.74. Agent stub objects use
an explicit static pattern rule: the old implicit rule failed in the first build
even though its stamp recipe had generated the source files. zlib excludes DEBUG
as in the existing CMake build. Debug check-core, check-libraries and check-install
pass on DR2; the Java repeated-suspend/single-resume fixture also passes.

Compile tests/debugger/Debuggee115.java and Exception115.java with the guest javac
`-g -d /private/tmp/jdk115-phase7/debug-classes`. On the host, run:

```
python tests/awt-dr2/debugger-ssh.py --host phase7 --port 2637 --ssh-config <private-config> --home /private/tmp/jdk115-phase7/tree/out/dr2-debug-clean --classes /private/tmp/jdk115-phase7/debug-classes --out out/rhapsody115/debug-session-attach --mode attach
```

Authentication comes from normal SSH config/agent/askpass. Repeat with modes
caught, uncaught and stopped and separate output directories. Both Java processes
execute inside the guest; the host reuses the existing protocol assertions.
All four modes pass, including exact stack/local/step checks, two uncaught
continuations and disconnect while stopped. The driver owns and closes each
remote process. Mode `launch` additionally starts jdb without an attach password,
so the unchanged RemoteAgent launches java_g itself. That mode passes the same
breakpoint, stack, local-variable, stepping, continuation and quit assertions on
the native DR2 Debug image. No launcher or VM correction was required for this gate.

Caught/uncaught tests initially failed: the cthreads bounded wait saw the reported
exception as a pending Thread.stop and immediately resumed the throwing thread.
The callback now temporarily clears that exception while waiting, retaining its
GC roots and restoring it unless a new asynchronous exception arrives. Donor
sysMonitorWait at 0x5c1362f4 does not inspect pending Java exceptions; this correction
is specific to the reconstructed bounded-wait adaptation. The protocol fixture
also accepts the observed asynchronous run acknowledgement between event text and
its prompt without relaxing the stack, locals or continuation assertions.

## Installed AppKit image

The native image now includes reconstructed libawt and libmmedia alongside the
separate, byte-preserving rhapsody-peers.zip. The baseline classes.zip is unchanged.
DR2 selects RToolkit by default; VM invocation, tool launchers and JRE include
the peer archive in their default runtime paths. Explicit Java classpaths retain
their normal override semantics.

`check-installed-awt` copies the installed image to a fresh path containing a
space, unsets home/library/classpath environment variables, compiles a plain AWT
fixture, and launches it through both java and jre without a toolkit override or
test native library. It opens/disposes a Frame/Button and loads mmedia. Both pass;
installed AWT/audio files compare byte-for-byte with the reconstructed build.
The relocated console/JRE/javah gate and check-core/check-libraries also pass.
The sound-device probe returns -1 on this guest, matching the donor; controlled
SoundKit conversion and JNI failure/retry tests pass, but audible playback has
not been established.

## Demo acceptance runner

The installed-image runner is `tests/awt-dr2/demos.py`. It consumes the same
81-case inventory as Windows and Linux. The helper invokes unchanged demo mains
or the existing AppletViewer helper; it does not rewrite demo classes or disable
AppletViewer security. Because RToolkit does not populate SunToolkit.peerMap,
DR2DemoObserver115 enumerates AppKit's Java windows through a test-only native
probe, then uses the shared public-component snapshot format. The probe posts
AppKit mouse events and invokes native window activation/resize/close; keyboard
input comes through the private VM's QMP connection. QMP also captures the desktop
with each client raised for inspection. The runner verifies the QEMU guest name
before sending input. Authenticate through SSH config/agent/askpass, never a
password argument.

Compile DR2DemoObserver115.java, tests/awt118/DemoObserver115.java and
WinAwtDemoApplet115.java into a private helper directory with the reconstructed
javac. Build demoprobe115.m as a dynamic library linked to the reconstructed
AppKit library and system AppKit/Foundation. Supply that directory as --helpers.
The Animator pilot checks changing image pixels before input, stable pixels after
pause, native interaction, screenshots and clean window closure. The complete matrix has 81 clean functional runs and 96 inspected client views;
see demo-results.json for hashes, provenance and donor limitations.

DR2 acceptance uses bounded waits of up to two minutes for initial and nested
windows, thirty seconds for native command acknowledgement, and ten seconds for
asynchronous application exit after closing. The Collate demo constructs and
formats its Unicode collation tables in the event callback; a native stack capture
showed active String construction when the earlier thirty-second window timeout
expired. Waiting for completion preserves the unchanged demo and its functional
choice/closure assertions. It does not require a runtime workaround.
