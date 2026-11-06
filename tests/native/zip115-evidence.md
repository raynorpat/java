# ZIP DEFLATE reconstruction

Primary VM evidence: `vendor/jdk1.1.5/bin/javai_g.dll`, SHA-256
`1BE736EAD48839D0FD7312C8162113F414CA67BE4327AC5895E5C0B87D652BFC`.
Companion codec: `vendor/jdk1.1.5/bin/zip_g.dll`, SHA-256
`99AA190D83C12A6B3A676A63AE92938BA561245A5B8D55593BB3922190D6A110`.

IDA evidence in javai_g.dll:

- `1005125f` implements zip_get. The central-directory entry has filename at 0,
  uncompressed length at 4, compressed length at 8, method at 12, modification
  time at 16 and local-header offset at 20. The reconstructed direl_t follows
  these fields. Data-descriptor ZIP members require the central-directory sizes.
- The local header is checked for encryption, and payload bounds use compressed
  length. Method 0 reads directly, method 8 delegates to inflateFully, and other
  methods fail.
- `10051524` lazily loads the ZIP library and resolves `inflateFully`.

IDA evidence in zip_g.dll:

- `1000f9b8`: `deflate 1.0.4 Copyright 1995-1996 Jean-loup Gailly`.
- `1000fe00`: `inflate 1.0.4 Copyright 1995-1996 Mark Adler`.
- `10001e5b`: inflateFully passes -15, version `1.0.4`, and stream size 56 to
  inflateInit2_ at `10008b9b`, reads at most 512 input bytes per chunk, calls
  inflate with Z_PARTIAL_FLUSH (1), and compares total input/output against both
  directory lengths when it receives Z_STREAM_END.

The decoder is original published zlib 1.0.4 source from the authors' official
[fossil archive](https://zlib.net/fossils/zlib-1.0.4.tar.gz), not a new codec and
not claimed to be extracted Sun source. See its ORIGIN.md for the archive hash.

## Explicit integration differences

The reader links the inflate subset directly, including standalone tools,
instead of loading zip.dll. Each call uses an independent z_stream and frees
it afterward instead of sharing the binary's cached stream/reset operation.
Malformed input returns false on every non-Z_STREAM_END terminal result;
the primary's inferred pseudocode leaves its success variable set on some
inflater errors. The primary success path and exact input/output checks are
preserved. No CRC check is added to the ZIP reader in this slice.

Build the eight upstream C files adler32, infblock, infcodes, inffast, inflate,
inftrees, infutil and zutil; expose their directory for zlib.h and link all
targets containing zip.c. Undefine DEBUG only for the codec target because the
original DEBUG Trace macro emits unconditional diagnostic output.

## Verification

MSVC x86 native zip115-test passed against the unchanged checked-in DEFLATE
resource fixture. It checks decompressed properties, a wrong output length and
a missing member. The resource fixture's entries use method 8, including the
jar-generated data-descriptor form; the old reader rejected that method.
A temporary copy with the compressed properties block corrupted was also
rejected by zip_get (native harness exit 4), without hanging.
The same native test passed on Linux GCC -m32. Integrated Windows Release
vm-Resources passed with the original compressed fixture after the ZIP update.
