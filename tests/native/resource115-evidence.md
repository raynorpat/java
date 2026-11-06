# JDK 1.1.5 system resources

Primary evidence is `vendor/jdk1.1.5/bin/javai_g.dll`, SHA-256
`1BE736EAD48839D0FD7312C8162113F414CA67BE4327AC5895E5C0B87D652BFC`.
Implementation and native stack conventions were inspected with IDA.

| Address | Function / observation |
| --- | --- |
| `1003f090` | `resource_ValidateSystemResource(zip, entry, name)` verifies that the entry belongs to the classpath; ZIP uses zip stat, directory uses access then Java File.exists. |
| `1003f35a` | `resource_GetZipByteArray(entry, name)` rejects case-insensitive .class suffix with SecurityException, then finds matching ZIP classpath entry. |
| `1003f46d` | ZIP bytes: stat, malloc, zip_get, Java byte array copy, free. |
| `100148e6` | Array copy helper allocates T_BYTE (8) and copies the native bytes. |
| `1003f51b` | `resource_GetEntryFromKey(index)` returns classpath directory/ZIP filename at the zero-based index, or null. |
| `1003f5ea`, `1003f63e` | getSystemResourceAsStream0 converts the Java name into a 260-byte buffer, rejects .class case-insensitively, then scans classpath in order. Directory becomes FileInputStream; ZIP becomes ByteArrayInputStream. |
| `1003f873` | Constructs java/io/ByteArrayInputStream with signature `([B)`. |
| `1003f8c8`, `1003f91c` | getSystemResourceAsName0 scans in order and returns the systemresource URL string. |
| `10078c10`, `10078c2c` | Exact format strings: `systemresource:/FILE%d/+/%s` and `systemresource:/ZIP%d/+/%s`. |
| `10061649`, `10061674` | Static resource stubs pass null receiver and argument slot 0; result occupies slot 0. |
| `100615ac`, `100615f1`, `10061619` | defineClass0, resolveClass0, findSystemClass0 stack layouts match their earlier counterparts. |

The Java File/FileInputStream paths load SecurityException first and clear only
that exception type after Java calls, returning null/false. Other exceptions
remain pending. AsName does not reject .class; the stream and ZIP-byte helpers do.
Directories use the platform separator and native path conversion, while ZIP
names retain slash spelling. If a found directory resource fails Java checks,
the search returns instead of continuing to later classpath entries.

## Transitional mappings and bounded differences

The reconstruction uses existing cpe_t, sysGetClassPath, zip_stat and zip_get
instead of replacing the classpath/ZIP subsystems. The *0 ClassLoader stubs map
to existing defineClass/resolveClass/findSystemClass implementations; these still
use the older class parser/resolver and initialized check, so this is not a claim
that ClassLoader/class resolution has been fully reconstructed.

The six-character suffix test checks string length before examining the suffix;
the binary computes a pointer before the name buffer for shorter names. The
reconstruction avoids that undefined memory read. file_exists also checks a
pending constructor exception before issuing the virtual exists call.

Native source and new stub source compiled with MSVC x86 `/TC /W3` against the
current source headers. End-to-end resource gates are in `tests/vm/Resources.java`.

The integrated Release VM produced the complete expected Resources output
(system/custom properties, custom class definition and missing-resource null)
with both the resource directory and a temporary ZIP_STORED archive containing
the same payload. The checked-in DEFLATE archive initially failed because the
pre-existing zip_get explicitly rejected compressed members. The follow-up ZIP
slice adds the binary-identified original zlib 1.0.4 decoder without changing
the fixture; see zip115-evidence.md.
