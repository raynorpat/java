/* Native debugger bridge reconstructed from Sun 1.1.5 breakpoints.h/binary. */
#ifndef BREAKPOINTS115_H
#define BREAKPOINTS115_H
#include "tree.h"
#include "jni.h"
#include "interpreter.h"
#include "javaThreads.h"
typedef void (*HandleSingleStepHook)(ExecEnv *, unsigned char *);
typedef void (*HandleBreakpointHook)(ExecEnv *, unsigned char *);
typedef void (*HandleExceptionHook)(ExecEnv *, unsigned char *, HObject *);
void JNICALL set_debugger_hooks(HandleSingleStepHook, HandleBreakpointHook, HandleExceptionHook);
extern int single_stepping;
void set_single_step_thread(TID, bool_t);
#ifdef DEBUG
bool_t set_breakpoint(ExecEnv *, unsigned char *);
bool_t clear_breakpoint(ExecEnv *, unsigned char *);
#else
void set_breakpoint(ExecEnv *, unsigned char *);
void clear_breakpoint(ExecEnv *, unsigned char *);
#endif
int get_opcode(ExecEnv *, unsigned char *);
int get_breakpoint_opcode(ExecEnv *, unsigned char *, bool_t);
bool_t set_breakpoint_opcode(ExecEnv *, unsigned char *, unsigned char);
void notify_debugger_of_exception(unsigned char *, ExecEnv *, HObject *);
void notify_debugger_of_single_step(unsigned char *, ExecEnv *);
#endif
