# Colin Plumb bnlib 1.1

Unmodified original Colin Plumb multiprecision library, Copyright 1995.
The required 16-bit implementation, headers and license were extracted from:
https://archive.debian.org/debian/pool/main/b/bnlib/bnlib_1.1.orig.tar.gz

Archive SHA-256:
`dd159000628ddf52292becb662f2b7b326458036f66ebb29698cade989585e95`

License: GPL version 2 or later; see legal.c and COPYING. The repository owner
explicitly authorized vendoring this original dependency on 2026-11-06.

Sun's Windows math.dll contains the 16-bit limb implementation (assertion
paths name src/share/java/math/bn-1.1/lbn16.c). Rhapsody's libmath uses the
generated 32-bit variant. Both use the same BigNum interface. This build
retains the Windows implementation on both Windows and Linux. The archive's
unused architecture assembly, alternative word sizes, tools and documents
were not imported. Imported upstream files are unchanged.

See docs/superpowers/plans/2026-11-06-jdk115-math-evidence.md for the binary
mapping and comparison. Sun's reconstructed Java wrappers are kept outside
this directory in BigInteger.c.
