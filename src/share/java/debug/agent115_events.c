/* Reconstructed from agent_g.dll 1000106D--10001474, 1000256C--10002630,
 * 10002DB3, 10003650--100036D6 and 10003849. */
#include <stdlib.h>
#include <string.h>
#include "StubPreamble.h"
#include "breakpoints115.h"
#include "threads_md.h"
#include "monitor.h"
#include "java_lang_Thread.h"
#include "sun_tools_debug_Agent.h"
#include "sun_tools_debug_BreakpointHandler.h"
#include "sun_tools_debug_BreakpointQueue.h"
#ifdef XP_UNIX
#include <unistd.h>
#endif
extern int agent115_registered_thread(Hjava_lang_Thread *);
extern int JDB_loadclass_locked(ExecEnv *);
extern void lock_for_debugger(void), unlock_for_debugger(void);
static Hsun_tools_debug_Agent *agent;
#ifdef DEBUG
static ClassClass *handler_class;
static Hsun_tools_debug_BreakpointQueue *queue;
static Hsun_tools_debug_BreakpointQueue *get_queue(void)
{
    ClassClass **classes; long *value; int i;
    if (!queue) {
        if (!handler_class) {
            classes=get_binclasses();
            for(i=0;i<get_nbinclasses();++i)
                if (!strcmp(cbName(classes[i]),"sun/tools/debug/BreakpointHandler") && !unhand(classes[i])->loader) {
                    handler_class=classes[i]; MakeClassSticky(handler_class); break;
                }
            if (!handler_class) return NULL;
        }
        value=getclassvariable(handler_class,"the_bkptQ");
        if (!value || !*value) return NULL;
        queue=(Hsun_tools_debug_BreakpointQueue *)*value;
    }
    return queue;
}
static void report_event(Hjava_lang_Thread *thread,unsigned char *pc,ExecEnv *ee,HObject *exception,unsigned char *catch_pc)
{
    if (JDB_loadclass_locked(ee)) return;
    monitorEnter((unsigned int)queue);
    while (!unhand(queue)->waiting_for_event) monitorWait((unsigned int)queue,-1);
    unhand(queue)->thread=thread; unhand(queue)->pc=(long)pc;
    unhand(queue)->exception=(void*)exception; unhand(queue)->catch_pc=(long)catch_pc;
    unhand(queue)->waiting_for_event=0;
    monitorNotifyAll((unsigned int)queue);
    monitorWait((unsigned int)queue,-1);
    monitorExit((unsigned int)queue);
}
static void handle_breakpoint(ExecEnv *ee,unsigned char *pc)
{
    Hjava_lang_Thread *thread=threadSelf();
    if (!agent115_registered_thread(thread) && get_queue()) report_event(thread,pc,ee,NULL,NULL);
}
static void handle_exception(ExecEnv *ee,unsigned char *pc,HObject *exception)
{
    Hjava_lang_Thread *thread=threadSelf(); JavaFrame *frame; unsigned char *catch_pc=NULL;
#ifdef JDK115_CTHREADS
    char saved_kind;
    JHandle *saved_exception;
#endif
    if (agent115_registered_thread(thread) || !get_queue()) return;
    for(frame=ee->current_frame;frame;frame=frame->prev)
        if(frame->current_method) {
            catch_pc=ProcedureFindThrowTag(ee,frame,exception,frame->lastpc);
            if(catch_pc) break;
        }
#ifdef JDK115_CTHREADS
    /* The bounded cthreads wait polls pending exceptions for Thread.stop.
     * The exception being reported must not wake its own debugger wait.
     * Keep it rooted on this stack and then in the event queue; preserve any
     * new asynchronous exception posted while the debugger has us stopped. */
    saved_kind=ee->exceptionKind; saved_exception=ee->exception.exc;
    exceptionClear(ee);
#endif
    report_event(thread,pc,ee,exception,catch_pc);
#ifdef JDK115_CTHREADS
    if (!exceptionOccurred(ee)) {
        ee->exception.exc=saved_exception; ee->exceptionKind=saved_kind;
    }
#endif
}
#endif
static void handle_exit(void)
{
    execute_java_dynamic_method(EE(),(HObject *)agent,"reportAppExit","()V");
#ifdef XP_PC
    Sleep(1000);
#else
    sleep(1);
#endif
}
void sun_tools_debug_Agent_initAgent(Hsun_tools_debug_Agent *self)
{
#ifndef DEBUG
    SignalError(NULL,"java/lang/IllegalAccessError","not a debugging VM");
#else
    agent=self;
    sysAtexit(handle_exit);
    set_debugger_hooks(handle_breakpoint,handle_breakpoint,handle_exception);
#endif
}
void sun_tools_debug_Agent_runMain(Hsun_tools_debug_Agent *self,ClassClass *clazz,HArrayOfString *args)
{ execute_java_static_method(NULL,clazz,"main","([Ljava/lang/String;)V",args); }
long sun_tools_debug_Agent_peek(Hsun_tools_debug_Agent *self,long pc)
{ return (unsigned char)get_opcode(EE(),(unsigned char *)pc); }
void sun_tools_debug_BreakpointHandler_setBreakpoint(Hsun_tools_debug_BreakpointHandler *self,long pc)
{ set_breakpoint(EE(),(unsigned char *)pc); }
void sun_tools_debug_BreakpointHandler_clrBreakpoint(Hsun_tools_debug_BreakpointHandler *self,long pc)
{ clear_breakpoint(EE(),(unsigned char *)pc); }
void sun_tools_debug_Agent_setSingleStep(Hsun_tools_debug_Agent *self,Hjava_lang_Thread *thread,long enabled)
{ set_single_step_thread(thread,enabled); }
void sun_tools_debug_Agent_suspendSpecificThread(Hsun_tools_debug_Agent *self,Hjava_lang_Thread *thread)
{ lock_for_debugger(); java_lang_Thread_suspend0(thread); unlock_for_debugger(); }
void sun_tools_debug_Agent_suspendListOfThreads(Hsun_tools_debug_Agent *self,long count,HArrayOfObject *threads)
{
    int i; Hjava_lang_Thread *thread;
    lock_for_debugger();
    for(i=0;i<count;++i) {
        thread=(void*)unhand(threads)->body[i];
        if(!agent115_registered_thread(thread)) java_lang_Thread_suspend0(thread);
    }
    unlock_for_debugger();
}
long sun_tools_debug_Agent_getThreadStatus(Hsun_tools_debug_Agent *self,Hjava_lang_Thread *thread)
{
    sys_thread_t *system; ExecEnv *ee; int state;
    if(!thread) return -1;
    system=SYSTHREAD(thread); if(!system) return 0;
#ifdef XP_PC
    state=system->state;
#elif defined(JDK115_CTHREADS)
    /* DR2 libagent 5BD0118C: Mach states are not the Win32 state numbers. */
    switch(system->state) {
    case RUNNABLE: case RUNNING: return 1;
    case SUSPENDED: case MONITOR_SUSPENDED: return 5;
    case MONITOR_WAIT: case CONDVAR_WAIT:
        if(system->mpending_suspend || system->cpending_suspend) return 5;
        if(system->mon_wait==(sys_mon_t *)system) return 2;
        return system->state==MONITOR_WAIT ? 3 : 4;
    case SUSPENDED_AT_BREAKPOINT: return 6;
    default: return -1;
    }
#else
    /* Platform adaptation: pthread backend represents suspension as stop bits. */
    state=system->want_stop ? 1 : (system->waiting_on ? 3 : 0);
#endif
    switch(state) {
    case 1:
        ee=(ExecEnv *)unhand(thread)->eetop;
        if(ee && ee->current_frame && ee->current_frame->lastpc && *ee->current_frame->lastpc==202) return 6;
        return 5;
    case 2: return 3;
    case 3: return 4;
    default: return 1;
    }
}
