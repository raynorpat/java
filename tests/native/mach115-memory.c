#include <stdio.h>
#include <stdlib.h>
#include <mach/mach.h>
#include "sys_api.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
int task_self_ = 7;
vm_size_t vm_page_size = 4096;
static int fail, allocs, frees;
kern_return_t vm_allocate(unsigned int task, vm_address_t *addr, vm_size_t size, boolean_t anywhere)
{ CHECK(task == 7 && size == 8192 && anywhere); ++allocs; *addr = 0x10000; return fail; }
kern_return_t vm_deallocate(unsigned int task, vm_address_t addr, vm_size_t size)
{ CHECK(task == 7 && addr == 0x10000 && size == 8192); ++frees; return fail; }
unsigned char *gc0_locked(int a, unsigned int b) { return NULL; }
int main(void)
{
    long size;
    CHECK(sysMapMem(4097, &size) == (void *)0x10000 && size == 8192 && allocs == 1);
    fail = 1;
    CHECK(sysMapMem(4097, &size) == NULL && size == 0);
    CHECK(sysUnmapMem((void *)0x10000, 4097, &size) == NULL && size == 0);
    fail = 0;
    CHECK(sysUnmapMem((void *)0x10000, 4097, &size) == (void *)0x10000 && size == 8192 && frees == 2);
    /* Shared collector requires the exact caller address/length back. */
    CHECK(sysCommitMem((void *)0x10005, 11, &size) == (void *)0x10005 && size == 11);
    CHECK(sysUncommitMem((void *)0x10005, 11, &size) == (void *)0x10005 && size == 11);
    CHECK(allocs == 2 && frees == 2);
    puts("Mach heap contracts passed");
    return 0;
}
