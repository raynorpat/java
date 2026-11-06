/* Native host API regression: readiness must not consume data or seek. */
#include "sys_api.h"
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef XP_PC
#include <io.h>
#define make_pipe(p) _pipe(p, 512, _O_BINARY)
#else
#include <unistd.h>
#define make_pipe(p) pipe(p)
#endif

int main(void)
{
    FILE *file;
    int fd, pipes[2];
    long bytes;
    struct stat st;
    sys_dir_t *dir;
    file = tmpfile();
    assert(file != NULL);
    fd = fileno(file);
    assert(sysWrite(fd, "hello", 5) == 5);
    assert(sysSeek(fd, 2, SEEK_SET) == 2);
    assert(sysAvailable(fd, &bytes) == 1 && bytes == 3);
    assert(sysSeek(fd, 0, SEEK_CUR) == 2);
    assert(sysFStat(fd, &st) == 0 && st.st_size == 5);
    fclose(file);
    assert(make_pipe(pipes) == 0);
    assert(sysWrite(pipes[1], "abc", 3) == 3);
    assert(sysAvailable(pipes[0], &bytes) == 1 && bytes == 3);
    assert(sysAvailable(pipes[0], &bytes) == 1 && bytes == 3);
    sysClose(pipes[1]);
    assert(sysAvailable(pipes[0], &bytes) == 1 && bytes == 3);
    { char buffer[3]; assert(sysRead(pipes[0], buffer, 3) == 3); }
    assert(sysAvailable(pipes[0], &bytes) == 1 && bytes == 0);
    sysClose(pipes[0]);
    dir = sysOpenDir(".");
    assert(dir != NULL);
    {
        char first[MAXPATHLEN];
        struct dirent *entry = sysReadDir(dir);
        assert(entry != NULL);
        strcpy(first, entry->d_name);
        while (sysReadDir(dir) != NULL) {}
        assert(sysReadDir(dir) == NULL);
        rewinddir(dir);
        entry = sysReadDir(dir);
        assert(entry != NULL && strcmp(first, entry->d_name) == 0);
#ifdef XP_PC
        assert(sizeof(DIR) == 588);
        assert(offsetof(DIR, path) == 260 && offsetof(DIR, handle) == 264);
        assert(offsetof(DIR, find_data) == 268);
#endif
    }
    assert(sysCloseDir(dir) == 0);
    assert(sysIsMP() == 0 || sysIsMP() == 1);
    sysMemoryFlush();
    sysStoreBarrier();
    return 0;
}
