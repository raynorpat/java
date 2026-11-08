#ifndef JDK115_BRIDGE_CODING_H
#define JDK115_BRIDGE_CODING_H
#import <Foundation/Foundation.h>
#include "oobj.h"
void SerializeInitialization(void);
void NSArchiveJavaObject(HObject *object, NSCoder *coder);
HObject *NSUnarchiveJavaObject(NSCoder *coder);
HObject *ObjectOutputStreamForCoder(NSCoder *coder);
HObject *ObjectInputStreamForCoder(NSCoder *coder);
NSCoder *CoderForObjectOutputStream(HObject *stream);
NSCoder *CoderForObjectInputStream(HObject *stream);
void NSSerializeObjCObject(id object, HObject *stream);
id NSDeserializeObjCObject(HObject *stream);
#endif
