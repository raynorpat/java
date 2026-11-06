/* Reconstructed from Sun JDK 1.1.5 sysresource.dll and sysresource_g.dll.
 * The VM owns resource validation, classpath lookup and ZIP storage.
 */
#include "tree.h"
#include "interpreter.h"
#include "javaString.h"

JAVA_PUBLIC_API(HString *) resource_GetEntryFromKey(long key);
JAVA_PUBLIC_API(HArrayOfByte *) resource_GetZipByteArray(char *entry, char *name);
JAVA_PUBLIC_API(long) resource_ValidateSystemResource(long zip, char *entry, char *name);

#define STUB(name) Java_sun_net_www_protocol_systemresource_SystemResourceManager_##name##_stub
#ifdef XP_WIN32
#define RESOURCE_PATH_MAX 260
#else
#define RESOURCE_PATH_MAX 1024
#endif

stack_item *STUB(getEntryFromKey)(stack_item *sp, ExecEnv *ee)
{
    sp[0].p = resource_GetEntryFromKey(sp[0].i);
    return sp + 1;
}

stack_item *STUB(getZipResourceByteArray)(stack_item *sp, ExecEnv *ee)
{
    char entry[RESOURCE_PATH_MAX], name[RESOURCE_PATH_MAX];
    javaString2CString((HString *)sp[0].p, entry, sizeof(entry));
    javaString2CString((HString *)sp[1].p, name, sizeof(name));
    sp[0].p = resource_GetZipByteArray(entry, name);
    return sp + 1;
}

stack_item *STUB(validateSystemResource)(stack_item *sp, ExecEnv *ee)
{
    char entry[RESOURCE_PATH_MAX], name[RESOURCE_PATH_MAX];
    javaString2CString((HString *)sp[1].p, entry, sizeof(entry));
    javaString2CString((HString *)sp[2].p, name, sizeof(name));
    sp[0].i = resource_ValidateSystemResource(sp[0].i, entry, name) != 0;
    return sp + 1;
}
