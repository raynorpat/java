/* Adapt DR2's short-float constructor ABI to the common five-argument VM API. */
#include "constructor.h"
#include "invocation.h"
#include <stdlib.h>
#include <string.h>

extern HObject *execute_java_constructor_vararg(ExecEnv *, char *,
    ClassClass *, char *, va_list);

HObject *bridgeNewJavaObject(ExecEnv *env, ClassClass *clazz,
    const char *signature, bool_t shortFloats, va_list args)
{
    size_t length = strlen(signature);
    char *descriptor, *promoted, *out;
    const char *p, *input = (const char *)args;
    struct methodblock *method;
    HObject *result;
    if (!shortFloats || length > 254)
        return execute_java_constructor_vararg(env, NULL, clazz,
            (char *)signature, args);

    /* Resolve before reading arguments: a missing constructor must raise its
     * Java lookup error even when the supplied argument area is empty. */
    descriptor = malloc(length + 2);
    if (!descriptor) {
        SignalError(env, JAVAPKG "OutOfMemoryError", NULL);
        return NULL;
    }
    memcpy(descriptor, signature, length);
    strcpy(descriptor + length, "V");
    method = _JAVAMethodLookup(env, clazz, "<init>", descriptor, FALSE, FALSE);
    free(descriptor);
    if (!method)
        return execute_java_constructor_vararg(env, NULL, clazz,
            (char *)signature, args);

    /* One descriptor byte can require at most an eight-byte promoted value. */
    promoted = malloc(8 * (length + 1));
    if (!promoted) {
        SignalError(env, JAVAPKG "OutOfMemoryError", NULL);
        return NULL;
    }
    out = promoted;
    p = fieldsig(&method->fb) + 1;
    while (*p != ')') {
        char kind = *p++;
        unsigned size = 4;
        if (kind == '[') {
            while (*p == '[') ++p;
            if (*p == 'L') while (*p != ';') ++p;
            ++p;
            kind = 'L';
        } else if (kind == 'L') {
            while (*p++ != ';') {}
        }
        if (kind == 'F') {
            float value;
            double wide;
            memcpy(&value, input, 4);
            wide = value;
            memcpy(out, &wide, 8);
            input += 4;
            out += 8;
        } else {
            if (kind == 'J' || kind == 'D') size = 8;
            memcpy(out, input, size);
            input += size;
            out += size;
        }
    }
    result = execute_java_constructor_vararg(env, NULL, clazz,
        (char *)signature, (va_list)promoted);
    free(promoted);
    return result;
}
