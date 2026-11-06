# JDK 1.1.5 string helpers and profiler filename

Primary evidence is vendor/jdk1.1.5/bin/javai_g.dll, SHA256
1BE736EAD48839D0FD7312C8162113F414CA67BE4327AC5895E5C0B87D652BFC.
IDA instance7c45480630fb was used explicitly without database renaming.

Recovered functions:

| Address | Contract identified from header and call graph |
| --- | --- |
|1004AEE0 | makeJavaStringFromPlatformCString |
|1004B04E | cache file.encoding, default8859_1 |
|1004B10C | makePlatformCString |
|1004B283 | javaStringUTFLength |
|1004B30D | javaString2UTF |
|1004B3B1 | makeJavaStringUTF |
|10039A31 | java_mon_dump, profiler filename fallback |
|10071878 | prof_file storage (IDA name Destination) |

The platform helpers select the shipped sun.io converter classes using the
cached file.encoding property. Their convert calls use the exact byte/character
array bounds recovered from disassembly. Converter lookup or conversion
exceptions are cleared and fall back to the original one-byte string helpers.
The byte-output allocation is count*MB_CUR_MAX+1, including the original CRT
locale-dependent capacity choice. No modern encoding library is substituted.
Modified-UTF helpers preserve slices, return -1 for invalid string handles in
UTFLength, and terminate bounded output without splitting a character.
makeJavaStringUTF uses the original char-array constructor path.

javaString.h is copied verbatim from the shipped include tree. The five missing
functions now have implementations in stringhelpers115.c. Existing legacy
string definitions remain in string.c and retain their explicit public exports.
No new private helper names are added to the public DLL export manifest.

profile.h adopts Sun's prof_file declaration; profiler.c reads it and defaults
NULL to java.prof at report time, matching10039A31. The original prof_md.h include is retained. Neither vendor tree contains that
file, but this repository already has a shared compatibility header defining
MARK as a no-op. It now explicitly documents that nonrecovered provenance.
An initial attempt to omit the include broke Release GC MARK calls; restoring
the existing compatibility header fixed that regression.
Existing profiling aggregation and heap report formatting remain transitional.
The native test checks filename selection and a nonempty report, not full
profiler-output equivalence.

The native fixture has two build modes. SUN_ORACLE dynamically loads the exact
primary DLL and calls these private RVAs; normal mode links javai_core. It checks
UTF null input, embedded NUL, multibyte characters, sliced strings, allocation,
bounded output, platform conversion/fallback, and custom profiler paths.
Three captured Sun runs (8859_1, Cp1252, and invalid encoding) exited0. The old
Cp1252 converter maps byte80 to U+FFFD and euro to '?'; modern mappings must not
be substituted for this evidence.

Oracle setup detail: Sun JNI initialization calls the platform helpers before
applying invocation properties, leaving its encoding cache set to8859_1. The
fixture explicitly resets the oracle-only cache at RVA7EE64 after VM creation
to exercise helper first-use selection under each configured property. It does
not patch function code or substitute expected output. Each encoding runs in a
separate process. Reconstruction startup does not yet call these helpers in
all of Sun's original places; the fixture is a helper contract test, not proof
of identical invocation startup ordering.

Decoder audit: decode.c is absent from all current CMake targets; javap launches
Sun's JavaP class, and javah defines its own unrelated PrintType. The native
PrintPublicOnly/PrintInternalSigs contract lacks a primary native implementation
in the supplied javai/javap binaries. decode.h is therefore left deferred rather
than reviving an unused C decoder merely to add declarations.

The new fixture initially reproduced a signed-byte widening defect in the old
str2unicode implementation: bytes80/e9 became FF80/FFE9. Primary1004E2D0
explicitly zero-extends bytes. A one-line unsigned-byte cast in util.c corrected
the mismatch; both Latin fallback tests then passed without oracle changes.

Targeted Windows Debug and Release string/profile+HPI gates each passed4/4.
Linux Release paired gates also passed: all three string/profile oracle cases and the HPI native test.
