/* Sun profiler-control adapters10062EA3 through10062EFA. */
#include "StubPreamble.h"

stack_item *Java_sun_misc_VM_getState_stub(stack_item *p, struct execenv *ee)
{
    extern long sun_misc_VM_getState(void *);
    p[0].i = sun_misc_VM_getState(NULL); return p + 1;
}
stack_item *Java_sun_misc_VM_threadsSuspended_stub(stack_item *p, struct execenv *ee)
{
    extern long sun_misc_VM_threadsSuspended(void *);
    p[0].i = sun_misc_VM_threadsSuspended(NULL); return p + 1;
}
stack_item *Java_sun_misc_VM_unsuspendThreads_stub(stack_item *p, struct execenv *ee)
{
    extern void sun_misc_VM_unsuspendThreads(void *);
    sun_misc_VM_unsuspendThreads(NULL); return p;
}
stack_item *Java_sun_misc_VM_unsuspendSomeThreads_stub(stack_item *p, struct execenv *ee)
{
    extern void sun_misc_VM_unsuspendSomeThreads(void *);
    sun_misc_VM_unsuspendSomeThreads(NULL); return p;
}
JAVA_PUBLIC_API(stack_item*)
Java_sun_misc_VM_resumeJavaMonitor_stub(stack_item *p, struct execenv *ee)
{
    extern void sun_misc_VM_resumeJavaMonitor(void *);
    sun_misc_VM_resumeJavaMonitor(NULL); return p;
}
JAVA_PUBLIC_API(stack_item*)
Java_sun_misc_VM_suspendJavaMonitor_stub(stack_item *p, struct execenv *ee)
{
    extern void sun_misc_VM_suspendJavaMonitor(void *);
    sun_misc_VM_suspendJavaMonitor(NULL); return p;
}
JAVA_PUBLIC_API(stack_item*)
Java_sun_misc_VM_resetJavaMonitor_stub(stack_item *p, struct execenv *ee)
{
    extern void sun_misc_VM_resetJavaMonitor(void *);
    sun_misc_VM_resetJavaMonitor(NULL); return p;
}
JAVA_PUBLIC_API(stack_item*)
Java_sun_misc_VM_writeJavaMonitorReport_stub(stack_item *p, struct execenv *ee)
{
    extern void sun_misc_VM_writeJavaMonitorReport(void *);
    sun_misc_VM_writeJavaMonitorReport(NULL); return p;
}
