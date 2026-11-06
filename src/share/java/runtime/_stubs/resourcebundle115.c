/* Reconstructed from JDK 1.1.5 javai_g.dll, stub VA 0x1006221e. */
#include "StubPreamble.h"

JAVA_PUBLIC_API(stack_item *)
Java_java_util_ResourceBundle_getClassContext_stub(stack_item *p, struct execenv *ee)
{
    extern void *java_util_ResourceBundle_getClassContext(void *);
    p[0].p = java_util_ResourceBundle_getClassContext(NULL);
    return p + 1;
}
