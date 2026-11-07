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
