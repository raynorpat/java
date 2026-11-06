/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/* JDK 1.1.5 resource reconstruction. See resource115-evidence.md.
 * Uses this port's existing classpath, ZIP and Java object APIs.
 */
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include "tree.h"
#include "interpreter.h"
#include "javaString.h"
#include "path.h"
#include "sys_api.h"

#define RESOURCE_PATH_MAX 260

static int class_resource(char *name)
{
    size_t length;
    char *suffix;
    length = strlen(name);
    if (length < 6) return 0;
    suffix = name + length - 6;
    return suffix[0] == '.' && tolower((unsigned char)suffix[1]) == 'c' &&
        tolower((unsigned char)suffix[2]) == 'l' &&
        tolower((unsigned char)suffix[3]) == 'a' &&
        tolower((unsigned char)suffix[4]) == 's' &&
        tolower((unsigned char)suffix[5]) == 's';
}

static char *entry_name(cpe_t *entry)
{
    return entry->type == CPE_DIR ? entry->u.dir : entry->u.zip->fn;
}

static int resource_path(char *buffer, char *dir, char *name)
{
    return jio_snprintf(buffer, RESOURCE_PATH_MAX, "%s%c%s", dir,
                        DIRECTORY_SEPARATOR, name) != -1;
}

static int clear_security(ClassClass *security)
{
    ExecEnv *ee;
    ee = EE();
    if (exceptionOccurred(ee) && is_instance_of(ee->exception.exc, security, ee)) {
        exceptionClear(ee);
        return 1;
    }
    return 0;
}

static long file_exists(char *path, ClassClass *security)
{
    HObject *file;
    HString *name;
    long exists;
    path = sysNativePath(path);
    name = makeJavaString(path, strlen(path));
    file = execute_java_constructor(EE(), "java/io/File", NULL,
                                    "(Ljava/lang/String;)", name);
    if (exceptionOccurred(EE())) {
        clear_security(security);
        return 0;
    }
    exists = execute_java_dynamic_method(EE(), file, "exists", "()Z");
    if (clear_security(security)) return 0;
    return exists;
}

static HArrayOfByte *zip_bytes(zip_t *zip, char *name)
{
    struct stat st;
    void *buffer;
    HArrayOfByte *bytes;
    if (!zip_stat(zip, name, &st)) return NULL;
    buffer = sysMalloc(st.st_size);
    if (buffer == NULL) return NULL;
    if (!zip_get(zip, name, buffer, st.st_size)) {
        sysFree(buffer);
        return NULL;
    }
    bytes = (HArrayOfByte *)ArrayAlloc(T_BYTE, st.st_size);
    if (bytes != NULL) memcpy(unhand(bytes)->body, buffer, st.st_size);
    sysFree(buffer);
    return bytes;
}

JAVA_PUBLIC_API(long)
resource_ValidateSystemResource(long is_zip, char *entry, char *name)
{
    cpe_t **path;
    ClassClass *security;
    struct stat st;
    char buffer[RESOURCE_PATH_MAX];
    security = FindClass(EE(), "java/lang/SecurityException", TRUE);
    if (security == NULL) return 0;
    for (path = sysGetClassPath(); path != NULL && *path != NULL; ++path) {
        if (strcmp(entry_name(*path), entry) != 0) continue;
        if ((*path)->type == CPE_ZIP)
            return is_zip && zip_stat((*path)->u.zip, name, &st);
        if (is_zip || !resource_path(buffer, (*path)->u.dir, name)) return 0;
        if (sysAccess(buffer, 0) != 0) return 0;
        return file_exists(buffer, security);
    }
    return 0;
}

JAVA_PUBLIC_API(HArrayOfByte *)
resource_GetZipByteArray(char *entry, char *name)
{
    cpe_t **path;
    if (class_resource(name)) {
        SignalError(EE(), "java/lang/SecurityException",
                    "Can't read classfile as system resource");
        return NULL;
    }
    for (path = sysGetClassPath(); path != NULL && *path != NULL; ++path) {
        if (strcmp(entry_name(*path), entry) == 0)
            return (*path)->type == CPE_ZIP ? zip_bytes((*path)->u.zip, name) : NULL;
    }
    return NULL;
}

JAVA_PUBLIC_API(HString *)
resource_GetEntryFromKey(long key)
{
    cpe_t **path;
    long index;
    char *name;
    for (path = sysGetClassPath(), index = 0;
         path != NULL && *path != NULL; ++path, ++index) {
        if (index == key) {
            name = entry_name(*path);
            return makeJavaString(name, strlen(name));
        }
    }
    return NULL;
}

HObject *
java_lang_ClassLoader_getSystemResourceAsStream0(void *unused, HString *resource)
{
    cpe_t **path;
    ClassClass *security;
    HArrayOfByte *bytes;
    HString *file;
    HObject *stream;
    struct stat st;
    char name[RESOURCE_PATH_MAX], buffer[RESOURCE_PATH_MAX];
    javaString2CString(resource, name, sizeof(name));
    security = FindClass(EE(), "java/lang/SecurityException", TRUE);
    if (security == NULL || class_resource(name)) return NULL;
    for (path = sysGetClassPath(); path != NULL && *path != NULL; ++path) {
        if ((*path)->type == CPE_ZIP) {
            if (!zip_stat((*path)->u.zip, name, &st)) continue;
            bytes = zip_bytes((*path)->u.zip, name);
            if (bytes == NULL) return NULL;
            return execute_java_constructor(EE(), "java/io/ByteArrayInputStream",
                                            NULL, "([B)", bytes);
        }
        if (!resource_path(buffer, (*path)->u.dir, name)) return NULL;
        if (sysAccess(buffer, 0) != 0) continue;
        sysNativePath(buffer);
        file = makeJavaString(buffer, strlen(buffer));
        stream = execute_java_constructor(EE(), "java/io/FileInputStream",
                                          NULL, "(Ljava/lang/String;)", file);
        if (clear_security(security)) return NULL;
        return stream;
    }
    return NULL;
}

HString *
java_lang_ClassLoader_getSystemResourceAsName0(void *unused, HString *resource)
{
    cpe_t **path;
    ClassClass *security;
    struct stat st;
    long key;
    char name[RESOURCE_PATH_MAX], buffer[RESOURCE_PATH_MAX];
    javaString2CString(resource, name, sizeof(name));
    security = FindClass(EE(), "java/lang/SecurityException", TRUE);
    if (security == NULL) return NULL;
    for (path = sysGetClassPath(), key = 0;
         path != NULL && *path != NULL; ++path, ++key) {
        if ((*path)->type == CPE_ZIP) {
            if (!zip_stat((*path)->u.zip, name, &st)) continue;
        } else {
            if (!resource_path(buffer, (*path)->u.dir, name)) return NULL;
            if (sysAccess(buffer, 0) != 0) continue;
            if (!file_exists(buffer, security)) return NULL;
        }
        if (jio_snprintf(buffer, sizeof(buffer), "systemresource:/%s%ld/+/%s",
                         (*path)->type == CPE_ZIP ? "ZIP" : "FILE", key, name) == -1)
            return NULL;
        return makeJavaString(buffer, strlen(buffer));
    }
    return NULL;
}
