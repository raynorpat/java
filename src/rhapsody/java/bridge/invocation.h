#include "oobj.h"
#include "interpreter.h"

struct methodblock *_JAVAMethodLookup(ExecEnv *env, ClassClass *clazz,
    const char *name, const char *signature, bool_t searchSuper, bool_t isStatic);
long __JAVAMethodInvokeNoExceptionConversion(ExecEnv *env, HObject *target,
    struct methodblock *method, bool_t isStatic, bool_t shortFloats,
    va_list args, long *highBits, HObject **exception);
