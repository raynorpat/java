# math reconstruction evidence

Inputs inspected in IDA: Sun math.dll, math_g.dll; Rhapsody libmath.A.dylib.
All twelve Windows exports are classic `Java_java_math_BigInteger_plumb*`
stubs. They import only ArrayAlloc, SignalError, FindClass, EE and
execute_java_constructor from javai. java.lang.Math is unrelated to this DLL.

| plumb operation | Release implementation | Release stub | Debug stub |
|---|---|---|---|
| Init | 10001000 | 10007B50 | 10009C60 |
| Add | 10001010 | 10007B60 | 10009C7D |
| Subtract | 100010B0 | 10007B90 | 10009CAF |
| Multiply | 10001190 | 10007BC0 | 10009CE1 |
| Divide | 10001250 | 10007BF0 | 10009D13 |
| Remainder | 10001330 | 10007C20 | 10009D45 |
| DivideAndRemainder | 100013F0 | 10007C50 | 10009D77 |
| Gcd | 10001530 | 10007C80 | 10009DA9 |
| ModPow | 100015F0 | 10007CB0 | 10009DDB |
| ModInverse | 100016F0 | 10007CE0 | 10009E14 |
| Square | 100017B0 | 10007D10 | 10009E46 |
| GeneratePrime | 10001840 | 10007D30 | 10009E71 |

Array conversion at 100018B0 calls bnInsertBigBytes with offset zero and
the Java array length. Result conversion at 10001910 computes (bnBits+7)/8,
allocates a T_BYTE array and calls bnExtractBigBytes. Helper 10001980 maps
only -1 to OutOfMemoryError. All wrappers initialize their BigNums, convert
inputs, call the named bn operation, convert outputs, and free every BigNum.
Subtraction obtains sign from bnSub's return and constructs BigInteger with
the private `([BI)` constructor. DivideAndRemainder creates T_CLASS length
two and installs FindClass("[B") as its component class. GeneratePrime calls
primeGen(number,NULL,NULL,NULL,0).

Rhapsody preserves the bnInit_32, bnEnd_32, bnAdd_32, lbnExpMod_32, primeGen,
sieveBuildBig and other names, with BigNum {pointer,size,allocated} (12 bytes).
Windows is the 16-bit version: 10001AF0 frees allocated*2 bytes; 10001C50
rounds byte count to 16-bit words; 100026C0 checks an odd modulus and invokes
the Montgomery exponentiation path, special-casing base two. Inverse
10002B40 has the source's exact pre-reduction branch and return behavior.
primeGen 100066F0 has the 2048-byte sieve and 256-entry candidate list.

These routines match Colin Plumb bnlib 1.1 (1995), including the original
Sun assertion filename src/share/java/math/bn-1.1/lbn16.c and line 2248.
Archive: https://archive.debian.org/debian/pool/main/b/bnlib/bnlib_1.1.orig.tar.gz
The original source is GPL-2.0-or-later, documented in legal.c. The user
explicitly approved vendoring it on 2026-11-06. The unmodified required
sources and license now reside in src/share/java/math/bn-1.1, following the
source path recovered from Sun's assertion strings.

The reconstructed Sun wrappers are implemented. An isolated build using
that staged dependency passes MathLibrary115 on Windows Release and Debug,
and Linux Release and Debug (-m32 -std=c89). Windows exports match exactly
twelve original names. The initial VM fails with UnsatisfiedLinkError for
math; the original Sun run supplies the committed expected output, including
the deterministic 128-bit prime. Test coverage includes all twelve natives,
large products, signed division/remainder, zero/equality, shifts, odd/even
moduli, negative powers, modular inverse, gcd, primes and Java exceptions.
An additional deterministic oracle covers limb boundaries at 15/16/17,
31/32/33, 63/64/65 and 127/128/129 bits, extending through 4096-bit inputs,
with quotient reconstruction and carry identities plus an output checksum.
It also covers numerator smaller than divisor and zero gcd inputs. All four
platform/configuration combinations pass the expanded case.

Original limitation: b=98765432109876543210987654321 raised to 123 modulo
100000000000000000000 triggers Sun's `nlen <= mlen` lbn16.c:2248 assertion.
The ordinary even-modulus case uses base 12345. No silent fix or replacement
arithmetic algorithm has been introduced. cmake/math115.cmake builds the
original 16-bit limb variant on Windows and Linux and registers the oracle
and export checks. NDEBUG is explicitly undefined for this target because
Sun retains bnlib assertions in Release as well as Debug. No new VM exports
are required.
