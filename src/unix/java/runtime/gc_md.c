/*
 * Unix support for the garbage collector in runtime/gc.c: heap memory,
 * the gc_locked() hook, and class structure allocation.
 */
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <setjmp.h>

#include "sys_api.h"
#include "oobj.h"

#if !defined(MAP_ANON) && defined(MAP_ANONYMOUS)
#define MAP_ANON MAP_ANONYMOUS
#endif

/************************************************************************/
/* Heap memory: reserve address space, then commit as the heap grows.   */

void *sysMapMem(long requestedSize, long *mappedSize)
{
    void *p = mmap(NULL, (size_t) requestedSize, PROT_NONE,
		   MAP_PRIVATE | MAP_ANON, -1, 0);

    if (p == MAP_FAILED) {
	*mappedSize = 0;
	return NULL;
    }
    *mappedSize = requestedSize;
    return p;
}

void *sysUnmapMem(void *addr, long size, long *unmappedSize)
{
    *unmappedSize = munmap(addr, (size_t) size) == 0 ? size : 0;
    return addr;
}

static unsigned long page_mask(void)
{
    return (unsigned long) sysconf(_SC_PAGESIZE) - 1;
}

/* gc.c requires the caller's address back, not the page-rounded base. */
void *sysCommitMem(void *addr, long size, long *committedSize)
{
    unsigned long mask = page_mask();
    char *lo = (char *) ((unsigned long) addr & ~mask);
    char *hi = (char *) (((unsigned long) addr + size + mask) & ~mask);

    if (mprotect(lo, (size_t) (hi - lo), PROT_READ | PROT_WRITE) != 0) {
	*committedSize = 0;
	return NULL;
    }
    *committedSize = size;
    return addr;
}

/* Only pages lying entirely inside the range are given back. */
void *sysUncommitMem(void *addr, long size, long *uncommittedSize)
{
    unsigned long mask = page_mask();
    char *lo = (char *) (((unsigned long) addr + mask) & ~mask);
    char *hi = (char *) (((unsigned long) addr + size) & ~mask);

    if (hi > lo) {
	/* Fresh PROT_NONE pages: the old contents go back to the system. */
	mmap(lo, (size_t) (hi - lo), PROT_NONE,
	     MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0);
    }
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

/* Managed class allocation is shared in gc.c (allocClassClass). */
