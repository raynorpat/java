/* DR2 ObjCJava raw lookup/invocation, 0x53d0c688 and 0x53d0c780. */
#include "invocation.h"
#include "tree.h"
#include <string.h>

struct methodblock *_JAVAMethodLookup(ExecEnv *env, ClassClass *clazz,
    const char *name, const char *signature, bool_t searchSuper, bool_t isStatic)
{
    unsigned i, count;
    struct methodblock *method;
    if (!clazz || !name) return NULL;
    if (searchSuper) {
        count = cbMethodTableSize(clazz);
        for (i = 0; i < count; ++i) {
            method = cbMethodTable(clazz)->methods[i];
            if (method && !strcmp(fieldname(&method->fb), name) &&
                (!signature || !strcmp(fieldsig(&method->fb), signature)))
                return method;
        }
    } else {
        count = cbMethodsCount(clazz);
        for (i = 0; i < count; ++i) {
            method = &cbMethods(clazz)[i];
            if (((method->fb.access & ACC_STATIC) != 0) == isStatic &&
                !strcmp(fieldname(&method->fb), name) &&
                (!signature || !strcmp(fieldsig(&method->fb), signature)))
                return method;
        }
    }
    return NULL;
}

long __JAVAMethodInvokeNoExceptionConversion(ExecEnv *env, HObject *target,
    struct methodblock *method, bool_t isStatic, bool_t shortFloats,
    va_list args, long *highBits, HObject **exception)
{
    long result = do_execute_java_method_vararg(env, target, NULL, NULL,
        method, isStatic, args, highBits, shortFloats);
    if (!env) env = EE();
    *exception = exceptionOccurred(env) ? env->exception.exc : NULL;
    exceptionClear(env);
    return result;
}
