/* Reconstructed from JDK 1.1.5 javai_g.dll, NameAndTypeToHash 10012685. */
#include "tree.h"
#include "interpreter.h"
#include "monitor.h"
#include "str2id.h"
#include "classhash115.h"

static StrIDhash *interfaceHash;
static sys_mon_t *interfaceHashLock;
/* Compatibility only: old diagnostic consumers need a stable combined string.
 * A separate table keeps those strings out of Sun's member-ID sequence. */
static StrIDhash *diagnosticHash;

/* Called during single-threaded VM/tool initialization, before resolving classes. */
int initializeClassHash115(void)
{
    if (interfaceHashLock) return TRUE;
    interfaceHashLock = sysMalloc(sysMonitorSizeof());
    if (!interfaceHashLock) return FALSE;
    memset(interfaceHashLock, 0, sysMonitorSizeof());
    monitorRegister(interfaceHashLock, "Name and type hash lock");
    return TRUE;
}

JAVA_PUBLIC_API(unsigned int)
NameAndTypeToHash(char *name, char *type)
{
    unsigned short nameID, typeID;
    unsigned int result = 0;
    sysAssert(interfaceHashLock != NULL);
    sysMonitorEnter(interfaceHashLock);
    nameID = Str2ID(&interfaceHash, name, NULL, TRUE);
    if (nameID && (typeID = Str2ID(&interfaceHash, type, NULL, TRUE)) != 0)
        result = ((unsigned int)nameID << 16) + typeID;
    else
        SignalError(NULL, JAVAPKG "OutOfMemoryError", NULL);
    sysMonitorExit(interfaceHashLock);
    return result;
}

char *IDToType(unsigned int ID)
{
    char *type;
    sysMonitorEnter(interfaceHashLock);
    type = ID2Str(interfaceHash, (unsigned short)ID, NULL);
    sysMonitorExit(interfaceHashLock);
    return type;
}

char *IDToNameAndTypeString(unsigned int ID)
{
    char *name, *type, *combined, *result = NULL;
    size_t nameLength, typeLength;
    unsigned short stringID;
    sysMonitorEnter(interfaceHashLock);
    name = ID2Str(interfaceHash, (unsigned short)(ID >> 16), NULL);
    type = ID2Str(interfaceHash, (unsigned short)ID, NULL);
    nameLength = strlen(name);
    typeLength = strlen(type);
    combined = sysMalloc(nameLength + typeLength + 2);
    if (combined) {
        memcpy(combined, name, nameLength);
        combined[nameLength] = ' ';
        memcpy(combined + nameLength + 1, type, typeLength + 1);
        stringID = Str2ID(&diagnosticHash, combined, NULL, TRUE);
        sysFree(combined);
        if (stringID) result = ID2Str(diagnosticHash, stringID, NULL);
    }
    if (!result) SignalError(NULL, JAVAPKG "OutOfMemoryError", NULL);
    sysMonitorExit(interfaceHashLock);
    return result;
}
