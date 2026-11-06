/* Reconstructed from JDK 1.1.5 javai_g.dll, stub VA 0x10061434. */
#include "StubPreamble.h"

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_Double_valueOf0_stub(stack_item *p, struct execenv *ee)
{
    Java8 value;
    extern double java_lang_Double_valueOf0(void *, void *);
    SET_DOUBLE(value, p, java_lang_Double_valueOf0(NULL, p[0].p));
    return p + 2;
}
