/* Isolated registry probe wrappers; production uses the full generated ABI. */
#include "StubPreamble.h"
#include "sun_tools_debug_Agent.h"

JAVA_PUBLIC_API(stack_item *)
Java_sun_tools_debug_Agent_systemThread_stub(stack_item *p, struct execenv *ee)
{
    (void)ee;
    p[0].i = sun_tools_debug_Agent_systemThread(NULL, p[0].p) ? 1 : 0;
    return p + 1;
}
JAVA_PUBLIC_API(stack_item *)
Java_sun_tools_debug_Agent_addSystemThread_stub(stack_item *p, struct execenv *ee)
{
    (void)ee;
    sun_tools_debug_Agent_addSystemThread(NULL, p[0].p);
    return p;
}
JAVA_PUBLIC_API(stack_item *)
Java_sun_tools_debug_Agent_removeSystemThread_stub(stack_item *p, struct execenv *ee)
{
    (void)ee;
    sun_tools_debug_Agent_removeSystemThread(NULL, p[0].p);
    return p;
}
JAVA_PUBLIC_API(stack_item *)
Java_sun_tools_debug_Agent_objectId_stub(stack_item *p, struct execenv *ee)
{
    (void)ee;
    p[0].i = sun_tools_debug_Agent_objectId(p[0].p, p[1].p);
    return p + 1;
}
JAVA_PUBLIC_API(stack_item *)
Java_sun_tools_debug_Agent_getSingleStep_stub(stack_item *p, struct execenv *ee)
{
    (void)ee;
    p[0].i = sun_tools_debug_Agent_getSingleStep(p[0].p, p[1].p) ? 1 : 0;
    return p + 1;
}
