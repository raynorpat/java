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

## Native view state and hit testing

AwtView state (5400ed94--5400f51c) now preserves peer identity, retained font,
color and cursor values, immediate Java-view color/font inheritance, enabled-state
inheritance across native views, event eligibility, and cursor/tracking rectangles.
The enabled setter calls resetCursorRects, as confirmed by its selector; it does
not request a redraw. NSWindow's deepest-view lookup (54010b20) converts window
coordinates into the content superview and walks upward to an eligible view.

The unchanged view115 probe passes against donor and reconstruction. It checks
nested and detached views, an intervening ordinary NSView, mouse-motion gating,
disabled hit testing, cursor identity and exact release of shared retained values.
The reconstruction initially failed because AwtView was absent; the native
check-view target now passes. This standalone checkpoint does not yet implement
Canvas drawing, Java focus callbacks or native Frame creation, and is not linked
into the partial Toolkit test library.

## Native Java-event covers and paint construction

NSEvent coverEventForJavaEvent:inEnv: (54005354), isJavaEvent (540053c8),
javaEvent (54005414), and releaseJavaEvent: (54005434) now use the donor's
NSApplicationDefined/NSAWTEventType cover. data1 owns a JNI global reference;
explicit release deletes it and clears the field. SyntheticEvent marks synthetic
covers. makeWrappedPaintEvent (54006684) constructs a real PaintEvent from the
peer's fJavaCounterpart and a Rectangle whose native floating-point coordinates
are truncated toward zero, then wraps it for native event queuing.

EventCover115 passes against donor and reconstruction: 40 cycles each of object
identity and an otherwise unreferenced object surviving GC, null input, ordinary
event rejection, and cleared handles after release. PaintCover115 also passes
unchanged against both, checking 40 PAINT/UPDATE covers, source identity, fractional
negative/positive coordinates and GC. Its harness allocates an original RCanvasPeer
without invoking native Canvas creation; this isolates event construction. The
donor requires Toolkit initialization to populate its cached JNI metadata.

The reconstruction first failed with an unrecognized cover selector, then with an
undefined makeWrappedPaintEvent. Both durable targets now pass, along with the
original Toolkit startup/callback and screen-value regression checks. Event code
is linked into the test-only AWT library; Canvas drawing, application event routing
and native Frame construction remain unfinished.

## Canvas rendering and background inheritance

AwtCanvas (54003d6c--54003f64) now fills its dirty rectangle with the native
background, flushes PostScript and queues a synthetic Java PAINT cover. Bounds,
visible bounds, focus locking and drawable-window checks follow the donor.
NSView's awtComponentGetsBackgroundFromItsContainer (5400ff44) inspects the Java
Component.background field directly; inherited background updates preserve an
explicit Java color.

CanvasDraw115 runs the same probe against donor and reconstruction. Eight native
window/view lifecycles alternate explicit and inherited Java backgrounds, verify
background propagation, read rendered red pixels, and inspect the exact queued
paint rectangle/source after GC. The probe drains an initial full-window repaint
before the explicit dirty-rectangle operation. Donor dequeuing dispatches covers
itself; the test releases covers returned by a plain queue so either path retains
no pending Java event reference. The reconstructed probe first failed because
AwtCanvas did not exist; check-canvas-draw now passes, alongside check-paint-cover
and check-view. Native RCanvasPeer construction and general input routing remain
separate unfinished integration work.

## Containers and child clipping

AwtContainer (54004214--540044f4) now clips drawing around each child frame,
tracks whether subview clipping is needed, rejects hit tests while disabled,
propagates native-widget notifications and remembers the last entered Java view.
The clip sequence follows rectClipHole/psw_rectClipHole (5401fe4c/5401fdb8): an
expanded outer bounds rectangle and a child rectangle form an even-odd clip.
Successive clips preserve overlapping child regions.

ContainerDraw115 shares the native drawing harness and passes unchanged against
donor and reconstruction. Eight lifecycles check two overlapping child holes with
pixel reads, removal of one child while retaining the other hole, removal of the
last child restoring unrestricted drawing, enabled/disabled hit testing in the
correct superview coordinates, notification delivery to both children, and
last-entered-view identity. Reconstruction first failed because AwtContainer was
absent. Durable check-container-draw, check-canvas-draw and check-toolkit now pass.
This supplies the container used by native Frames; Frame construction, window
callbacks and general event routing are still pending.

## Native Window construction and delegates

AwtWindow construction (5401008c--5401037c) now creates deferred native windows
from frame rectangles, assigns the content view and inset-adjusted bounds origin,
and installs the initial responder without invoking focus callbacks. Borderless,
fixed-size and resizable window styles match the donor. awtSetFrame:display:
(54010720) suppresses delegate feedback during programmatic geometry changes.
The donor requests mouse-move delivery before creating the server window; DR2's
setter ignores that request while the window is deferred. The focused test records
that observed state; live input behavior remains to be verified.

Window delegate callbacks (5401079c--54010a78) now reflect native moves/resizes into
Java Component bounds, preserving the existing 64-bit event mask and enabling
Component events. Y conversion uses the screen's visible-frame height, matching
invertYCoordinate (5401f8d4). Lifecycle notifications queue genuine WindowEvents
through makeWrappedWindowEvent (54006634); the close delegate posts WINDOW_CLOSING
and returns NO so Java retains control over closing.

windowstate115 passes against both runtimes for all three styles, exact frame and
content geometry, responder/delegate identity, and suppressed versus normal resize
notifications. Its subclass declares the complete donor superclass layout because
DR2's fragile Objective-C ABI otherwise underallocates subclass instances.
WindowCallbacks115 passes ten real-peer/native-window lifetimes, alternating move
and resize reflection, all five lifecycle event IDs, source identity, GC and a high
64-bit event-mask flag. Reconstruction initially failed for missing AwtWindow and
then missing windowDidMove:. Durable check-window-state, check-window-callbacks
and check-paint-cover now pass with Window linked into the test-only AWT library.

These probes explicitly invoke native delegate callbacks and construct peers with
AllocObject. Java Frame peer construction, interactive window-manager events,
focus/menu behavior, native disposal and normal application event routing remain
unfinished; these checks do not stand in for those integration gates.

## Original Frame peer integration and native disposal

The original RFramePeer now creates AwtContainer/AwtWindow objects through the
AppKit connection. RComponentPeer font/color setters, container validation redraw
control, Frame title/resizability, and Window visibility/bounds/insets/location/
ordering/disposal are connected. Window115 passes its three show/hide/resize/
dispose cycles using the unchanged peer classes. The native minimum-size helper
(54010bf4) uses the donor's fixed `Hi!` title; SmallFrame115 matches all ten donor
geometry rows, including the Y shift when a short nonzero height is clamped.

End-to-end construction exposed an ABI error that local probes could not: donor
initWithPeer methods encode the JNI handle as `int` in Objective-C metadata.
Encoding it as a pointer to `_jobject` makes NSConnection attempt pointer
marshalling and leaves the caller waiting. Runtime method-type comparisons and
an attached stack trace identified the failing constructor call. View, Container
and Window constructors now transmit the integer value and store the JNI pointer
locally; peer getters retain the donor's pointer return type.

Ruling: disposal releases the window, its allocated content view and the owning
JNI global peer reference instead of reproducing the donor's empty disposal
scaffold. The phase requires repeated resource release. WindowLifetime115 proves
the donor retains a native window after disposal; the reconstruction releases all
five windows and views. Test-only Objective-C subclasses count initialization and
deallocation, and the fixture checks actual native visibility through hide/show
cycles. The compatibility cost is different lifetime behavior around disposal;
callback-time disposal remains an explicit integration gate.

Fresh check-window (normal lifetimes, native ownership and small-frame oracle),
check-container-draw, check-window-state and check-window-callbacks all pass.
The library remains test-only. Java paint/input event routing, focus/menu/widgets,
constructor-failure cleanup and disposal during callbacks remain unfinished;
these results do not establish full AWT acceptance or Java-source collectability.


### Frame conversion failures

WindowLifetime115 now uses a Font subclass whose getSize throws a specific
RuntimeException. Before the change, addNotify rethrows that same exception but
leaks one allocated AwtContainer (0 windows / 1 view). The native tracker counts
allocation rather than initialization so it also detects uninitialized views.
Frame creation now converts Java font, colors, cursor and title before acquiring
the global peer and native view. The failure test then observes zero windows and
views, no installed Java peer, and the exact original throwable. The existing
five disposal cycles and ten small-frame oracle rows remain covered.

A separate exploratory test overriding Frame.setBounds to throw during native
minimum-size adjustment still crashes: GDB shows ProcedureFindThrowTag receiving
a null throwable. Foundation's NSException coder (425738d4/42573928) serializes
only name, reason and userInfo; AwtException has no coder override for its Java
throwable ivar. Cross-connection exception transport and cleanup after native
window allocation remain open. This checkpoint does not claim those paths pass.


### Cross-connection exceptions and Frame rollback

The late construction crash above is now fixed. Foundation copies NSException
name, reason and userInfo, but does not preserve AwtException's added ivar.
Furthermore, a JNI local reference belongs to its originating thread. The
reconstruction now puts a reference-owning object in userInfo: that object crosses
NSConnection by reference and owns a JNI global throwable root. The receiver
retrieves the integer handle and calls Throw with its own JNIEnv. The root is
released when the final owner is destroyed. The exception itself still crosses
by value: making it remote causes DR2 NSConnection to recursively forward raise.

Frame construction keeps volatile ownership variables across NS_DURING's
setjmp/longjmp boundary. On failure, an AppKit-thread helper releases the allocated
view and window, and the JNI entry releases its global peer. Normal disposal uses
the same native cleanup helper.

WindowLifetime115 now checks five repetitions of four failure points: a Java
Font accessor, native view initialization, native window initialization, and a
Java setBounds callback during minimum-size adjustment. Each must preserve or
translate the exception as appropriate, leave no installed Java peer, and return
native window/view/throwable-owner counts to zero. Five normal visible window
lifetimes follow. The owner tracker is reconstruction-specific; this expanded
fixture is not a donor compatibility comparison.

Fresh check-exceptions (java and jre, including Foundation thread teardown),
check-window, check-container-draw, check-window-state and check-window-callbacks
all pass. The small-frame geometry still matches the ten donor rows exactly.
Ruling: retain throwables safely across the connection rather than reproduce the
donor's uncoded subclass field. This adds internal reference ownership and
userInfo state; it preserves Java exception identity and avoids the crash.
This does not yet prove complete Java-source collectability or safe disposal
inside delivered user event callbacks; normal event routing remains unfinished.


### Queued Java events and initial screen graphics

NSApplication's category hook now follows donor 54003110: dequeue:NO calls the
original implementation directly; dequeue:YES dispatches Java covers and returns
the next native event. The original method lookup follows 54003094, retaining the
last matching selector implementation across the DR2 method lists. This also
covers embedded NSApplication instances. The cover path follows 54002adc/54002c58,
including the global-reference boundary and release after dispatch. Native mouse
and key conversion is still pending; non-cover events currently proceed to AppKit.
REventQueue.postSyntheticOtherEvent (54013e88/54006034) now appends SyntheticEvent
covers through the AppKit proxy, enabling the Java window-opened/closed events.

ToolkitStartup115 proved RED when the queue failed to deliver an event, then
passed on donor and reconstruction: twenty queued mouse events follow twenty
direct dispatches, re-enter AppKit during callbacks, run GC, and each release its
native Java reference exactly once. Peeking leaves the event queued. DR2 copies
posted NSEvents, so the probe compares the contained reference rather than object
identity and counts releaseJavaEvent calls on the delivered copies.

Real paint dispatch exposed an UnsatisfiedLinkError for nCreateNativeGraphics.
The original RToolkit catches Throwable and prints a diagnostic, so a process
exit of zero was insufficient evidence. check-window now rejects that diagnostic.
The first screen-graphics group reconstructs surface/color/font ownership,
origin and clipping state, rectangle stroke/fill/background clear, focus locking,
flushing and disposal (54006b48--540071b4, 5400a028--5400a1d4, JNI 54019a38 and
54016144--54017270 for the bound subset). The canvas constructor passes flipped:NO
and calls allocateGState; the canvas itself supplies flipped coordinates.

Raster115 uses real Java Frame.paint calls and original RScreenGraphics classes.
It checks five exact RGB pixels across red fills, green background clearing,
blue clipping after copying and translating Graphics, and an unchanged parent
origin/color. All fifteen pixel checks pass on the donor and reconstruction.
WindowLifetime115 additionally delivers three native window-closing events to
Java listeners which dispose their Frames, sync through AppKit and run GC. Native
window/view counts return to zero and Java peers are cleared each time.

Fresh check-handoff, check-dispatch, check-toolkit, check-window, check-canvas-draw,
check-container-draw, check-window-state and check-window-callbacks pass. The
existing constructor-failure and small-frame oracle cases remain in check-window.
Ruling: pull the initial Task 5 screen-graphics group forward because normal
Task 4 event routing exposes paint callbacks immediately. This changes execution
order, not the remaining acceptance requirements. Lines, text, XOR, shapes,
images, printing, synthetic input and native input translation are still pending.
The partial library remains confined to tests; this is not full AWT acceptance.


### Native mouse conversion and dispatch

The mouse path now follows donor conversion 5400625c, modifiers 54005478/54005cc0,
target selection 54002998, click synthesis 54006424, and the mouse portions of
54002c58/5400fbe8. Press and motion use the deepest eligible view; drag and release
use AppKit's captured _lastLeftHit/_lastRightHit. Consuming a press must not prevent
native capture, while consuming a release suppresses the derived Java click.
Motion subscription follows 5400fd10: old-event components always request motion;
new-event components require the motion mask, a listener, or a lightweight
container dispatcher. The native window's motion flag follows that result.

MouseValues115 checks 144 event-type/modifier/hit combinations against both donor
and reconstruction. It uses real NSEvents and a controlled conversion-view object
to isolate the converter: negative fractional coordinates truncate toward zero,
Java wall-clock time replaces NSEvent uptime, right-button events add Meta,
right-button press alone is a popup trigger, and both enter/exit notifications
use a fresh hit test to select their Java event ID. Native window hit testing is
covered separately, not claimed by this controlled-view fixture.

Input115 posts a native press/drag/release sequence into the real AppKit queue of
a Java Frame. Both donor and reconstruction pass listener/source/coordinate/
modifier checks, callback GC and AppKit re-entry, native tracking after a consumed
press, and suppression of the click after a consumed release. The harness first
activates the application: without activation DR2 treats the first click as an
activation click and does not capture its later drag/release.

The reconstruction-only `dispose` case initially exposed stale Java input after
Frame disposal inside mousePressed. AwtWindow cleanup now clears the view's
borrowed JNI peer handle before releasing its owner. Dispatch retains the target
across Java callbacks and checks that handle before conversion and before derived
events. The case now passes with a cleared Java peer and no later drag/release
callbacks. This continues the earlier resource-release ruling rather than
reproducing the donor's empty native disposal entry.

Fresh check-input, check-mouse-values, check-toolkit, check-window, check-handoff
and check-dispatch pass. Hover enter/exit delivery, initial motion subscription
before the first press, keyboard/focus translation, synthetic input and remaining
widgets are still incomplete; this checkpoint does not establish full input
acceptance. The partial library remains confined to tests.

### Phase 7 responder focus checkpoint

Donor AwtView becomeFirstResponder (5400f51c) and resignFirstResponder
(5400f578) call their superclass and queue a non-temporary Java focus event only
on success. NSView dispatchFocusEventGained:temporary: (5400fcb8) appends the
synthetic cover; makeWrappedFocusEvent (540065c4) constructs FocusEvent with
ID 1004/1005 and the supplied temporary flag. The reconstruction now implements
that path and skips callbacks after the borrowed peer handle has been cleared.

Input115 invokes both native responder callbacks on the AppKit thread and checks
exact lost/gained order, Java source identity, non-temporary flags, callback GC,
and Toolkit re-entry. The unmodified reconstruction failed with no focus events;
the donor passed. Window activation can independently emit temporary focus
notifications, so the fixture limits its collection to the explicit transition.
Window key-status focus notifications and hover delivery remain separate work.

Fresh check-input, check-mouse-values, check-toolkit, check-window, check-handoff
and check-dispatch all pass with the responder focus bridge enabled.

Window activation focus checkpoint: AwtWindow becomeKeyWindow/resignKeyWindow
(540104f4/54010598) capture the first responder before calling the superclass,
then dispatch temporary focus gained/lost when that responder accepts event type
101/100. Input115 now checks both permanent and temporary lost/gained sequences,
source identity, flags, callback GC and Toolkit re-entry. The donor passed; the
reconstruction initially failed with no temporary events. After implementation,
all six input/mouse-values/toolkit/window/handoff/dispatch targets pass. Menu
activation within these same donor methods remains part of the pending menu work.

### Phase 7 hover checkpoint

AwtView mouseEntered/mouseExited (5400f5d4/5400f5f8) now call the recovered
NSView dispatchEnterExitPair: (5400fa6c). It queries the window's current pointer
position, hit-tests for an enter/exit target, updates the tracking event location,
dispatches the previous target before the next target, records the new target,
and refreshes motion subscription and cursor state. NSEvent updateLocationTo:
(54005804) directly assigns the location: the donor's emitted x86 shift/test
never takes its invalid-event branch. Mouse dispatchNSEventToPeer: (5400f9a8)
uses the existing Java conversion and dispatch bridge.

Input115 moves a real native window under and away from the current pointer,
invokes its tracking callback with deliberately stale coordinates, and posts a
real native motion event between transitions. Donor and reconstruction both
produce ENTER/MOVE/EXIT, correct current coordinates and Java source identity,
with callback GC and Toolkit re-entry. Motion starts before any button press.
The reconstruction initially delivered no enter event. A reconstruction-only
case disposes the Frame from mouseEntered and verifies its Java peer is cleared.
ContainerDraw115 also checks removal of the recorded hover child clears tracking.

Ruling: retain callback targets through the native autorelease pool and clear
borrowed hover pointers on removal/disposal. This extends the earlier disposal
ruling; reproducing the donor's stale pointer would conflict with actual resource
release in this tree. The cost is an intentional difference from donor lifetime
behavior, whose native window disposal entry does nothing.

The view-state test exposed obsolete standalone linkage after the event bridge
was added. It now links the same test-only AWT library as the other native view
fixtures. Six input/mouse-values/toolkit/window/handoff/dispatch targets passed;
additional input (including hover disposal), view, container-draw, window-state
and window-callbacks targets passed. A NUL-filled section in the combined guest
log obscured input/view output; both checks were rerun directly and passed with
clean output. Keyboard, synthetic input, widgets, menus and the remaining Phase 7
acceptance work are still pending. The partial AWT library is not installed.

### Phase 7 key conversion checkpoint

Recovered nsKeyEventToJavaEvent (54006060), NSEvent javaCharacter/javaKeyCode
(540054c8/54005560), virtual-key conversion (54005d6c), modifier transitions
(54005ecc), and character normalization (54005f40). The complete ordered native
key table at 54026210 is retained, including keypad and shifted-punctuation mask
requirements. Foundation's original letter classification/lowercase conversion
is used. The five character normalization pairs come from 54026680. Java event
time remains wall-clock time. NSApplication records previous native event flags
once before each dequeue loop, as at 54003110/54003080.

KeyValues115 compares 61 native conversion cases against the donor: press/release,
ordinary/control/non-ASCII characters, keypad versus ordinary digits/operators,
shift-dependent punctuation, navigation/function keys, six modifier presses and
unchanged modifiers. Both donor and reconstruction pass; the original
reconstruction failed with the missing converter. The check-key-values target
requires exact output, so an uncaught Java exception cannot pass via exit status.
This fixture does not prove live key delivery or modifier release transitions;
those remain in the next keyboard integration group, along with typed events and
Java-listener changes propagated back to native key events.

Fresh key-values, input, toolkit, event-cover, handoff and dispatch checks pass.
Keyboard/input were also rerun directly with clean output after their earlier
output was lost from the combined guest log.

### Phase 7 live keyboard checkpoint

NSApplication now routes native key down/up/flags-changed events through Java,
following sendNSEventThroughJava: (54002c58). A consumed press suppresses native
handling and typed synthesis; a consumed key release still reaches native
handling, matching the donor. Listener changes are copied back before derived
events: updateToMatchJavaEvent: (540057bc/54005638) updates modifier flags and
replaces both native character strings when Java's character or key code changes.
Character normalization uses the reverse donor table's first match. Typed events
(54006178) use the updated native character and modifiers, and are omitted for
keys the donor marks non-typable. Derived-event JNI references are released even
when dispatch raises an Objective-C exception.

Keyboard115 posts native events to a real Frame and checks press/typed/release
order, consumed presses, listener edits to character/code/modifiers, newline
normalization, arrow-key suppression of typed events, and Shift release followed
by Alt press/release. Listener callbacks re-enter AppKit and trigger GC. Donor
and reconstruction pass these sequences; reconstruction initially delivered no
keys. A reconstruction-only case disposes the Frame from keyPressed and verifies
that no typed/release Java callback follows and the Java peer is cleared.

Fresh check-keyboard, check-key-values, check-input, check-toolkit, check-handoff,
check-dispatch and check-window all pass, with clean streamed output. The window
target includes lifetime failure cleanup and Java painting checks. Command-key
menu preprocessing, editable widgets, synthetic input and queue peek/get bindings
remain pending; this is not full keyboard/widget or Phase 7 acceptance.

### Phase 7 synthetic input checkpoint

Recovered SyntheticEvent key/mouse factories (54005878/54005a3c), the native
modifier inverse (54005cf8), and the key-code lookup (54005e94). Key codes for
modifiers create flags-changed events. Other key press/release IDs create native
key events; typed events are omitted. A zero character uses the first native key
table match, with no inferred alphabetic/digit fallback. Mouse press/release/drag
map to left-button native events, including when Java Meta is set. Clicked, moved,
entered and exited events are omitted by these factories. Native timestamps are
zero, mouse pressure is one, and coordinates are converted through the view.

The donor's five unsupported SyntheticEvent accessors now raise the same AWT
exception: eventNumber, trackingNumber, userData, isARepeat and keyCode. The
original Java EventQueue's two posting JNI methods (54013f80/540140ac) convert
through the view proxy and append non-null native events on the AppKit thread.
They do not wrap or preserve the original Java event object.

SyntheticValues115 checks 21 donor/reconstruction cases, including factory
omissions, modifier/key mapping, zero-character fallback, coordinates, pressure,
time, synthetic identity, unsupported accessors and invalid types. The native
factory fixture uses a controlled view/window object to isolate conversion.
SyntheticInput115 separately uses a real Frame and the unchanged EventQueue peer:
posted keys deliver press/typed/release, posted mouse input delivers press/drag/
release/click, and omitted types remain omitted. Callbacks validate source and
values, trigger GC, and re-enter AppKit. The reconstruction first failed for
missing factories, then for the missing posting JNI entry; both fixtures now pass
against donor and reconstruction. The partial AWT library has 39 JNI entries and
remains confined to tests. Queue peek/get bindings and the rest of Phase 7 remain
unfinished.

Fresh check-synthetic-values, check-synthetic-input, check-keyboard, check-input,
check-toolkit, check-event-cover and check-window all pass with clean streamed
output, including window lifetime cleanup and Java painting.

### Phase 7 queue retrieval checkpoint

Recovered NSApplication nextJavaEventOfType:dequeue: (54002e40), the native event
mask mapping (540066ec), and the three original REventQueue JNI read methods
(540141d4/540142ac/54014384). Peek performs one non-dispatching native queue lookup;
get waits for convertible input and uses the intercepted native dequeue path, so
Java listeners receive that input before the returned event is converted again.
Unsupported Java event IDs return null immediately. The return handle crosses
NSConnection as a 32-bit integer, preserving the DR2 bridge ABI.

QueueRead115 compares donor and reconstruction behavior using an atomic native
probe for non-empty peeks/get, then a normal Java callback for a non-empty peek
across the AppKit connection. It checks event-ID filtering, source/key values,
distinct Java conversion objects, exactly one press/typed dispatch during get,
callback re-entry, and a returned event surviving GC. The initial donor probe
hung because activation had not yet registered a key window; the queued key had
no AWT target. Activation is now a separate step and the probe verifies the key
window before retrieval. The reconstruction initially failed at missing peekEvent.
Both donor and reconstructed queue tests now pass.

Ruling: the reconstruction releases each temporary cross-thread global JNI
reference after rooting a local reference in the caller. The donor returns its
global reference without releasing it. This continues the explicit resource
ownership policy used elsewhere in the port; the intentional difference removes
an internal reference leak without changing returned Java identity or values.
AppKit conversion locals are released as well. The partial library now implements
42 JNI entries and remains test-only. Child components/widgets and the rest of
Phase 7 acceptance are still pending.

Fresh queue-read, synthetic-input, keyboard, input, toolkit, handoff, dispatch and
window regression targets all pass with clean streamed output.

### Phase 7 child-view checkpoint

Recovered shared View/Canvas/Container construction (540124f8/540112e0/54013344),
parent-controlled attachment (5401272c), enabled state (54012890), and bounds
(540129e8). The original constructor's second boolean is visibility, not a parent
handle. Attachment occurs through the Java parent's peer after construction.
The bounds helper is an AWT NSView category (5400f63c), not an AppKit SDK method:
it invalidates the old parent rectangle, changes the frame, and marks the child
for display. Scroll document views retain a zero origin.

ChildViews115 first passed on the donor and failed on the reconstruction's missing
container constructor. After adding constructors it exposed the missing bounds
category. The reconstructed fixture now passes painting, signed origins, resizing,
disable/enable, hide/show, removal, reattachment and three complete lifetimes.
Reconstruction-only allocation tracking also checks Java font exceptions and
native exceptions after view initialization for Canvas and Panel, preserving the
Java throwable identity and releasing every owned native view. Disposal waits up
to two seconds for pending AppKit references to drain, as the window fixture does.
The test target requires exact output and has an owned-process timeout.

Ruling: component disposal releases the native view and global peer reference,
where the donor's 54012954 is a no-op. This extends the existing window ownership
policy: clear the borrowed peer before detaching, dispose on the AppKit thread,
and release the global afterward. Constructor failures use the same cleanup.
The partial test library now has 49 JNI entries. Generic component construction
shares the tested helper but does not yet have a direct Java fixture; scroll
origin handling also awaits the scroll-widget tests. Neither is claimed covered.
Task 4 and Phase 7 remain incomplete.

Fresh check-child-views, check-view, check-container-draw, check-window,
check-input, check-keyboard, check-queue-read and check-handoff all pass in the
native DR2 guest. The window group includes raster painting and failure cleanup.
