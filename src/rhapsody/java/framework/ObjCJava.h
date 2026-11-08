#ifndef JDK115_PUBLIC_OBJCJAVA_H
#define JDK115_PUBLIC_OBJCJAVA_H
#import "java-objc.h"
void *NSLoadVirtualMachine(NSString *classpath);
void NSUnloadVirtualMachine(void *vm);
BOOL NSAddClassFile(void *vm, NSString *name, NSData *bytes);
void JAVAAttachCurrentThread(void *vm);
void JAVADetachCurrentThread(void *vm);
#endif
