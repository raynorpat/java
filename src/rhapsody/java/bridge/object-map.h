#ifndef JDK115_BRIDGE_OBJECT_MAP_H
#define JDK115_BRIDGE_OBJECT_MAP_H
#import <Foundation/Foundation.h>

void bridgeInitializeObjectMap(void);
void NSJavaMapObjects(void *javaHandle, id object, BOOL retainObject);
void NSJavaRemoveMapping(void *javaHandle, id object, BOOL releaseObject);
void *NSJavaLookupId(id object);
id NSJavaLookupHandle(void *javaHandle);
#endif
