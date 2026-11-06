/* Reconstructed from net.dll 0x10001160-0x10001210, 0x100020a0-0x10002250. */
#include "net.h"
struct Hjava_lang_String *java_net_InetAddressImpl_getLocalHostName(struct Hjava_net_InetAddressImpl *self)
{
    char name[256];
    if (gethostname(name, sizeof(name)) < 0) strcpy(name, "localhost");
    name[255] = 0;
    return makeJavaString(name, strlen(name));
}
void java_net_InetAddressImpl_makeAnyLocalAddress(struct Hjava_net_InetAddressImpl *self, struct Hjava_net_InetAddress *addr)
{
    if (!addr) { SignalError(0, "java/lang/NullPointerException", 0); return; }
    unhand(addr)->address = 0;
    unhand(addr)->family = AF_INET;
}
long java_net_InetAddressImpl_getInetFamily(struct Hjava_net_InetAddressImpl *self) { return AF_INET; }
HArrayOfArray *java_net_InetAddressImpl_lookupAllHostAddr(struct Hjava_net_InetAddressImpl *self, struct Hjava_lang_String *host)
{
    char name[256];
    struct hostent *entry;
    HArrayOfArray *array;
    HArrayOfByte *bytes;
    int n, i;
    if (!host) { SignalError(0, "java/lang/NullPointerException", 0); return 0; }
    javaString2CString(host, name, sizeof(name));
    entry = gethostbyname(name);
    if (!entry) { SignalError(0, "java/net/UnknownHostException", name); return 0; }
    for (n = 0; entry->h_addr_list[n]; n++) {}
    array = (HArrayOfArray *)ArrayAlloc(T_CLASS, n);
    if (!array) return 0;
    unhand(array)->body[n] = (JHandle *)FindClass(0, "[B", TRUE);
    for (i = 0; i < n; i++) {
        bytes = (HArrayOfByte *)ArrayAlloc(T_BYTE, 4);
        if (!bytes) return 0;
        memcpy(unhand(bytes)->body, entry->h_addr_list[i], 4);
        unhand(array)->body[i] = (JHandle *)bytes;
    }
    return array;
}
struct Hjava_lang_String *java_net_InetAddressImpl_getHostByAddr(struct Hjava_net_InetAddressImpl *self, long address)
{
    unsigned long network = htonl(address);
    struct hostent *entry = gethostbyaddr((char *)&network, 4, AF_INET);
    if (!entry) { SignalError(0, "java/net/UnknownHostException", 0); return 0; }
    return makeJavaString(entry->h_name, strlen(entry->h_name));
}
