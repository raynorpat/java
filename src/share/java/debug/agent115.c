/* Reconstructed from JDK 1.1.5 agent.dll and agent_g.dll.
 * See tests/native/agent115-evidence.md for addresses and coverage. */
#include <stdlib.h>
#include "StubPreamble.h"
#include "sun_tools_debug_Agent.h"
#include "java_lang_Thread.h"

typedef struct AgentThread {
    Hjava_lang_Thread *thread;
    struct AgentThread *next;
} AgentThread;
static AgentThread *system_threads;

int agent115_registered_thread(Hjava_lang_Thread *thread)
{
    AgentThread *entry;
    for (entry = system_threads; entry; entry = entry->next)
        if (entry->thread == thread) return 1;
    return 0;
}

long sun_tools_debug_Agent_systemThread(Hsun_tools_debug_Agent *self,
    Hjava_lang_Thread *thread)
{
    (void)self;
    return unhand(thread)->daemon || agent115_registered_thread(thread);
}

void sun_tools_debug_Agent_addSystemThread(Hsun_tools_debug_Agent *self,
    Hjava_lang_Thread *thread)
{
    AgentThread *entry;
    (void)self;
    if (agent115_registered_thread(thread)) return;
    entry = (AgentThread *)malloc(sizeof(*entry));
    if (!entry) {
        SignalError(NULL, "java/lang/NullPointerException", "malloc failed");
        return;
    }
    entry->thread = thread;
    entry->next = system_threads;
    system_threads = entry;
}

void sun_tools_debug_Agent_removeSystemThread(Hsun_tools_debug_Agent *self,
    Hjava_lang_Thread *thread)
{
    AgentThread **link, *entry;
    (void)self;
    for (link = &system_threads; *link; link = &(*link)->next) {
        entry = *link;
        if (entry->thread == thread) {
            *link = entry->next;
            free(entry);
            return;
        }
    }
}

long sun_tools_debug_Agent_objectId(Hsun_tools_debug_Agent *self,
    Hjava_lang_Object *object)
{
    (void)self;
    return (long)object;
}

long sun_tools_debug_Agent_getSingleStep(Hsun_tools_debug_Agent *self,
    Hjava_lang_Thread *thread)
{
    (void)self;
    return unhand(thread)->single_step;
}
