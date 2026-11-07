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
