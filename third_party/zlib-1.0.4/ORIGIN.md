# zlib 1.0.4 codec

Downloaded unchanged from the authors' official archive:
https://zlib.net/fossils/zlib-1.0.4.tar.gz

Archive SHA-256:
`e5c260cd3db1370fb3e0c193e9cbd9f127a9bd055d622b3fb55b82747f6e5b24`

The 22 upstream files here are unchanged from the archive, apart from checkout
line-ending normalization. The inflate and
deflate implementations, CRC32, dependencies, headers, README and ChangeLog are
included. Phase 2 adds deflate.c, deflate.h, trees.c and crc32.c unchanged.
The original license is retained in zlib.h and the upstream copyright notices
are intact. ORIGIN.md is project documentation, not an upstream file.

This version is selected from binary evidence: Sun JDK 1.1.5 zip_g.dll contains
the zlib 1.0.4 inflate and deflate copyright strings and passes version 1.0.4 to
inflateInit2_. See tests/native/zip115-evidence.md for addresses and integration.
