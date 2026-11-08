/* DR2 ObjCJava VM adapter. The first two fields match the donor ABI. */
#import <Foundation/Foundation.h>
#include "jni.h"

typedef struct {
    JavaVM *vm;
    JNIEnv *env;
} JavaBridgeVM;

void *JAVALoadVirtualMachine(NSString *classpath, id options);
void JAVAUnloadVirtualMachine(void *data);
void JAVAAttachCurrentThread(void *data);
void JAVADetachCurrentThread(void *data);
jclass JAVAAddClassFile(void *data, const char *name, const void *bytes,
    unsigned length);
