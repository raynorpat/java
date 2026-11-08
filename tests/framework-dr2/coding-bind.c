#include "oobj.h"
#include "interpreter.h"
void codingBind(void *method, void *code, void *invoker)
{
    struct methodblock *mb = method;
    mb->code = code;
    mb->invoker = invoker;
}
