/* Reconstructed JDK 1.1.5 static resource stubs and ClassLoader native names. */
#include "StubPreamble.h"

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_ClassLoader_getSystemResourceAsStream0_stub(stack_item *p, struct execenv *ee)
{
    extern void *java_lang_ClassLoader_getSystemResourceAsStream0(void *, void *);
    p[0].p = java_lang_ClassLoader_getSystemResourceAsStream0(NULL, p[0].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_ClassLoader_getSystemResourceAsName0_stub(stack_item *p, struct execenv *ee)
{
    extern void *java_lang_ClassLoader_getSystemResourceAsName0(void *, void *);
    p[0].p = java_lang_ClassLoader_getSystemResourceAsName0(NULL, p[0].p);
    return p + 1;
}

/* Transitional mapping to the existing class parser/resolver. */
JAVA_PUBLIC_API(stack_item *)
Java_java_lang_ClassLoader_defineClass0_stub(stack_item *p, struct execenv *ee)
{
    extern void *java_lang_ClassLoader_defineClass0(void *, void *, void *, long, long);
    p[0].p = java_lang_ClassLoader_defineClass0(p[0].p, p[1].p, p[2].p, p[3].i, p[4].i);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_ClassLoader_resolveClass0_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_ClassLoader_resolveClass0(void *, void *);
    java_lang_ClassLoader_resolveClass0(p[0].p, p[1].p);
    return p;
}

JAVA_PUBLIC_API(stack_item *)
Java_java_lang_ClassLoader_findSystemClass0_stub(stack_item *p, struct execenv *ee)
{
    extern void *java_lang_ClassLoader_findSystemClass0(void *, void *);
    p[0].p = java_lang_ClassLoader_findSystemClass0(p[0].p, p[1].p);
    return p + 1;
}
