#include <stddef.h>
#include "sys_api.h"
#include "threads_md.h"

#define CHECK(name, expression) typedef char name[(expression) ? 1 : -1]
CHECK(cookie_offset, offsetof(sys_thread_t, cookie) == 0);
CHECK(handle_offset, offsetof(sys_thread_t, handle) == 4);
CHECK(stack_offset, offsetof(sys_thread_t, stack_base) == 12);
CHECK(state_offset, offsetof(sys_thread_t, state) == 16);
CHECK(interrupt_event_offset, offsetof(sys_thread_t, interrupt_event) == 24);
CHECK(interrupted_offset, offsetof(sys_thread_t, interrupted) == 28);
CHECK(vmsuspended_offset, offsetof(sys_thread_t, vmsuspended) == 32);
CHECK(start_proc_offset, offsetof(sys_thread_t, start_proc) == 36);
CHECK(start_parm_offset, offsetof(sys_thread_t, start_parm) == 40);
CHECK(next_offset, offsetof(sys_thread_t, next) == 44);
CHECK(cache_offset, offsetof(sys_thread_t, monitorCache) == 52);
CHECK(cache_size, sizeof(((sys_thread_t *)0)->monitorCache) == 32);

int main(void)
{
    return 0;
}
