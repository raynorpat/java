#include "sys_api.h"
#include <mach/cthreads.h>
#include <stdlib.h>
#include <stdio.h>

static any_t check(any_t cookie)
{
    time_t timer = (long)cookie * 86400;
    struct tm value;
    int i;
    for (i = 0; i < 10000; i++) {
        if (sysGmtime(&timer, &value) != &value ||
            value.tm_year != 70 || value.tm_mon != 0 ||
            value.tm_mday != (long)cookie + 1)
            return (any_t)1;
        if (sysLocaltime(&timer, &value) != &value ||
            value.tm_year != 70 || value.tm_mon != 0 ||
            value.tm_mday != (long)cookie + 1)
            return (any_t)1;
    }
    return 0;
}

int main(void)
{
    cthread_t threads[4];
    int i, failed = 0;
    putenv("TZ=GMT0");
    tzset();
    for (i = 0; i < 4; i++) threads[i] = cthread_fork(check, (any_t)(long)i);
    for (i = 0; i < 4; i++) if (cthread_join(threads[i])) failed = 1;
    if (!failed) puts("DR2 concurrent time conversion passed");
    return failed;
}
