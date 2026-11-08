#ifndef JDK115_BRIDGE_RETENTION_H
#define JDK115_BRIDGE_RETENTION_H
#include "oobj.h"
#include "interpreter.h"

void JavaGCInitialization(void);
/* Raw adapters: callers must convert any pending Java exception. */
HObject *bridgeRetainJavaObject(HObject *object);
void bridgeReleaseJavaObject(HObject *object);
#endif
