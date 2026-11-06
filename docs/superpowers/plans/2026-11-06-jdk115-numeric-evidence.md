# Numeric parsing and converter discovery

Primary evidence: `vendor/jdk1.1.5/bin/javai_g.dll`, SHA-256
`1BE736EAD48839D0FD7312C8162113F414CA67BE4327AC5895E5C0B87D652BFC`.

The exported `Java_java_lang_Double_valueOf0_stub` at `0x10061434` takes one
String stack slot, calls the native at `0x10032f10`, writes a double across two
slots, and returns stack + 2. Rhapsody DR2 identifies the implementation as
`_java_lang_Double_valueOf0` at `0x5c128da8`. Sun's shipped Float bytecode calls
Double.valueOf0 too; no new Float parsing native is needed.

The native rejects null with NullPointerException. It obtains the modified-UTF
length, uses a 32-byte local buffer for lengths below 32, allocates length + 1
otherwise, and throws OutOfMemoryError on allocation failure. It converts the
String to modified UTF and invokes parser `0x10033062`. If parsing returns the
original pointer as its end pointer, it restores the original text (the parser
mutates decimal points and type suffixes), throws NumberFormatException with
that text, and returns zero. For ERANGE with a zero result it retries using
atof. Heap storage is freed in either case. Existing unicode2utfstrlen and
unicode2utf match the recovered conversion loops at `0x1004dfc4` and
`0x1004de30`, including modified-UTF encoding of NUL.

The parser is a finite-state machine accepting decimal digits, optional sign,
decimal point, exponent, and optional d/D/f/F suffix. It substitutes the host
locale decimal character for the Java dot and replaces a suffix with a space
before calling strtod. The whitespace helper at `0x100335b1` accepts only tab,
line feed, form feed, carriage return, and space; vertical tab is rejected.
Suffix helper `0x10033629` accepts exactly d/D/f/F. The states and final-state
acceptance follow the binary, including delegation of a bare dot's rejection to
strtod. NaN, Infinity, hexadecimal notation, embedded NUL, and trailing junk are
rejected. No speculative modern Java parsing behavior was added.

`Numeric115` was compiled with Sun javac and its expected output captured from
Sun's VM. The pre-implementation runner failed with UnsatisfiedLinkError. Cases
cover the local/heap buffer paths, whitespace, suffixes, signed zero, exponent,
overflow/underflow, null, invalid strings, modified-UTF NUL/non-ASCII input,
and Float's shared native path.

After integration, Windows Release `Numeric115` matches the Sun oracle exactly.
The existing Maths fixture also passes against Sun's 1.1.5 class library.

## Converter failure

Sun javap shows `sun.io.CharToByteConverter.<clinit>` reads
`System.getProperty("file.encoding.pkg")` without a fallback and caches it in
`pkgString`. getConverter constructs `pkgString + ".CharToByte" + encoding`,
then maps ClassNotFoundException to UnsupportedEncodingException. Therefore
missing startup property `file.encoding.pkg=sun.io` explains failure even when
direct loading of sun.io.CharToByteUTF8 works. Setting the property after
bootstrap is too late once the converter class has cached the missing value.
The property fix belongs to System property initialization, not to a converter
native or a changed Unicode fixture.

With that startup property added, Windows Release `Unicode` matches its Sun
oracle without fixture changes, including UTF8, UnicodeBig, stream redirection,
and the non-ASCII filename roundtrip.

## Command-line property regression

Testing the converter property through `-D` exposed an existing launcher bug:
`add_user_prop` appended key/value pairs to malloc storage without terminating
the pointer array, while System property initialization scanned for a null
pointer. The fix reserves one additional slot and writes the terminator after
each pair. This is a memory-safety repair, not reconstructed Sun behavior.
`CommandLineProps115` exercises nine pairs, crossing the original 16-pointer
capacity, with an empty final value. Its Sun-captured oracle matches the repaired
Windows Release launcher.
