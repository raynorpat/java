#include "oobj.h"
#include "interpreter.h"

/* Both VM donors and this tree use the recovered DR2 ExecEnv exception ABI. */
int checkExceptionState(ExecEnv *env, HObject *expected)
{
    int matches = exceptionOccurred(env) && env->exception.exc == expected;
    exceptionClear(env);
    return matches;
}
