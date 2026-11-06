/*
 * Win32 support for the garbage collector in runtime/gc.c: heap memory,
 * the gc_locked() hook, and class structure allocation.
 */
#include <windows.h>
#include <setjmp.h>

#include "sys_api.h"
#include "oobj.h"
#include "arena.h"

/************************************************************************/
/* Heap memory: reserve address space, then commit as the heap grows.   */

void *sysMapMem(long requestedSize, long *mappedSize)
{
    void *p = VirtualAlloc(NULL, (SIZE_T) requestedSize, MEM_RESERVE, PAGE_READWRITE);

    *mappedSize = p ? requestedSize : 0;
    return p;
}

void *sysUnmapMem(void *addr, long size, long *unmappedSize)
{
    *unmappedSize = VirtualFree(addr, 0, MEM_RELEASE) ? size : 0;
    return addr;
}

/* gc.c requires the caller's address back, not the page-rounded base. */
void *sysCommitMem(void *addr, long size, long *committedSize)
{
    if (VirtualAlloc(addr, (SIZE_T) size, MEM_COMMIT, PAGE_READWRITE) == NULL) {
	*committedSize = 0;
	return NULL;
    }
    *committedSize = size;
    return addr;
}

/* Only pages lying entirely inside the range are decommitted. */
void *sysUncommitMem(void *addr, long size, long *uncommittedSize)
{
    SYSTEM_INFO si;
    char *lo, *hi;

    GetSystemInfo(&si);
    lo = (char *) (((ULONG_PTR) addr + si.dwPageSize - 1) & ~(ULONG_PTR) (si.dwPageSize - 1));
    hi = (char *) (((ULONG_PTR) addr + size) & ~(ULONG_PTR) (si.dwPageSize - 1));
    if (hi > lo)
	VirtualFree(lo, (SIZE_T) (hi - lo), MEM_DECOMMIT);
    *uncommittedSize = size;
    return addr;
}

/************************************************************************/

extern unsigned char *gc0_locked(int async_call, unsigned int free_space_goal);

/*
 * Called by gc0() with the other threads stopped. The collecting thread's
 * own stack is scanned from inside gc0_locked(), so spill its callee-saved
 * registers into this frame first; registers of the stopped threads come
 * from sysThreadRegisters().
 */
unsigned char *gc_locked(int async_call, unsigned int free_space_goal)
{
    jmp_buf regs;
    unsigned char *volatile last_free;

    setjmp(regs);
    last_free = gc0_locked(async_call, free_space_goal);
    return last_free;
}

/************************************************************************/

/*
 * Class structures live outside the Java heap and are reached through the
 * class table; each class's other storage comes from its own arena.
 */
ClassClass *AllocClass(void)
{
    ClassClass *cb = (ClassClass *) sysCalloc(1, sizeof(ClassClass));
    ClassArena *arena;

    if (cb == NULL)
	return NULL;
    if ((arena = (ClassArena *) sysCalloc(1, sizeof(ClassArena))) == NULL) {
	sysFree(cb);
	return NULL;
    }
    ArenaInit(&arena->pool, 2 * 1024);
    cb->classStorageArena = arena;
    return cb;
}
