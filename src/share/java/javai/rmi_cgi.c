/* Reconstructed from JDK 1.1.5 java-rmi.cgi (Win32), main at 0x401000. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef XP_PC
#include <process.h>
#else
#include <unistd.h>
#endif

static int failure(const char *reason)
{
    printf("Status: 500 Server Error: %s\nContent-type: text/plain\n\n%s",
           reason, reason);
    return 1;
}

int main(void)
{
    static const char *names[] = {
        "AUTH_TYPE", "CONTENT_LENGTH", "CONTENT_TYPE", "GATEWAY_INTERFACE",
        "HTTP_ACCEPT", "PATH_INFO", "PATH_TRANSLATED", "QUERY_STRING",
        "REMOTE_ADDR", "REMOTE_HOST", "REMOTE_IDENT", "REMOTE_USER",
        "REQUEST_METHOD", "SCRIPT_NAME", "SERVER_NAME", "SERVER_PORT",
        "SERVER_PROTOCOL", "SERVER_SOFTWARE"
    };
    char **args = malloc(21 * sizeof(char *));
    const char *value;
    int i;
    if (!args) return failure("memory allocation failure");
    args[0] = "java";
    for (i = 0; i < 18; ++i) {
        value = getenv(names[i]);
        if (!value) value = "";
        args[i + 1] = malloc(strlen(names[i]) + strlen(value) + 6);
        if (!args[i + 1]) return failure("memory allocation failure");
#ifdef XP_PC
        sprintf(args[i + 1], "-D%s=\"%s\"", names[i], value);
#else
        /* POSIX exec passes argument boundaries directly, without CRT quotes. */
        sprintf(args[i + 1], "-D%s=%s", names[i], value);
#endif
    }
    args[19] = "sun.rmi.transport.proxy.CGIHandler";
    args[20] = NULL;
#ifdef XP_PC
    _execvp("java", (const char * const *)args);
#else
    execvp("java", args);
#endif
    return failure("interpreter execution failure");
}
