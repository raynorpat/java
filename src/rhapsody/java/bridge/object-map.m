/* DR2 non-owning identity maps, libObjCJava 0x53d08614-0x53d087fc. */
#import "object-map.h"
#include <mach/cthreads.h>

static NSMapTable *objcToJava, *javaToObjc;
struct mutex MapLock;
#define mapLock MapLock

void bridgeInitializeObjectMap(void)
{
    mutex_lock(&mapLock);
    if (!objcToJava) {
        objcToJava = NSCreateMapTableWithZone(NSNonOwnedPointerMapKeyCallBacks,
            NSNonOwnedPointerMapValueCallBacks, 1, NULL);
        javaToObjc = NSCreateMapTableWithZone(NSNonOwnedPointerMapKeyCallBacks,
            NSNonOwnedPointerMapValueCallBacks, 1, NULL);
    }
    mutex_unlock(&mapLock);
}

void NSJavaMapObjects(void *javaHandle, id object, BOOL retainObject)
{
    mutex_lock(&mapLock);
    NSMapInsert(objcToJava, object, javaHandle);
    NSMapInsert(javaToObjc, javaHandle, object);
    mutex_unlock(&mapLock);
    if (retainObject) [object retain];
}

void NSJavaRemoveMapping(void *javaHandle, id object, BOOL releaseObject)
{
    mutex_lock(&mapLock);
    if (javaToObjc && javaHandle) NSMapRemove(javaToObjc, javaHandle);
    if (objcToJava && object) NSMapRemove(objcToJava, object);
    mutex_unlock(&mapLock);
    if (releaseObject) [object release];
}

void *NSJavaLookupId(id object)
{
    void *handle;
    mutex_lock(&mapLock);
    handle = objcToJava ? NSMapGet(objcToJava, object) : NULL;
    mutex_unlock(&mapLock);
    return handle;
}

id NSJavaLookupHandle(void *javaHandle)
{
    id object;
    mutex_lock(&mapLock);
    object = javaToObjc ? NSMapGet(javaToObjc, javaHandle) : nil;
    mutex_unlock(&mapLock);
    return object;
}
