# JPEG binary mapping and verification

Primary IDA inputs: vendor/jdk1.1.5/bin/jpeg.dll and jpeg_g.dll.
Secondary: Rhapsody libjpeg.A.dylib, whose symbols retain Sun callback names.

| Function | Windows Release | Windows Debug | Rhapsody |
|---|---|---|---|
| sun_jpeg_init_source | 10001000 | 10001000 | 5C302534 |
| sun_jpeg_fill_input_buffer | 10001020 | 1000103A | 5C302574 |
| sun_jpeg_fill_suspended_buffer | 100010D0 | 10001141 | 5C302684 |
| sun_jpeg_skip_input_data | 100011C0 | 100012B5 | 5C302804 |
| sun_jpeg_term_source | 100012D0 | 10001450 | 5C3029A0 |
| sun_awt_image_JPEGImageDecoder_readImage | 100012E0 | 1000145B | 5C3029A8 |
| error exit | 10001750 | 10001A15 | local |
| output message | 10001770 | 10001A3F | local |
| readImage classic stub | 1000E6E0 | 100141E0 | 5C316310 |

Exactly one Windows export exists:
Java_sun_awt_image_JPEGImageDecoder_readImage_stub. It passes receiver, stream
and buffer from slots 0,1,2 and returns the original stack pointer (void).
The six javai imports are EE, SignalError, ArrayAlloc,
execute_java_dynamic_method, jio_snprintf and jio_fprintf. No VM ABI additions
are needed.

## Retained source and binary changes

The repository's unused 1.0.2 runtime/jpegdecoder.c, revision 1.8 95/11/27,
is the starting source. Its comments, structures, function names, progressive
decoding loop and pixel conversion are retained under src/share/sun/awt/image.
Only binary-confirmed 1.1.5 changes and marked portability adjustments were
made:

- EOF synthesizes two bytes FF D9 and emits JWRN_JPEG_EOF, including during
  suspended fills and skipped markers. The old source emitted only D9.
- Skip reads through the Java input buffer with read([BII)I. It keeps the
  bytes read beyond the end of a marker. It no longer calls skip(J)J.
- Warnings format a 200-byte message and use jio_fprintf(stderr,"%s\n",msg).
- A false pixel callback or pending callback exception destroys the decoder
  immediately; it does not attempt jpeg_finish_decompress on incomplete data.
- Modern Windows boolean/FAR declarations are isolated from IJG's names;
  the array-body cast expresses the existing 32-bit representation on GCC.

The IJG core is release 6, 2-Aug-95. Unmodified source and README license
are in third_party/jpeg-6; PROVENANCE.md records archive origin and SHA-256.
CMake copies the original ANSI jconfig.doc into the build tree. jmemnobs
matches Sun's malloc/free-only backing implementation. No upstream binaries
or generated JPEG fixtures are committed.

## Oracle and Phase 2 boundary

tests/native/jpeg115-fixtures.c generates 17x9 RGB, grayscale and progressive
files using IJG6. JpegLibrary115 implements only the native decoder callback
protocol. The test-only jpeg115probe forwards the original exported stub
unchanged with that receiver; it does not implement, replace or load any AWT
peer. The same bridge runs on original Sun java.exe/jpeg.dll and on this VM.

The Sun oracle records dimensions, color/progressive flags, final pixel
checksums and exceptions. It includes three-byte reads, a 9000-byte APP2
marker with normal and short reads, truncated marker/data, empty and invalid
files, header/pixel cancellation, header/pixel/input exceptions and null
stream/buffer arguments. Warning stderr is compared exactly: three
"Premature end of JPEG file" messages. Progressive final RGB matches baseline.

Before implementation the reconstructed VM reports UnsatisfiedLinkError for
decode. After reconstruction Windows Release and Debug pass the oracle and
the exact one-symbol export check. Linux Release and Debug C89/m32 standalone
builds pass the same oracle, including stderr. The original IJG files were
hash-compared with the extracted archive and are unchanged.

Isolated validation lives under out/jpeg-agent-build (Windows) and
out/jpeg-agent-linux-{release,debug}. cmake/jpeg115.cmake registers
vm-JpegLibrary115 and jpeg-exports115 and installs the production library.
Image display in AWT windows and unchanged applet demos remain Phase 4/6
acceptance; these tests prove decoding and native callbacks only.
