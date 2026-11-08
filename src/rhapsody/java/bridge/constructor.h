#include "oobj.h"
#include "interpreter.h"

/* Raw constructor adapter: Java exceptions remain pending in env. */
HObject *bridgeNewJavaObject(ExecEnv *env, ClassClass *clazz,
    const char *signature, bool_t shortFloats, va_list args);
