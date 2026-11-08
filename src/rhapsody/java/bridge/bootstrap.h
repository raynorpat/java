#ifndef JDK115_BRIDGE_BOOTSTRAP_H
#define JDK115_BRIDGE_BOOTSTRAP_H
#import <Foundation/Foundation.h>
#include <mach/cthreads.h>
extern NSRecursiveLock *_NSBridgeLock;
extern cthread_t _NSFirstThread;
extern int _NSJavaSafeDeallocation;
void *NSLoadVirtualMachine(NSString *classpath);
void NSUnloadVirtualMachine(void *vm);
BOOL NSAddClassFile(void *vm, NSString *name, NSData *bytes);
void _NextJavaTableInitialization(void);
void ObjCJava_AInitialization(void);
void ObjCJavaInitialization(void);
int _NSJavaStubEnter(void);
void _NSJavaStubExit(int state);
void _NSJavaSafeRelease(id object);
void _NSJavaSafePurge(void);
void _NSJavaEnableSafeDeallocation(BOOL enabled);
void GoThreadSafe(void);
#endif
