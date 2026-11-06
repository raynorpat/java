# sysresource evidence

IDA primary inputs: vendor JDK 1.1.5 bin/sysresource.dll and
sysresource_g.dll. Release has three exports and four javai imports.
Debug has the same API and behavior.

| Operation | Release body / stub | Debug body / stub |
|---|---|---|
| getEntryFromKey | 10001000 / 100010D0 | 10001000 / 100010E0 |
| getZipResourceByteArray | 10001010 / 100010F0 | 1000101C / 1000110B |
| validateSystemResource | 10001070 / 10001120 | 10001078 / 1000113D |

Every export has the prefix
`Java_sun_net_www_protocol_systemresource_SystemResourceManager_` and suffix
`_stub`. Static native arguments start at stack slot zero. Each stub writes
its return to slot zero and returns the input pointer plus one. Validation
normalizes the VM helper's return to boolean. The two string-taking functions
convert entry and resource names using javaString2CString, each into a
260-byte automatic array, then invoke the corresponding resource_* helper.
Rhapsody libsysresource.A.dylib bodies at 5C901D2C, 5C901D4C, 5C901DB0
confirm these names and calls; Unix uses 1024-byte buffers, recovered directly
from its stack allocation and conversion limits. This platform distinction is
retained behind XP_WIN32.
getEntryFromKey forwards its integer directly. There is no DLL-owned resource
handle, cache, allocation, finalizer or lifecycle state.

The existing VM resource115.c supplies all three helper exports. No VM ABI
change is required. Java URL code (unmodified classes.zip) owns URL parsing
and stream lifecycle. ResourceLibrary115 exercises ZIP and directory paths,
reopening a resource after close, missing resources, invalid keys and the
classfile security restriction. The original Sun Release run produces the
committed expected output; the initial reconstructed VM fails with
UnsatisfiedLinkError for sysresource, and the new library matches the oracle.
Release native export inspection confirms exactly three original names.
Release and Debug isolated builds both pass the expected-output script and
the three-symbol export check. The directory-backed run also matches.
Linux Release and Debug standalone C89/m32 shared-library builds pass the
same Java oracle for both directory and ZIP resources. Isolated outputs are
under out/math-agent-linux-{release,debug}; tests use the existing VM builds
under out/linux115 and out/linux115-debug without modifying either.

The library filename remains sysresource.dll in Debug as well as Release,
matching this port's existing Debug javai.dll naming. The configuration is
the debug variant; no change to runtime library-name resolution is needed.
