/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
#include "tree.h"
#include "javaString.h"
#include "str2id.h"
#include "utf.h"
#include "monitor.h"

StrIDhash *stringHash;
extern sys_mon_t *_heap_lock;

Hjava_lang_String *java_lang_String_intern(Hjava_lang_String *string)
{
    char *utf;
    void **slot;
    Hjava_lang_String *result;
    Classjava_lang_String *value = unhand(string);
    utf = unicode2utf(unhand(value->value)->body + value->offset,
                      value->count, NULL, 0);
    if (!utf) {
        SignalError(0, JAVAPKG "OutOfMemoryError", 0);
        return NULL;
    }
    /* deviation: the existing collector's heap lock also protects its roots,
       replacing Sun's separate string-table lock and GC lock ordering. */
    sysMonitorEnter(_heap_lock);
    if (!Str2ID(&stringHash, utf, &slot, TRUE)) {
        sysMonitorExit(_heap_lock);
        free(utf);
        SignalError(0, JAVAPKG "OutOfMemoryError", 0);
        return NULL;
    }
    if (!*slot) *slot = string;
    result = (Hjava_lang_String *)*slot;
    sysMonitorExit(_heap_lock);
    free(utf);
    return result;
}
