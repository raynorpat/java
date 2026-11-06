/* javai_g.dll10002980--10002E0A; caller holds debugger locks for mutation. */
#include <stdlib.h>
#include <string.h>
#include "breakpoints115.h"
#include "java_lang_Thread.h"
#include "opcodes.h"
#include "finalize.h"
#include "monitor_cache.h"

#ifdef DEBUG
typedef struct { unsigned char *pc; unsigned char opcode; } Breakpoint;
static Breakpoint *breakpoints;
static int breakpoint_count;
int single_stepping;
static HandleSingleStepHook step_hook;
static HandleBreakpointHook breakpoint_hook;
static HandleExceptionHook exception_hook;
#endif

/* Sun1002D2C0/1002D36C acquire the runtime's shared data locks. Mapping:
 * current heap lock also protects strings/JNI; verifier uses class lock.
 * There is no separate exit lock or Sun allocator critical section here. */
void lock_for_debugger(void)
{
    extern sys_mon_t *_heap_lock;
    extern void debugger_lock_code(void);
    lock_classes();
    sysMonitorEnter(_heap_lock);
    HASFINALQ_LOCK();
    FINALMEQ_LOCK();
    QUEUE_LOCK();
    CACHE_LOCK();
    REGISTRY_LOCK();
    debugger_lock_code();
}
void unlock_for_debugger(void)
{
    extern sys_mon_t *_heap_lock;
    extern void debugger_unlock_code(void);
    debugger_unlock_code();
    REGISTRY_UNLOCK();
    CACHE_UNLOCK();
    QUEUE_UNLOCK();
    FINALMEQ_UNLOCK();
    HASFINALQ_UNLOCK();
    sysMonitorExit(_heap_lock);
    unlock_classes();
}

#ifdef DEBUG
void JNICALL set_debugger_hooks(HandleSingleStepHook step,
    HandleBreakpointHook breakpoint, HandleExceptionHook exception)
{ step_hook = step; breakpoint_hook = breakpoint; exception_hook = exception; }

void notify_debugger_of_single_step(unsigned char *pc, ExecEnv *ee)
{ if (debugging && step_hook) step_hook(ee, pc); }
void notify_debugger_of_exception(unsigned char *pc, ExecEnv *ee, HObject *object)
{ if (debugging && exception_hook) exception_hook(ee, pc, object); }

void set_single_step_thread(TID thread, bool_t enabled)
{
    long old = unhand(thread)->single_step;
    unhand(thread)->single_step = enabled;
    if (old != enabled) {
        if (enabled) single_stepping++;
        else if (single_stepping > 0) single_stepping--;
    }
}

static Breakpoint *find_breakpoint(ExecEnv *ee, unsigned char *pc, int create)
{
    Breakpoint *free_slot = NULL, *table;
    int i, count;
    for (i = 0; i < breakpoint_count; i++) {
        if (breakpoints[i].pc == pc) return &breakpoints[i];
        if (!breakpoints[i].pc) free_slot = &breakpoints[i];
    }
    if (!create) return NULL;
    if (free_slot) return free_slot;
    count = breakpoint_count ? breakpoint_count * 2 : 1;
    table = (Breakpoint *)realloc(breakpoints, count * sizeof(*table));
    if (!table) { SignalError(ee, JAVAPKG "OutOfMemoryError", NULL); return NULL; }
    breakpoints = table;
    free_slot = table + breakpoint_count;
    memset(free_slot, 0, (count - breakpoint_count) * sizeof(*table));
    breakpoint_count = count;
    return free_slot;
}
bool_t set_breakpoint(ExecEnv *ee, unsigned char *pc)
{
    Breakpoint *entry = find_breakpoint(ee, pc, TRUE);
    if (!entry) return TRUE; /* Sun returns true with a pending OOM. */
    if (entry->pc) {
        SignalError(ee, JAVAPKG "IllegalArgumentException", "Duplicate breakpoint");
        return FALSE;
    }
    entry->pc = pc; entry->opcode = *pc; *pc = opc_breakpoint;
    /* Current collector scans all classes; Sun invalidates its scan cache. */
    return TRUE;
}
int get_opcode(ExecEnv *ee, unsigned char *pc)
{
    Breakpoint *entry = find_breakpoint(ee, pc, FALSE);
    return entry ? entry->opcode : *pc;
}
int get_breakpoint_opcode(ExecEnv *ee, unsigned char *pc, bool_t notify)
{
    Breakpoint *entry = find_breakpoint(ee, pc, FALSE);
    int opcode;
    if (entry) {
        opcode = entry->opcode;
        if (notify && debugging && breakpoint_hook) breakpoint_hook(ee, pc);
        return opcode;
    }
    if (*pc != opc_breakpoint) return *pc;
    SignalError(ee, JAVAPKG "InternalError", "Phantom breakpoint");
    return -1;
}
bool_t set_breakpoint_opcode(ExecEnv *ee, unsigned char *pc, unsigned char opcode)
{
    Breakpoint *entry = find_breakpoint(ee, pc, FALSE);
    if (!entry) { SignalError(ee, JAVAPKG "InternalError", "Phantom breakpoint"); return FALSE; }
    entry->opcode = opcode; return TRUE;
}
bool_t clear_breakpoint(ExecEnv *ee, unsigned char *pc)
{
    Breakpoint *entry = find_breakpoint(ee, pc, FALSE);
    if (!entry) {
        SignalError(ee, JAVAPKG "IllegalArgumentException", "Not a breakpoint");
        return FALSE;
    }
    *pc = entry->opcode; entry->pc = NULL; return TRUE;
}
#else
/* Release javai.dll10002320/330/340/350 deliberately omits breakpoint support. */
int get_opcode(ExecEnv *ee, unsigned char *pc) { return *pc; }
void set_breakpoint(ExecEnv *ee, unsigned char *pc) { }
void clear_breakpoint(ExecEnv *ee, unsigned char *pc) { }
void set_single_step_thread(TID thread, bool_t enabled) { }
#endif
