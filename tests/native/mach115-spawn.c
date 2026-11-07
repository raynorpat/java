/* Real pipe/exec contract for the DR2 host adapter (run on Linux). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include "cthreads/process.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
int main(void)
{
    int pipes[6], i, status, result;
    pid_t pid;
    char buffer[32];
    char *args[] = { "sh", "-c", "cat; echo $VALUE >&2; exit 7", NULL };
    char *env[] = { "VALUE=mach", NULL };
    char *missing[] = { "/no/such/program115", NULL };
    for (i = 0; i < 6; i += 2) CHECK(pipe(pipes + i) == 0);
    CHECK(mach115_spawn(&pid, args, env, pipes) == 0);
    close(pipes[0]); close(pipes[3]); close(pipes[5]);
    CHECK(write(pipes[1], "hello\n", 6) == 6); close(pipes[1]);
    CHECK(read(pipes[2], buffer, sizeof(buffer)) == 6 && !memcmp(buffer, "hello\n", 6));
    CHECK(read(pipes[4], buffer, sizeof(buffer)) == 5 && !memcmp(buffer, "mach\n", 5));
    close(pipes[2]); close(pipes[4]);
    CHECK(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 7);
    for (i = 0; i < 6; i += 2) CHECK(pipe(pipes + i) == 0);
    result = mach115_spawn(&pid, missing, env, pipes);
    CHECK(result == ENOENT);
    for (i = 0; i < 6; i++) close(pipes[i]);
    CHECK(waitpid(-1, &status, WNOHANG) == -1 && errno == ECHILD);
    puts("Mach process adapter contracts passed");
    return 0;
}
