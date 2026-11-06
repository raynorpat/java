/* Reconstructed legacy adapter for the JDK 1.1.5 native. */
#include "StubPreamble.h"
JAVA_PUBLIC_API(stack_item *)
Java_java_lang_String_intern_stub(stack_item *p, struct execenv *ee)
{
    extern void *java_lang_String_intern(void *);
    p[0].p = java_lang_String_intern(p[0].p);
    return p + 1;
}
