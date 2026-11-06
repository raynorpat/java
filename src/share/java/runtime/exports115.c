/* Sun 1.1.5 runtime ABI helpers; see exports115-evidence.md. */
#include "tree.h"
#include "interpreter.h"
#ifdef XP_PC
#include <windows.h>
#endif

/* Transitional mapping: use this tree's ClassClass pointer and Sticky flag. */
void MakeClassSticky(ClassClass *cb)
{ cb->flags |= CCF_Sticky; }

ClassClass *FindStickySystemClass(ExecEnv *ee, char *name, long resolve)
{
    ClassClass *cb = FindClassFromClass(ee, name, resolve, NULL);
    if (cb) MakeClassSticky(cb);
    return cb;
}

#ifdef XP_PC
int sleep(unsigned int seconds)
{
    Sleep(seconds * 1000);
    return 0;
}
#endif
