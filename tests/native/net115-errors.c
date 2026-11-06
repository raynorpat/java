/* Socket error mapping recovered from Sun net.dll 0x10001be0. */
#include "net.h"
#include <stdio.h>
static char *error_type, *error_text;
void SignalError(struct execenv *ee, char *type, char *message)
{
    error_type = type;
    error_text = message;
}
int main(void)
{
    int i;
    static const struct { int code; char *message; } cases[] = {
        { NET_ERR(EPROTOTYPE), "Invalid protocol type" },
        { NET_ERR(ENOPROTOOPT), "Option unsupported by protocol" },
        { NET_ERR(EPROTONOSUPPORT), "Option unsupported by protocol" },
        { NET_ERR(ESOCKTNOSUPPORT), "Option unsupported by protocol" },
        { NET_ERR(EOPNOTSUPP), "Option unsupported by protocol" },
        { NET_ERR(EPFNOSUPPORT), "Option unsupported by protocol" },
        { NET_ERR(EAFNOSUPPORT), "Option unsupported by protocol" },
        { NET_ERR(ESHUTDOWN), "Connection shutdown" },
        { NET_ERR(EHOSTDOWN), "Host down" }
    };
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        netSignalError(cases[i].code, "operation");
        if (strcmp(error_type, "java/net/SocketException") || strcmp(error_text, cases[i].message)) {
            fprintf(stderr, "error %d mapped to %s: %s\n", cases[i].code, error_type, error_text);
            return 1;
        }
    }
    return 0;
}
