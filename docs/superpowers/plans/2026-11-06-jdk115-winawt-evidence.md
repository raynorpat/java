# Windows AWT 1.1.5 reconstruction evidence

## Source inventory

Primary files are vendor/jdk1.1.5/bin/winawt.dll (229376 bytes) and
winawt_g.dll (325632 bytes). SHA256:

- Release: `7db7bc81d3474781343a6ee12399cf9e0c14f86b1c14d7e74cae7f58796f7169`
- Debug: `d6003d6acd29ef3306e9a59501b8b61c8a2b2d94a7474b114803f02193d24b4b`

IDA lists 166 named exports in each, with identical names. Its extra entry for
DllEntryPoint is the PE entry point, not an exported API. The complete names
are recorded in tests/native/winawt115.exports. They comprise 163 Java native
stubs plus AwtInit, CreateEmbeddedFrame and PrintEmbeddedFrame.

Both binaries import KERNEL32, USER32, GDI32, comdlg32, IMM32, ADVAPI32 and
MSVCRT, plus the matching javai/javai_g VM. The VM imports include EE,
SignalError, ArrayAlloc, FindClass/FindStickySystemClass, monitor operations,
Java string conversion and execute_java_* callbacks. These are the legacy
stack-item/handle interfaces, not a JNI peer library.

The existing src/win32/sun/awt sources are the older MFC implementation:
AwtButton inherits CButton, messages use MFC maps, and generated peers are
sun.awt.win32.M*. The 1.1.5 library instead names sun.awt.windows.W* peers;
debug assertions identify src/win32/sun/windows/awt_Toolkit.cpp,
awt_Component.cpp, awt_Button.cpp, Hashtable.cpp, ObjectList.cpp, CmdIDList.cpp
and their sibling implementation files. MFC is not an imported DLL (this alone
does not rule out static MFC, but old layouts are independently incompatible).

Ruling: place reconstructed 1.1.5 code under the binary's sun/windows path;
leave the unbuilt old sun/awt source available for comparison. Do not enable
that old implementation by merely renaming its peer classes. Cost: substantial
1.1 toolkit/peer reconstruction is required before GUI demos can run.

## Task 1: toolkit queries

| Operation | Debug stub | Debug body | Release stub |
|---|---|---|---|
| getScreenWidth | 1002DD47 | 10023625 | 1001E7E0 |
| getScreenHeight | 1002DD70 | 1002360D | 1001E800 |
| getScreenResolution | 1002DD1E | 1002363D | 1001E7C0 |
| loadSystemColors | 1002DDDB | 100236DC | 1001E860 |

Width and height call GetSystemMetrics(0/1). Resolution acquires a desktop DC,
queries LOGPIXELSX (88) and releases the DC. Each integer query overwrites the
receiver slot and returns sp+1; loadSystemColors uses the array in sp[1] and
returns sp unchanged.

The debug table at 100491B8 has the following 26 indices, matching
java.awt.SystemColor order:
`1,2,9,10,3,19,11,5,6,8,4,7,15,18,13,14,17,15,18,22,20,16,21,0,24,23`.
The helper at 100237F3 converts COLORREF BGR bytes to Java RGB; no alpha is
added. The original loop compares its element index against 0x68 (104), the
table's byte size. The normal Java array length of 26 hides that overread.

Ruling: bound the reconstructed loop by 26 elements and array length. Cost:
oversized arrays retain their trailing entries instead of receiving colors
derived from adjacent binary data. This is an explicit bounds fix, not a
claim of bit-for-bit behavior for malformed/nonstandard calls.

The native contract executable loads Sun's DLL with its own dependency search
directory, then compares all three screen queries and color arrays of every
length from zero through 26. It verifies stack advancement, exact colors and
untouched storage beyond the array. An oversized-array check exercises only
the corrected implementation, avoiding an unsafe oracle overread. No windows
or toolkit threads are created by this test.

RED: CMake could not generate the target because awt_Toolkit.cpp did not exist.
GREEN: the implementation builds as C++ and the comparison passes against
Sun's debug DLL. Initial DLL loading failed with ERROR_MOD_NOT_FOUND because
LOAD_WITH_ALTERED_SEARCH_PATH received a CMake forward-slash path; normalize
with GetFullPathNameA before loading. No library search configuration outside
the test process was changed.

Final slice verification: comparison passes against both Sun debug and release
DLLs. Full Windows Debug115/115 and Release115/115; Linux Release108 passed
plus one expected audio-device skip. Build/test logs:
out/phase4-toolkit-win-{debug,release}-{build,tests}.log and
out/phase4-toolkit-linux-{build,tests}.log. Linux does not enable C++ or compile
the Windows target. Only the query slice is complete; GUI milestone gates remain.

## Task 2 starting evidence

WToolkit.init is an instance native taking java.lang.Thread. Its native stub
passes only the receiver to body100234A0. The body reads the four debug flags
at Java object offsets0/4/8/12, stores the toolkit handle and initializes the
singleton. AwtInit1002236F initializes that same singleton in embedded mode.
Initialization100227A0 rejects repeat initialization, checks color depth,
records the current thread ID, creates a toolkit window and installs a
thread-specific WH_GETMESSAGE hook. eventLoop stub1002DCD8 calls1002354A.
These dependencies must be recovered together before enabling a toolkit DLL.
The recovered query subset alone is deliberately not installed as winawt.

## Toolkit, window and component graphics foundation

Implemented init/eventLoop/sync/beep/AwtInit and true-color makeColorModel,
WComponentPeer methods, WWindowPeer methods, WFramePeer.create, WColor defaults,
and the component-backed subset of WGraphics. This is a partial DLL, not the
Phase4 completion gate. Controls, input replay, full graphics/images, printing,
embedding and demo parity remain open; it is not installed yet.

Initialization uses class AwtToolkit, title The AWT Toolkit Window, WS_DISABLED,
and a thread-local WH_GETMESSAGE hook (10022729/100227A0). The hook is currently
pass-through pending input recovery. The hidden window dispatches synchronous
requests with message 0x464, matching the stack-owned request at100229A4.
Shutdown uses0x466 and sysAtexit; the event loop uses WaitMessage/PeekMessage.
Component records are retired on the HWND owner thread and clear Java pData.
Java's SunToolkit peer map roots live peers; native requests remain on a Java
thread's stack until their synchronous owner-thread operation returns.

Frame creation10010C30 uses style0x02CF0000/exStyle0x100; window creation10023D0E
uses0x82000000/exStyle0. Insets10023E70 come from outer/client rectangles.
Title10024DAF uses Unicode on NT. Front/back10024CCE/10024D46 use SetWindowPos.
WColor10005210 maps selectors1..9 to5,8,6,0,4,7,15,18,13 and constructs Color(III).
True-color initialization10018F52 caps depth at24 and uses BGR masks
0xff/0xff00/0xff0000. Sun's color model returns86/52/18 for0x123456; the oracle
caught an initially incorrect conventional-RGB expectation. Palette and
16-bit device masks remain for the image task.

Graphics10011993/10011A5A acquire/reference the component surface and subtract
top-level insets from the Java origin. Clip1001449C is stored in device
coordinates;100146BF subtracts the current Java origin on retrieval. Copies
have independent clipping state. Clear10012672 uses the component background.
The reconstructed surface record is reference-counted and invalidated before
HWND destruction so retained Graphics cannot draw into a recycled HWND.
The full Sun AwtDC implementation and image/print surfaces remain separate work.
Window paint100245B6 validates the update region, adds insets and calls
handleExpose(IIII)V. Component paint10007280 and callback1001E341 supplied the
Java method/exception contracts.

Ruling: serialize native retirement and drawing on the owner thread rather
than reproduce the original disposal's PostMessage/500ms timeout/delete race
(10009D38). Cost: a stalled toolkit stalls disposal instead of returning after
500ms with potentially live queued references. Paint callbacks are queued by
HWND with accumulated rectangles, without heap-owned event payloads.

Ruling: bring the component-backed graphics foundation forward from Task4.
The stronger lifecycle test exposed createFromComponent failures on the Java
event thread during resize repaint. Cost: Task2 and Task4 cannot be verified
as completely independent units. Only implemented exports are emitted; missing
menu/icon/image methods are not filled with placeholder success bodies.

Ruling: use the current system hand cursor rather than Sun's HandCursor resource;
bound getContainerElement indices rather than reading outside the component
array. Costs: host cursor artwork differs, and malformed calls throw instead
of accessing unrelated memory. Native font records currently capture face,
size and style; logical-font/multifont mapping remains in Task4.

Toolkit115 was RED for the missing DLL; Lifecycle115 was RED for create,
getDefaultColor, createFromComponent, and an explicit initial paint callback.
Tests now cover repeated/worker window creation, title/bounds/insets,
show/hide/dispose, resize bursts, GC in paint/resize callbacks, clipping,
translated graphics copies and graphics retirement after frame disposal.
Both Sun and reconstructed Release pass Toolkit115, Lifecycle115 and
Graphics115. Sun does not reliably issue an initial expose on this host, so
the shared paint contract explicitly requests repaint. The harness rejects
background exception/error output even when main exits successfully.

## Native events and first child controls

WindowEvents115 uses a small native test driver to move and close only HWNDs
belonging to its own process. The initial test failed on missing native
geometry events. Move100243F7 updates target x/y and posts ComponentEvent100;
resize1002447C updates width/height and posts101. Close100243A4 posts
WindowEvent201 and does not destroy the HWND. Constructors10024131/1002425D
and postEvent1001E4AF supply the Java callback signatures. Native events are
queued by HWND before making Java calls, avoiding callbacks inside synchronous
cross-thread geometry requests. Window.dispose already posts202 in Java.

Canvas/Panel creation100031C7 ->10002DF6 uses child style0x46000000. Native
child reshape accounts for its top-level parent's non-client origin. The
Canvas115 test first failed at create, then checks nested Panel/Canvas
coordinates, repeated disposal, retained graphics and focus changes. Sun's
reshape is asynchronous, so the comparison waits for the resulting location.
Focus10006F9D/10006FC9 ->10009078 creates FocusEvent with signature
(Ljava/awt/Component;IZ), IDs1004/1005 and the recovered temporary-focus rule.
The focus test was RED for missing focus gained before these callbacks.
WM_NCDESTROY also invalidates surviving graphics surfaces, including when
Windows destroys children as part of parent HWND destruction.

Button115 was RED at create. Creation10002435 uses BUTTON with0x4400000B
(owner-drawn). BN_CLICKED100025B5 calls handleAction()V. Draw100025F2 fills the
background, draws raised/sunken edges, centers text, adds the focus rectangle,
and calls handlePaint. The reconstruction uses a subclassed native BUTTON and
keeps native drawing and action dispatch on the toolkit owner thread.
The test drives BM_CLICK against process-owned controls, verifies labels
including accented text, default/custom action commands, GC in the listener
and retirement. This is a control-action test, not mouse/key replay coverage.
Font-segment painting and disabled-text visual parity remain Task4 work.

All six Java AWT tests pass against the original Sun Release VM and current
reconstruction. Native window/input activation, modal dialogs, the other
controls, full raster/font/image tests and the demo gate remain open.

## Font metrics and corrected oracle home

Font metrics init1000E216 ->1000D2AF obtains the primary font's GDI metrics,
allocates256 integer widths, uses GetCharWidthA and computes fallback maxima.
Font descriptor selection1000DC4A defaults to index0 for an unknown descriptor;
creation1000C63B/1000CB6D and charset parsing1000CEB7 specify native face/style,
negative pixel height and charset. With font.properties absent, the recovered
fallback Dialog/DialogInput face is MS Sans Serif. With properties present,
the unchanged Java PlatformFont supplies Arial/Courier New and symbol segments.
needsConversion1000E864 recognizes NEED_CONVERTED; segment widths1000E5F8 use
GetTextExtentPoint32A for converted bytes and W for UTF-16. Component/native
graphics font setup now uses that same recovered descriptor mapping.

The Fonts115 test was RED at init. It compares exact metrics and Unicode-string
widths for seven logical/legacy font names and four styles, plus glyph coverage
probes. A first oracle comparison differed in every profile: Sun's launcher
was reading java.home from the machine registry, pointing at an installed
JDK21 without font.properties. Pinning the oracle's java.home to vendor/jdk1.1.5
made all28 profiles match exactly. The build/install copies the unchanged
Windows font.properties. Every AWT CTest now also runs the Sun oracle with its
home pinned and compares output, while checking both processes for exceptions.

canConvert10010380 ->100100E0/100101AA uses font coverage tables.
The standard table1000F3B4 ->1000ED7B selects Microsoft Unicode cmap platform3,
encoding1, format4. Membership1000F163 tests segment starts/ends and excludes
FFFF; it does not check glyph IDs. A provisional GetGlyphIndices implementation
failed the symbol-font comparison because GDI performs additional remapping.
It was replaced with the recovered cmap segment algorithm and checked offsets.
EUDC file/registry fallback1000FDC8 remains open; it is not silently claimed as
implemented by the standard-font tests. Font handles are currently scoped to
each measurement rather than retained in Sun's process-wide font cache.

Ruling: bytesWidth treats input bytes as unsigned indices and validates ranges
without offset+count overflow. Original1000E515 sign-extends byte indices and
can read before its256-entry table. A reconstruction-only assertion for233
was observed RED with the signed index and GREEN with the fix. Costs: malformed
ranges now throw consistently; high-bit byte widths use their actual table
entry instead of unrelated memory. MF segment bounds are also checked before
GDI calls. Unsafe malformed-input tests are not sent to the Sun oracle.

Ruling: missing/truncated Unicode cmap data returns false rather than parsing
the header as format4 at offset0. Sun's release Symbol/SYMBOL_CHARSET probe
reports character0 present after its missing-Unicode-table assertion; debug
1000ED7B explicitly asserts the table exists. The safe implementation rejects
that unsupported table. Cost: this out-of-contract symbol-font query differs;
normal symbol text uses the converted FontDescriptor path, which matches the
Unicode width oracle. The test checks this corrected case only on ours.

## Label and Checkbox peers

Label creation1001AB45 uses a custom child class/style0x44000000. Painting
1001AC91/1001AEC0 fills the client background and positions text vertically
centered, with x=0, centered, or right-aligned according to target.alignment.
Setters1001B0DB/1001B189 update text/invalidate; the alignment argument is not
stored separately from Java. Label115 was RED at create, then verifies actual
native text pixel positions, text changes, sizing and retirement against Sun.
Initial pixel reads returned CLR_INVALID on both implementations: CMake starts
the process with a hidden initial show state, leaving the frame HWND hidden
despite Java's visible flag. The process-owned probe explicitly shows its
frame before pixel reads. This is a harness correction, not a native peer fix.
Multifont text drawing remains part of Task4 integration.

Checkbox creation1000344F uses owner-drawn BUTTON/style0x4400000B. Command
100035BE toggles Java target.state (10003622) and calls handleAction(Z)V;
the Java method handles CheckboxGroup exclusion and posts ItemEvent.
Draw10003669 uses GetSystemMetrics(50) for the check size, DrawFrameControl
with radio/check/pressed/selected flags, label text and focus rectangle.
Setters10003BB4/10003C92/10003EEB update style/state/text and invalidate.
Checkbox115 was RED at create; checks and group exclusion, changing group,
accented labels and native item events now match Sun. The oracle corrected an
initial test assumption: removing a selected checkbox from its group clears
its state in Java1.1.5 before the next click. No Java library change was made.

Ruling: remaining Task4 is delegated under the subagent-driven-development
skill, with separate source/build ownership and a task review. The coordinator
continues controls/integration. The established AwtSurface and owner-thread
contracts make these areas separable. Cost: additional review/integration work;
CMake edits must be serialized between implementer and coordinator.

Choice control slice: binary 10004135 create (0x44200013 owner-drawn
COMBOBOX without HASSTRINGS), 10004395 native-height centering/drop-down
sizing, 1000455E selection callback, 10004686/1000473A/10004812 item mutation.
100086E7 draws target.getItem(index); 100086A6 routes item measurement.
Control IDs now route WM_MEASUREITEM even during synchronous native creation.
Choice115 first failed with missing create, then passed against Sun for native
selection, insert/delete/repopulate and item events under GC. Other control
regressions passed except one Lifecycle115 shutdown timeout after its success
output; five immediate repeats passed. This intermittent shutdown issue stays
open for the Task2 lifecycle gate. Primary-font painting still awaits Task4
multifont integration. Logs: out/phase4-choice-*, out/phase4-lifecycle-repeat.log.

Choice/List/Scrollbar control checkpoint: Choice115, List115 and Scrollbar115
exercise native selection, single/multiple list HWND recreation, insertion,
deleting/repopulation, item/action callbacks, both scrollbar orientations,
line/page increments and tracking. RED was missing create for each; Release
and Debug compare against Sun. Shared Choice/List owner-drawing routines
100086A6/100086E7 now use Task4's recovered multifont text helper, also used by
Label/Button/Checkbox. Six control comparisons pass in Debug; five multifont
control comparisons and Scrollbar separately pass in Release. List evidence:
1001B2C9 create, 1001B467 mode recreation, 1001B7DA event routing,
1001B968..1001BEC0 mutations/selection. Scrollbar evidence: 1001EE93 create,
1001F041 range, 1001F10B callbacks, 1001F447..1001F600 native setters.
Parallel generated-header corruption revealed CRT tempnam name collisions;
CMake isolates javah temporary directories per build and generator group.
Phase4 remains open for text/menu/input/remaining interfaces and final gates.

Text/ScrollPane/Dialog/window-state and lifetime checkpoint:
- Text115 matches Sun NT Unicode editing, CR/LF insertion/removal, native
  selection offsets, replace/insert, password/read-only setters and native
  text/action events. Evidence1002098A..100221F7.
- ScrollPane115 matches all policies, viewport/insets, spans/clamping and
  native H/V callbacks. Evidence1001F675..100207C2. Insets/span reads preserve
  original caller-thread ordering; a peer critical section coordinates them
  with native retirement without dispatching an extra owner-thread request.
- WindowState115 matches activation/deactivation and iconify/restore events
  (10010FA4,1002447C). Dialog115 matches modeless/modal creation, disabled
  owner windows, blocking show and close callbacks (1000AFC0..1000B94F).
- Review identified reentrant Java getters deleting a native painting peer.
  ReentrantPeers115 reproduced Debug exit0xc000041d before correction. Native
  leases now retain the C++ object and Java peer during callbacks/requests;
  disposal clears pData and destroys HWND immediately, deleting the native
  object after leases unwind. Drawing stops after reentrant disposal.
  Extended coverage includes Label/Button/Checkbox/Choice/List, parent
  disposal, GC and Choice reshape. Choice metrics run on the caller thread
  before HWND dispatch, avoiding callback treeLock deadlock.
- PaneRace115 races retained peer span/inset calls against disposal+GC.
  Safety cases are ours-only; unsafe Sun behavior is not executed. Task4
  also makes graphics acquired during retirement inert to tolerate queued
  Java paint events after native detachment.
- Combined Debug AWT31/31 passed, logs out/phase4-controls-lifetime-debug-*.
  Release AWT30/30 passed before Dialog and final low-depth integration;
  Dialog separately passed Release Sun. Full final platform/export/demo
  gates remain open. Controls scoped re-review pending.

Controls scoped re-review accepted both P1 lifetime fixes and identified a
P2 cleanup gap in early disposal exits. Label now retains the original paint
HWND and always reaches RestoreDC/EndPaint; Button/Checkbox/Choice/List shared
drawing also reaches RestoreDC without further painting after retirement.
Debug ReentrantPeers115, PaneRace115 and GraphicsResources115 pass3/3 after
this cleanup correction (out/phase4-paint-cleanup-debug-build.log).

FileDialog slice: recovered original ANSI common-dialog ABI at 1000BFF0,
OPENFILENAME version400/flags0x200006, parent ownership, initial directory/file,
and selected/cancel callbacks. Initial filename is bounded to MAX_PATH rather
than retaining Sun's overflow. FileDialog115 compares cancel, open/save selection
(without writing a file), and process current-directory restoration against Sun.
Modern Save's nested filename editor requires a different test-driver lookup.
Release and Debug FileDialog115/ScaledDelivery115 each pass 2/2 after full matching
builds (out/phase4-file-{release,debug}-build.log). Graphics final integration
includes ScaledDelivery115; independent Task4 review remains pending.

Input slice: recovered key/mouse construction10008D49/10008ED2, native event
handlers10007441..1000806E and synthetic/saved-message replay1000920A/10009352/
10009956. Native messages are staged, delivered as Java events, then replayed
only if not consumed. Opaque tokens replace raw saved MSG pointers; disposal
reclaims pending records, making late event returns harmless. Coordinates,
button modifiers, click counts, Unicode/ACP and typed synthetic replay covered.
Input115 initially failed with mouse delivery0:0:0 while Sun passed. InputRetire115
covers20 dispose/GC callbacks. Oracle getText is racy during native edits; test
now awaits TextEvent before reading settled text. Debug AWT34/34 plus both final
input tests2/2; Release AWT35/35 (43.66s), out/phase4-input-*-{build,tests}.log.
Independent input review pending; wider focus/IME/demo gate remains open.
Task4 scoped review accepted after b78706d restored retained GIF notice and
425390b preserved borrowed-HDC copy clipping; targeted3/3 Debug/Release pass.

Menu Task3 slice dd12c7a independently accepted:15exports, Unicode/native menu
mutations/actions, GC/disposal and Canvas popup, checked bitmap249ink exactSun.
Shared Component/Toolkit/CMake hooks integrated with drawing-surface checkpoint.
Input findings addressed and re-review accepted: orphaned motion records were
confirmed by blocked-listener40k-event RED retaining2,559,872 nativeheapbytes.
Motion carries inlinebuttonflags and replays finalJavaeventcoords (intentional
translatePoint replay deviation); no native allocation survives Java coalescing.
Exit maps GetMessagePos into client coordinates (100230A8). Debug/Release4/4
focused fixes pass. Non-ACP U+6F22 becomes? on bothSun/ours; e9 fixture is ACP
coverage, not a claim of arbitrary UTF16/IME input fidelity.

DrawingSurface8exports reconstructed from1000BC50..1000BF55, DC mutex/refcounts
10003290/10003300 and componentDC10005D82. Sun baseline state1/2,image0,depth24,
offscreenbitsNULL; ours initiallymissinggetHDC. NativeDCaccess, state/bounds,
lock exclusion, GC and postflush resource lifetime now pass Sun/reconstruction.
Reentrant locked-thread drawing with another waiter passedSun but timedoutours;
Toolkit carries callerthread through synchronousrequests, surface waits service
sent requests only, and lock-owner drawing safely reenters. Surface pins and
postwaitGraphicsidentity checks protect disposal during this dispatch. A waiting
retiredGraphics staysinert; retained lockedimage survivesflush. Nested lock on
sameinfo is rejected (Sun can lose its handle after one unlock); wrong-thread
unlock is rejected instead of corrupting a native criticalsection.
Debug fullAWT40/40 before reentry addition; scoped finalsurface2/2 Debug1.73s,
Release1.06s. Reentrant Release fullAWT40/40 passed51.84s. Logs:
out/phase4-surface-{integrated-debug,reentrant-release}-tests.log and
out/phase4-surface-{reentrant-debug,retire-release}-build.log.
Background launches intermittently prevented Sun WindowState115 activation;
fixture now posts activationnotifications explicitly, retaining realnative
iconify/restore operations. Actual desktop focus remains in finaldemo gate.
Independent surface review remains pending; Phase4 is not complete.

Remaining interface checkpoint:
- Printing, embedded frames and frame icons (a6c9599) passed Debug/Release
  three-test Sun comparisons and independent review. Print jobs use an owned
  fixture DC with process-local API interception; real spooler/driver output
  is not claimed. Reentrant end/disposal while printing a native control has
  a regression test and retained job/DC lifetime.
- Clipboard Unicode, CRLF/NUL handling, native ownership callbacks and detached
  Java-owner GC rooting pass Debug and Release. The fixture intercepts only
  the child winawt imports; it never changes the desktop clipboard. Rooting
  uses the toolkit event-loop stack, which the legacy collector scans.
  Independent review accepted the implementation. Two distinct WClipboard
  instances replacing one another share the native owner HWND: Sun and ours
  both omit notification to the displaced first instance. This is retained
  legacy behavior, confirmed in out/clipboard-review.
- Drawing-surface review identified unsynchronized lazy palette lookup. The
  getter now dispatches to the toolkit owner; scoped re-review accepted it.
  DrawingSurface115, SurfaceLocks115 and Clipboard115 passed 3/3 in each
  configuration after the correction.
- Exact 166-export audits pass for Debug and Release. The completed native
  DLL is now included in the install image; installed-image and demo gates
  still must pass before Phase 4 is complete.
- Lifecycle115 repeated 30 times in Release with no failure (49.88 seconds,
  out/phase4-lifecycle-repeat.log). The earlier sporadic exit timeout was not
  reproduced; this is stress evidence, not a claimed shutdown defect fix.

Integrated verification at 28887a3:
- Windows Release 160/160 passed. Linux Debug and Release each passed 108
  tests with one expected unavailable-audio-device skip. Logs are
  out/phase4-final-{win-release,linux-debug,linux-release}-tests.log.
- Fresh installed Windows Debug and Release images passed all eleven AWT
  smoke/behavior cases and the existing Phase 2 installed-image suite. Only
  test bridges are staged separately; runtime libraries resolve from the
  installed image. Logs are out/phase4-image-{debug,release}-{awt,base}.log.
- Full Windows Debug reached 159/160: WindowState115 timed out without
  output. Subsequent 100 normal and 100 hidden direct launches all passed.
  Final CTest repeat/integration confirmation remains pending; no unsupported
  shutdown fix is claimed from these repeated successes.
- Whole-branch review prompted Focus115: both Sun and reconstruction report
  temporary focus loss between sibling controls, including sibling Panels.
  The fixture preserves this observed legacy behavior rather than asserting
  modern focus semantics.
- Menu command IDs are deliberately not recycled, preventing queued raw
  WM_COMMAND messages from resolving to a replacement item. The resulting
  65,535 lifetime item-creation cap is a disclosed limitation. Sun's original
  monotonically increasing IDs can alias after the 16-bit command boundary;
  exhaustion is rejected rather than delivering a stale command. Recycling
  with queue retirement is deferred, not advertised as implemented.

Focus restoration deadlock resolved:
- The CTest watchdog reproduced WindowState115's timeout and captured native
  stacks in out/phase4-windowstate-ctest-28008-stacks.log. The main thread
  held a synchronized WComponentPeer.hide monitor while awaiting toolkit
  Invoke; WM_APP+8 on the toolkit called Java default focus, whose synchronized
  native requestFocus needed that same peer monitor.
- FocusLock115 forces this ordering with an empty frame and a held peer
  monitor. It timed out before the fix and now completes in both configs.
  Native default-focus selection follows WWindowPeer child order, recursive
  traversal and first-child fallback, then calls SetFocus without entering
  the Java peer monitor. It retains lightweight dispatcher focus requests
  and queued-key retargeting. Peer class checks precede pData access.
- DefaultFocus115 compares nested/hidden/disabled/heavyweight/lightweight
  selection with Sun, then repeats with public event-queue access denied.
  The internal protected queue path preserves applet permissions; the public
  accessor was rejected after the demo pilot exposed SecurityException.
- Deliberate safety deviation: automatic default-focus restoration reads
  visible/enabled/container fields and performs the native focus request
  directly, rather than invoking user overrides of those accessors or
  requestFocus. The virtual isFocusTraversable eligibility hook remains.
  Existing-focus restoration continues using the recovered Java peer lookup.
- WindowState115, Focus115, DefaultFocus115 and FocusLock115 passed 4/4 in
  Debug (4.19s) and Release (3.60s). Full final suites remain pending.

Demo pilot found missing appletviewer.properties and content-types.properties
in the build image. CMake now copies/installs appletviewer, AWT and MIME
resources plus all four shipped font-locale variants. The installed-image
resource gate first failed on the missing appletviewer.properties; it checks
all eight files against the unchanged vendor SHA256 values. The all-demo run
uses the unchanged AppletViewer init/parse APIs through a tiny test driver,
so it does not interact with the separate first-run copyright dialog or
write acceptance settings. Native demo fixtures reject that unexpected
startup dialog before sending input.
