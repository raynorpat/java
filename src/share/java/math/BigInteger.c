/* Reconstructed Sun JDK 1.1.5 BigInteger native wrappers.
 * See docs/superpowers/plans/2026-11-06-jdk115-math-evidence.md.
 * Helper/local names were not retained by the Windows or Rhapsody binaries.
 */
#include "tree.h"
#include "interpreter.h"
#include "bn.h"
#include "prime.h"

static int arrayToBigNum(HArrayOfByte *array, struct BigNum *number)
{
    if (bnInsertBigBytes(number, (unsigned char *)unhand(array)->body,
                         0, obj_length(array)) != -1) return 1;
    SignalError(EE(), "java/lang/OutOfMemoryError", "Allocating C bignum");
    return 0;
}

static HArrayOfByte *bigNumToArray(struct BigNum *number)
{
    unsigned length;
    HArrayOfByte *result;
    length = (bnBits(number) + 7) / 8;
    result = (HArrayOfByte *)ArrayAlloc(T_BYTE, length);
    if (result != NULL)
        bnExtractBigBytes(number, (unsigned char *)unhand(result)->body, 0, length);
    else
        SignalError(EE(), "java/lang/OutOfMemoryError", "Allocating Java byte array");
    return result;
}

static int checkResult(int result)
{
    if (result == -1)
        SignalError(EE(), "java/lang/OutOfMemoryError",
                    "Executing C bignum arithmetic operation");
    return result != -1;
}

static void java_math_BigInteger_plumbInit(void *unused)
{
    bnInit();
}

static HArrayOfByte *java_math_BigInteger_plumbAdd(void *unused,
                                                  HArrayOfByte *a, HArrayOfByte *b)
{
    struct BigNum x, y;
    HArrayOfByte *result = NULL;
    bnBegin(&x); bnBegin(&y);
    if (arrayToBigNum(a, &x) && arrayToBigNum(b, &y))
        if (checkResult(bnAdd(&x, &y))) result = bigNumToArray(&x);
    bnEnd(&x); bnEnd(&y);
    return result;
}

static HObject *java_math_BigInteger_plumbSubtract(void *unused,
                                                 HArrayOfByte *a, HArrayOfByte *b)
{
    struct BigNum x, y;
    HArrayOfByte *magnitude;
    HObject *result = NULL;
    int status, sign;
    bnBegin(&x); bnBegin(&y);
    if (arrayToBigNum(a, &x) && arrayToBigNum(b, &y)) {
        status = bnSub(&x, &y);
        if (checkResult(status)) {
            magnitude = bigNumToArray(&x);
            if (magnitude != NULL) {
                sign = status == 1 ? -1 : obj_length(magnitude) != 0;
                result = execute_java_constructor(EE(), "java/math/BigInteger",
                                                   NULL, "([BI)", magnitude, sign);
            }
        }
    }
    bnEnd(&x); bnEnd(&y);
    return result;
}

static HArrayOfByte *java_math_BigInteger_plumbMultiply(void *unused,
                                                       HArrayOfByte *a, HArrayOfByte *b)
{
    struct BigNum x, y, z;
    HArrayOfByte *result = NULL;
    bnBegin(&x); bnBegin(&y); bnBegin(&z);
    if (arrayToBigNum(a, &x) && arrayToBigNum(b, &y))
        if (checkResult(bnMul(&z, &x, &y))) result = bigNumToArray(&z);
    bnEnd(&x); bnEnd(&y); bnEnd(&z);
    return result;
}

static HArrayOfByte *java_math_BigInteger_plumbDivide(void *unused,
                                                     HArrayOfByte *a, HArrayOfByte *b)
{
    struct BigNum x, y, q, r;
    HArrayOfByte *result = NULL;
    bnBegin(&x); bnBegin(&y); bnBegin(&q); bnBegin(&r);
    if (arrayToBigNum(a, &x) && arrayToBigNum(b, &y))
        if (checkResult(bnDivMod(&q, &r, &x, &y))) result = bigNumToArray(&q);
    bnEnd(&x); bnEnd(&y); bnEnd(&q); bnEnd(&r);
    return result;
}

static HArrayOfByte *java_math_BigInteger_plumbRemainder(void *unused,
                                                        HArrayOfByte *a, HArrayOfByte *b)
{
    struct BigNum x, y, r;
    HArrayOfByte *result = NULL;
    bnBegin(&x); bnBegin(&y); bnBegin(&r);
    if (arrayToBigNum(a, &x) && arrayToBigNum(b, &y))
        if (checkResult(bnMod(&r, &x, &y))) result = bigNumToArray(&r);
    bnEnd(&x); bnEnd(&y); bnEnd(&r);
    return result;
}

static HArrayOfObject *java_math_BigInteger_plumbDivideAndRemainder(void *unused,
                                                                 HArrayOfByte *a, HArrayOfByte *b)
{
    struct BigNum x, y, q, r;
    HArrayOfObject *result = NULL;
    bnBegin(&x); bnBegin(&y); bnBegin(&q); bnBegin(&r);
    if (arrayToBigNum(a, &x) && arrayToBigNum(b, &y)) {
        if (checkResult(bnDivMod(&q, &r, &x, &y))) {
            result = (HArrayOfObject *)ArrayAlloc(T_CLASS, 2);
            if (result != NULL) {
                unhand(result)->body[0] = (HObject *)bigNumToArray(&q);
                unhand(result)->body[1] = (HObject *)bigNumToArray(&r);
                unhand(result)->body[2] = (HObject *)FindClass(EE(), "[B", TRUE);
            } else {
                SignalError(EE(), "java/lang/OutOfMemoryError", "Allocating Java byte array array");
            }
        }
    }
    bnEnd(&x); bnEnd(&y); bnEnd(&q); bnEnd(&r);
    return result;
}

static HArrayOfByte *java_math_BigInteger_plumbGcd(void *unused,
                                                 HArrayOfByte *a, HArrayOfByte *b)
{
    struct BigNum x, y, z;
    HArrayOfByte *result = NULL;
    bnBegin(&x); bnBegin(&y); bnBegin(&z);
    if (arrayToBigNum(a, &x) && arrayToBigNum(b, &y))
        if (checkResult(bnGcd(&z, &x, &y))) result = bigNumToArray(&z);
    bnEnd(&x); bnEnd(&y); bnEnd(&z);
    return result;
}

static HArrayOfByte *java_math_BigInteger_plumbModPow(void *unused,
                                                    HArrayOfByte *a, HArrayOfByte *b, HArrayOfByte *m)
{
    struct BigNum x, y, modulus, z;
    HArrayOfByte *result = NULL;
    bnBegin(&x); bnBegin(&y); bnBegin(&modulus); bnBegin(&z);
    if (arrayToBigNum(a, &x) && arrayToBigNum(b, &y) && arrayToBigNum(m, &modulus))
        if (checkResult(bnExpMod(&z, &x, &y, &modulus))) result = bigNumToArray(&z);
    bnEnd(&x); bnEnd(&y); bnEnd(&modulus); bnEnd(&z);
    return result;
}

static HArrayOfByte *java_math_BigInteger_plumbModInverse(void *unused,
                                                        HArrayOfByte *a, HArrayOfByte *m)
{
    struct BigNum x, modulus, z;
    HArrayOfByte *result = NULL;
    bnBegin(&x); bnBegin(&modulus); bnBegin(&z);
    if (arrayToBigNum(a, &x) && arrayToBigNum(m, &modulus))
        if (checkResult(bnInv(&z, &x, &modulus))) result = bigNumToArray(&z);
    bnEnd(&x); bnEnd(&modulus); bnEnd(&z);
    return result;
}

static HArrayOfByte *java_math_BigInteger_plumbSquare(void *unused, HArrayOfByte *a)
{
    struct BigNum x, z;
    HArrayOfByte *result = NULL;
    bnBegin(&x); bnBegin(&z);
    if (arrayToBigNum(a, &x))
        if (checkResult(bnSquare(&z, &x))) result = bigNumToArray(&z);
    bnEnd(&x); bnEnd(&z);
    return result;
}

static HArrayOfByte *java_math_BigInteger_plumbGeneratePrime(void *unused, HArrayOfByte *a)
{
    struct BigNum x;
    HArrayOfByte *result = NULL;
    bnBegin(&x);
    if (arrayToBigNum(a, &x))
        if (checkResult(primeGen(&x, NULL, NULL, NULL, 0))) result = bigNumToArray(&x);
    bnEnd(&x);
    return result;
}

#if defined(__GNUC__) && __GNUC__ >= 4
#define MATH_EXPORT __attribute__((visibility("default")))
#else
#define MATH_EXPORT
#endif
#define STUB(name) MATH_EXPORT stack_item *Java_java_math_BigInteger_##name##_stub
STUB(plumbInit)(stack_item *sp, ExecEnv *ee)
{
    java_math_BigInteger_plumbInit(NULL);
    return sp;
}
/* The classic javah ABI uses one slot per reference and returns sp + 1. */
#define BINARY_STUB(name) STUB(name)(stack_item *sp, ExecEnv *ee) { \
    sp[0].p = java_math_BigInteger_##name(NULL, (HArrayOfByte *)sp[0].p, \
                                        (HArrayOfByte *)sp[1].p); return sp + 1; }
BINARY_STUB(plumbAdd)
BINARY_STUB(plumbSubtract)
BINARY_STUB(plumbMultiply)
BINARY_STUB(plumbDivide)
BINARY_STUB(plumbRemainder)
BINARY_STUB(plumbDivideAndRemainder)
BINARY_STUB(plumbGcd)
BINARY_STUB(plumbModInverse)
STUB(plumbModPow)(stack_item *sp, ExecEnv *ee)
{
    sp[0].p = java_math_BigInteger_plumbModPow(NULL, (HArrayOfByte *)sp[0].p,
                                             (HArrayOfByte *)sp[1].p, (HArrayOfByte *)sp[2].p);
    return sp + 1;
}
STUB(plumbSquare)(stack_item *sp, ExecEnv *ee)
{
    sp[0].p = java_math_BigInteger_plumbSquare(NULL, (HArrayOfByte *)sp[0].p);
    return sp + 1;
}
STUB(plumbGeneratePrime)(stack_item *sp, ExecEnv *ee)
{
    sp[0].p = java_math_BigInteger_plumbGeneratePrime(NULL, (HArrayOfByte *)sp[0].p);
    return sp + 1;
}
