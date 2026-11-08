# DR2 native entry boundary

The reconstructed Objective-C boundary begins in src/rhapsody/sun/awt. It is
tested separately and is not yet an installed libawt or a working toolkit.

| Donor address | Recovered behavior |
| --- | --- |
| libawt 54003310 / 54003354 | AwtAutoreleasePool creates a real child pool on the first autorelease; subsequent objects enter that child. |
| libawt 5401d3e0 | NativeMethodExit clears the active flag and drains a nonempty deferred pool after eleven exits. Empty exits do not advance its counter. |
| libawt 5400ec20 / 5400ed44 | AwtThreadContext is retained in NSThread's dictionary under `AwtThreadContext Key`. It uses NSDPSContext.currentContext or NSDPSServerContext.contextForSecondaryThread. |
| libawt 5401d308 | NativeMethodEnter caches that context on the supplied JNI environment's thread. It returns no exit token for nested calls or an application-owned autorelease pool. |
| libjava 5c138064 / 5c13807c | get_awt_cookie/set_awt_cookie follow the JNI environment to its Java thread and native PrivateInfo; they do not use the current OS thread. |

The private Foundation selector `autoreleasePoolExists` resolves at 4261eb32;
AppKit's `contextForSecondaryThread` is at 436263e3. Both are present in the DR2
system frameworks but omitted from their public headers. Decompilation and
disassembly agree on the context dictionary ownership and deferred-release fields.

`autorelease115.m` passes unchanged against the original donor and reconstructed
code: no premature release, release on exit eleven, counter reset for the next
batch, and explicit nested Foundation pool release. `context115.m` also passes
both: existing main-thread pool, distinct secondary context/graphics context,
nested entry with no token, and token/context reuse after exit. Raw cthread_fork
alone does not put Foundation into multithreaded mode: the fixture first starts a
Foundation thread through its public API. Toolkit startup must establish that mode
before Java-created cthreads use Foundation services.

From the guest source root, run:

```
/bin/gnumake -f build/rhapsody.mk check-autorelease
/bin/gnumake -f build/rhapsody.mk check-context DESKTOP_PID=<current-Workspace-pid>
```

The context check adopts the selected desktop namespace only in its own process
and has a 30-second timeout. The standalone donor equivalents link the same test
source against the system JavaVM.framework/Libraries libawt, without reconstructed
Objective-C sources or DYLD_LIBRARY_PATH. Never mix donor and reconstructed AWT
classes in one process.

The cookie contract test passes on Windows and Linux OS doubles, including a
supplied environment belonging to a different thread. Native DR2 core fixtures
still pass after adding those VM exports. Actual JNI callback/exception/GC tests,
AppKit handoff, and complete resource teardown remain required before Task 3 is
complete; these focused tests do not establish them.

## Java and Objective-C exceptions

AwtException follows donor 54006788--54006b2c. It captures a JNI local throwable
reference, clears the pending Java exception while Objective-C handles it, and
throws the same reference back into Java. Generic NSException objects become
RuntimeException with the donor's description prefix. Unknown or absent Java
exception class names use a cached global AWTError class. The donor's literal
`.80` message suffix is preserved; its fixed 512-byte sprintf buffer is replaced
by an allocation sized for the class name.

ExceptionBridge115 and exceptionbridge115.m pass against the original donor and
the reconstructed runtime. Twenty cycles cover six paths: existing throwable,
native NSException, requested Java exception type/message, throwing Java callback,
unknown class and missing class. Each Objective-C handler invokes System.gc before
returning to Java; the fixture checks throwable identity and exact fallback text.
The newly created exception has no Java-side owner until it is rethrown, exercising
the JNI local-reference root through that GC.

`/bin/gnumake -f build/rhapsody.mk check-exceptions` runs the fixture through both
java and jre. The probe library stays in the test directory, outside the installed
image. Core fixtures and relocated installation checks also pass after the link
change below. Concurrent Foundation use and integrated native-entry callbacks
remain separate gates.

DR2 C launchers must link with `-u _objc_msgSend -framework Foundation`. Without
the forced runtime symbol, a later Foundation load reports duplicate `__objcInit`;
without the Foundation dependency, binding the dynamically loaded Objective-C
class module exits before entering its JNI method. The combination passes the
actual dynamic-load fixture without a custom initialization routine. This applies
to Java tool launchers and JNI invocation through jre, so the native Makefile
relinks each when its link recipe changes.

## Foundation bootstrap and Java thread teardown

The VM donor's sysThreadBootstrap (5c144a38) starts an NSThread before Java
cthreads enter Objective-C code. The reconstructed bootstrap now does the same;
ExceptionBridge115 checks that Foundation is in multithreaded mode. The helper
releases its allocation after detach retains the target, avoiding the donor's
one-object leak.

Donor sysThreadExit (5c144e24) selects NSThread.exit when the native cthread's
Objective-C slot at byte 124 is populated. The port reads that slot before
freeing the Java thread record, avoiding the donor's use-after-free, and lets
Foundation release the native thread dictionary before terminating the cthread.
Host-thread detach retains its existing separate ownership semantics.

FoundationThreads115 places a deallocation probe in each of 20 Java workers'
Foundation dictionaries. It passes on the original donor and failed on the
reconstruction at the first exit before this fix. It now passes under both java
and jre through check-exceptions, alongside the exception/GC fixture. The six
check-core fixtures and native repeated-suspend/single-resume check also pass.
These checks cover bootstrap and thread teardown, not complete AppKit startup.

## JNI native-entry integration

NativeEntry115 and nativeentry115.m pass unchanged against the donor AWT and the
reconstructed bridge in a live desktop bootstrap namespace. Four Java workers
make 20 outer calls each, with two nested JNI calls through Java callbacks and
System.gc at every depth. The test checks outer versus nested exit tokens,
Foundation thread-dictionary context, current DPS context, and the VM's cached
AWT cookie after the callbacks. No production changes were needed for this gate.
`make -f build/rhapsody.mk check-native-entry DESKTOP_PID=<Workspace PID>` runs
this check with a 30-second timeout. It does not prove AppKit event dispatch or
Toolkit startup; those remain pending.

## AppKit connection and proxy handoff

The application connection setup follows 54002858 and 54002f6c--54003080:
register a connection name using the donor's ten-name range, expose NSApplication
as its root, record the owning NSThread and JNI environment, and register modal,
tracking and AwtProcessRequestsMode request modes. Dispatcher construction and
port registration follow 54004a14--54004b38. Event dispatch handlers are not yet
implemented; these sources are linked only into focused tests at this checkpoint.

NSObject instance/class appkitProxy, windowProxy and allocInAppkitThread follow
54004c9c--54004de4. Class proxies require the donor's class-method override of
replacementObjectForPortCoder: to return an NSDistantObject. Without it, instance
calls passed but class calls incorrectly ran on the requesting worker; the test
caught that implementation failure before the override was recovered.

`make -f build/rhapsody.mk check-handoff DESKTOP_PID=<Workspace PID>` passes the
same handoff115.m used with the donor. It verifies owning-thread identity for
20 instance and class calls in each of default, modal, tracking and AWT request
modes, direct identity when already on the AppKit thread, windowProxy identity,
allocation on the AppKit thread, and a remote Objective-C exception. Initial
execution without these sources failed at the missing application setup selector.
The returned allocation proxy is autoreleased: the test balances the remote
allocation via a separate method instead of over-releasing the local proxy.
This is not yet a Toolkit startup or Java event-dispatch acceptance result.

## Java event dispatcher

AwtDispatchHandler (54004504--54004a14) attaches Foundation worker threads to the
VM and uses condition states 1 (available) and 2 (request ready). AppKit waits for
completion while accepting distributed-object requests in AwtProcessRequestsMode;
workers tickle that run loop after publishing completion. AwtDispatcher reuses
unlocked handlers and adds another when a nested callback occupies the existing
ones (54004b38--54004c9c). Both Java event and menu-item callback entry points are
implemented. JNI lookup uses the donor class names/signatures and raises
AwtException on pending Java exceptions, matching the donor callback helpers.
Method/class lookups currently occur per callback rather than using donor caches.

`make -f build/rhapsody.mk check-dispatch check-handoff DESKTOP_PID=<Workspace PID>`
passes. Dispatcher115 runs 20 three-level event dispatches plus 20 menu callbacks
that re-enter AppKit, invokes GC at every callback, checks event-source identity
and consumed results, and observes exactly three reused Java worker threads.
The same probe passes against original libawt; reconstruction initially failed
at its missing dispatchJavaEvent: selector. All four handoff modes still pass.

The dispatcher fixture supplies isolated RToolkit/RMenuItemPeer callback classes
in a test-only classpath; they are never included in the original-byte peer archive
or installed image. This isolates the native dispatcher from unfinished Toolkit
startup. It does not establish real-peer dispatch, uncaught Java callback behavior,
worker termination, UI disposal, or full Toolkit acceptance. Those remain gates
for subsequent integration work; no complete AWT image is installed yet.

## Original Toolkit startup boundary

AwtApplication sharedApplicationUsingEnv:awtToolkit: and finishLaunching follow
54003200/5400323c. The blocking RToolkit.run JNI frame roots the toolkit while
AppKit runs; finishLaunching establishes the application connection and invokes
the original peer's notifyRunning method. RToolkit startup entries follow
5401babc--5401bca4, including the genuinely empty initialize entry. No synthesized
Toolkit Java class is used by this test.

All nine RToolkit JNI entries are present in the native test dylib: initialize,
run, appkitIsRunning, tellAppkitAboutAwtToolkit, nDefaultBackgroundColor,
getScreenSize, getScreenResolution, sync and beep. IDA inspection of the freshly
built artifact found exactly these nine Java native functions (SHA-256
85671ff7adff1c5c44ed657c340b28ecd3d2c9bc38e7498d4c46267be7603bd9).
Screen size uses the donor's visibleFrame and upward rounding, resolution uses
NSDeviceResolution's width, and controlColor is converted through the device RGB
space into Color(float,float,float). In the current 800x600 desktop both donor
and reconstruction report 800x580, 72 dpi, and RGB 221,221,221. sync and beep
return normally; this does not establish the separate sampled-audio gate.

`make -f build/rhapsody.mk check-toolkit DESKTOP_PID=<Workspace PID>` uses the
unchanged peer archive and baseline classes.zip. It verifies startup and toolkit
identity, then dispatches 20 mouse events through the original RToolkit callback
into a Component, checks consumption, performs GC, and re-enters AppKit for screen
size. It passes against the donor too. An initial ActionEvent fixture produced
zero callbacks on both runtimes because generic Component does not deliver those
events; the corrected fixture uses an enabled mouse-event mask.

The reconstruction first failed at missing initialize, then getScreenSize, before
those functions were implemented. The test-only library remains under tests/toolkit;
no partial AWT library is installed. Private fixtures now exit explicitly on Java
failure and the durable runner bounds each process to 30 seconds. Dispatcher and
four-mode handoff regressions pass after integrating AwtApplication.

## Window construction dependencies

Window115 passes three original Frame lifetimes against the donor and currently
fails at createNativeFrame on the reconstruction. It covers bounds, insets,
location, title, resizability, hide/show, GC and disposal; this is the next
integration gate, not a passing reconstructed-window claim.

The Frame constructor at 5401599c needs shared Java-to-native conversions.
javaToNSString (5401f3c8) now preserves UTF-16 code units, including embedded NULs
and unpaired surrogates; javaToNSColor (5401f5f8) maps getRGB channels into an
opaque device-RGB NSColor. Its disassembly confirms distinct red/green/blue
channels despite misleading decompiler stack-variable reuse. Values115 passes
unchanged against donor and reconstruction, with seven null/Unicode string cases,
256 distinct color combinations across 20 GC cycles, and null-color handling.
The initial reconstruction probe failed to link both missing helpers; the durable
`make -f build/rhapsody.mk check-values` now passes.

Runtime Objective-C metadata must guide the upcoming view/window reconstruction:
AwtView's peer is at byte 80; AwtWindow's peer is at 132 and its menu bar at 136.
IDA's imported superclass layout mislabeled the menu-bar release in window
dealloc as a release of the JNI peer. A native runtime inventory confirmed the
actual names, types and offsets. The donor WindowPeer.dispose and ComponentPeer
nDispose entries contain only entry/exception/exit scaffolding; native ownership
and disposal therefore require explicit investigation before implementing them.

## Native font objects for window peers

RFontPeer.createNativeFont (54014a9c), javaToNSFont (54014a14), and getFontList
(54014ce0) now follow the donor. Font peers retain a size-12 NSFont after exact,
case-sensitive alias mapping and bold/italic conversion; javaToNSFont converts
that peer font to the Java Font's requested size. Unknown names use the native
system font. The donor maps ZapfDingbats to TimesRoman, which falls back to Charcoal
on this guest; that observed behavior is preserved rather than silently corrected.

FontValues115 compares 88 combinations (11 names, four styles, two sizes) and the
eight-family enumeration against the donor. The fixture includes lowercase dialog,
an unknown name, null conversion and GC between groups. The expected family list
is from the private DR2 v3 guest's installed fonts. Both original-peer executions
match exactly. Reconstruction initially failed to link javaToNSFont, and then
reported missing getFontList before that entry was implemented.

nsToJavaString (5401f438) now returns UTF-16 strings for enumeration. It uses a
freed temporary heap buffer instead of the donor's unbounded stack allocation;
Values115 also checks full native-to-Java round trips for its Unicode/null cases.
These helpers and both Font peer entries are included in the test-only AWT dylib.
Font metrics, rendering and native Frame construction remain incomplete.

## Cursor conversion for native views

javaToNSCursor follows the donor table at 54026cac and function 5401f4a8. It maps
the fixed Cursor constants 0--13 to AppKit cursor selectors, invokes them through
the AppKit class proxy, and returns nil for null or unknown cursor types. Horizontal
and vertical resize pairs deliberately share native cursors. Private diagonal,
wait, hand and move selectors match the donor rather than substituting arrow cursors.

Cursors115 passes unchanged against donor and reconstruction, checking actual
native cursor identities for all 14 types over 20 GC cycles, plus null and an
unknown type returned by a Cursor subclass. The initial reconstructed probe failed
to link javaToNSCursor. This verifies conversion and thread handoff; actual cursor
tracking rectangles and view mouse-enter/exit behavior remain part of window/input
integration. The test uses the original Toolkit peer and a live desktop namespace.
