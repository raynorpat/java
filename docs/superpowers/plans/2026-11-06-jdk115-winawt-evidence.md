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
