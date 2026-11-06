/* Reconstructed JDK 1.1.5 stack adapters for boot natives. */
#include "StubPreamble.h"

JAVA_PUBLIC_API(stack_item*)
Java_java_lang_Thread_interrupt0_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_Thread_interrupt0(void *);
    java_lang_Thread_interrupt0(p[0].p);
    return p;
}

JAVA_PUBLIC_API(stack_item*)
Java_java_lang_Thread_isInterrupted_stub(stack_item *p, struct execenv *ee)
{
    extern long java_lang_Thread_isInterrupted(void *, long);
    p[0].i = java_lang_Thread_isInterrupted(p[0].p, p[1].i) != 0;
    return p + 1;
}

/* javai_g.dll 0x10038b46: handle address divided by eight, including NULL. */
JAVA_PUBLIC_API(stack_item*)
Java_java_lang_System_identityHashCode_stub(stack_item *p, struct execenv *ee)
{
    p[0].i = ((unsigned long)p[0].p) >> 3;
    return p + 1;
}

JAVA_PUBLIC_API(stack_item*)
Java_java_lang_Class_getPrimitiveClass_stub(stack_item *p, struct execenv *ee)
{
    extern void *java_lang_Class_getPrimitiveClass(void *, void *);
    p[0].p = java_lang_Class_getPrimitiveClass(NULL, p[0].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item*)
Java_java_lang_Class_isPrimitive_stub(stack_item *p, struct execenv *ee)
{
    extern long java_lang_Class_isPrimitive(void *);
    p[0].i = java_lang_Class_isPrimitive(p[0].p);
    return p + 1;
}

JAVA_PUBLIC_API(stack_item*)
Java_java_lang_System_setIn0_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_System_setIn0(void *, void *);
    java_lang_System_setIn0(NULL, p[0].p);
    return p;
}

JAVA_PUBLIC_API(stack_item*)
Java_java_lang_System_setOut0_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_System_setOut0(void *, void *);
    java_lang_System_setOut0(NULL, p[0].p);
    return p;
}

JAVA_PUBLIC_API(stack_item*)
Java_java_lang_System_setErr0_stub(stack_item *p, struct execenv *ee)
{
    extern void java_lang_System_setErr0(void *, void *);
    java_lang_System_setErr0(NULL, p[0].p);
    return p;
}
