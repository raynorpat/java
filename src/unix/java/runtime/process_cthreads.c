/* DR2 uses execve and pipes (5C13733C), not posix_spawn. Keep the current
 * Win32Process class adapter, but use fork with an exec-error pipe rather
 * than the original vfork/start gate. No allocation or VM calls in the child. */
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "cthreads/process.h"

int mach115_spawn(pid_t *pid, char **argv, char **env, int *pipes)
{
    extern char **environ;
    const char *path, *end;
    char *program;
    int channel[2], error = ENOENT, i, status;
    size_t length, used = 0;
    ssize_t count;
    pid_t child;
    /* Resolve PATH before fork so the child only calls async-safe functions. */
    path = getenv("PATH");
    if (!path) path = "/bin:/usr/bin";
    program = malloc(strlen(path) + strlen(argv[0]) + 2);
    if (!program) return ENOMEM;
    if (strchr(argv[0], '/')) strcpy(program, argv[0]);
    else {
        for (;;) {
            end = strchr(path, ':');
            length = end ? (size_t)(end - path) : strlen(path);
            memcpy(program, path, length);
            if (length) program[length++] = '/';
            strcpy(program + length, argv[0]);
            if (access(program, X_OK) == 0) break;
            if (errno == EACCES) error = EACCES;
            if (!end) { free(program); return error; }
            path = end + 1;
        }
    }
    if (pipe(channel) != 0) { error = errno; free(program); return error; }
    if (fcntl(channel[1], F_SETFD, FD_CLOEXEC) < 0) {
        error = errno; close(channel[0]); close(channel[1]); free(program); return error;
    }
    child = fork();
    if (child == 0) {
        close(channel[0]);
        if (dup2(pipes[0], 0) < 0 || dup2(pipes[3], 1) < 0 || dup2(pipes[5], 2) < 0)
            error = errno;
        else {
            for (i = 0; i < 6; i++) if (pipes[i] > 2) close(pipes[i]);
            execve(program, argv, env ? env : environ);
            error = errno;
        }
        while (write(channel[1], &error, sizeof(error)) < 0 && errno == EINTR) {}
        _exit(127);
    }
    error = errno;
    free(program); close(channel[1]);
    if (child < 0) { close(channel[0]); return error; }
    error = 0;
    do {
        count = read(channel[0], (char *)&error + used, sizeof(error) - used);
        if (count > 0) used += count;
    } while ((count < 0 && errno == EINTR) || (count > 0 && used < sizeof(error)));
    if (count < 0) error = errno;
    close(channel[0]);
    if (error) {
        while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
        return error;
    }
    *pid = child;
    return 0;
}
