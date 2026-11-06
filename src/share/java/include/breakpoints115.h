/* Original debugger declarations, plus the actual Release no-op exports. */
#ifndef BREAKPOINTS115_H
#define BREAKPOINTS115_H
#ifdef DEBUG
#ifndef BREAKPTS
#define BREAKPTS
#endif
#endif
#include "tree.h"
#include "breakpoints.h"
#ifndef DEBUG
/* deviation: Release exports exist although the vendor header hides them. */
void set_breakpoint(ExecEnv *, unsigned char *);
void clear_breakpoint(ExecEnv *, unsigned char *);
void set_single_step_thread(TID, bool_t);
#endif
#endif
