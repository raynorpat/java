/* Test-only launcher: join a logged-in desktop's Mach bootstrap namespace.
 * An SSH session cannot otherwise find the desktop's distributed notification
 * service. This changes only the launcher, never the selected desktop task.
 */
#include <mach/mach.h>
#include <servers/bootstrap.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    task_t desktop;
    port_t port, service;
    kern_return_t result;
    if (argc < 3) {
        fprintf(stderr, "usage: desktop-launch desktop-pid executable [args...]\n");
        return 1;
    }
    result = task_by_unix_pid(task_self(), atoi(argv[1]), &desktop);
    if (!result) result = task_get_bootstrap_port(desktop, &port);
    if (!result) result = bootstrap_look_up(port,
        "Apple DNC Server (Local Host)", &service);
    if (!result) result = task_set_bootstrap_port(task_self(), port);
    if (result) {
        fprintf(stderr, "desktop bootstrap unavailable: %d\n", result);
        return 2;
    }
    execv(argv[2], argv + 2);
    perror("execv");
    return 3;
}
