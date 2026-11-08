#include "net.h"
#include <sys/resource.h>
#include <fcntl.h>
#include <stdio.h>
static char *error_type;
void SignalError(struct execenv *ee, char *type, char *message)
{
    error_type = type;
}
int main(void)
{
    struct rlimit limit;
    int pair[2], high, ready;
    Classjava_io_FileDescriptor value;
    Hjava_io_FileDescriptor handle;
    getrlimit(RLIMIT_NOFILE, &limit);
    limit.rlim_cur = 1024;
    if (setrlimit(RLIMIT_NOFILE, &limit)) { perror("setrlimit"); return 1; }
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair)) { perror("socketpair"); return 1; }
    high = fcntl(pair[0], F_DUPFD, 300);
    if (high < 300) return 2;
    value.fd = pair[0] + 1;
    handle.obj = &value;
    ready = netWait(&handle, 10, "timeout");
    if (ready != 0 || !error_type || strcmp(error_type, "java/io/InterruptedIOException")) {
        printf("timeout ready=%d error=%s errno=%d\n", ready, error_type ? error_type : "none", errno);
        return 3;
    }
    error_type = NULL;
    if (write(pair[1], "x", 1) != 1 || netWait(&handle, 10, "timeout") != 1 || error_type)
        return 4;
    error_type = NULL;
    value.fd = high + 1;
    if (netWait(&handle, 10, "timeout") != 0 || !error_type ||
        strcmp(error_type, "java/net/SocketException")) return 5;
    close(high); close(pair[0]); close(pair[1]);
    puts("DR2 socket readiness and select limit passed");
    return 0;
}
