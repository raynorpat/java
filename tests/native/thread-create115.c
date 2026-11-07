/* Shared HPI must reject every failed creation without publishing a pointer. */
#include <stdio.h>
#include <string.h>
#include "threads.h"
#include "java_lang_Thread.h"
static int status;
int sysThreadCreate(long size, unsigned int flags, void *(*start)(void *),
                    sys_thread_t **out, void *cookie)
{
    *out = (sys_thread_t *)0x1234;
    return status;
}
int main(void)
{
    struct Classjava_lang_Thread object;
    Hjava_lang_Thread handle;
    int errors[] = { SYS_ERR, SYS_NORESOURCE, SYS_NOMEM };
    unsigned int i;
    memset(&object, 0, sizeof(object));
    memset(&handle, 0, sizeof(handle));
    handle.obj = &object;
    for (i = 0; i < sizeof(errors)/sizeof(errors[0]); i++) {
        status = errors[i];
        if (threadCreate(&handle, 0, 0, NULL) != 1 || object.PrivateInfo != 0) {
            fprintf(stderr, "creation published failed status %d\n", status);
            return 1;
        }
    }
    status = SYS_OK;
    if (threadCreate(&handle, 0, 0, NULL) || object.PrivateInfo != 0x1234) return 1;
    puts("thread creation status contract passed");
    return 0;
}
