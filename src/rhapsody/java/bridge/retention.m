/* DR2 Java-side GC roots, libObjCJava 0x53d05448-0x53d05590. */
#import <Foundation/Foundation.h>
#include "retention.h"
#include "invocation.h"

static ClassClass *runtimeClass;
static struct methodblock *putMethod, *removeMethod;

void JavaGCInitialization(void)
{
    ExecEnv *env = EE();
    const char *name = "com/apple/yellow/foundation/NSRuntime";
    runtimeClass = FindClass(env, (char *)name, TRUE);
    if (!runtimeClass) {
        NSLog(@"ObjCJava: can't locate runtime class %s.  Objects will leak and the program may crash.", name);
        return;
    }
    putMethod = _JAVAMethodLookup(env, runtimeClass, "retainObject",
        "(ILjava/lang/Object;)V", FALSE, TRUE);
    removeMethod = _JAVAMethodLookup(env, runtimeClass, "releaseObject",
        "(I)V", FALSE, TRUE);
    if (!putMethod || !removeMethod)
        [NSException raise:NSInternalInconsistencyException
            format:@"ObjCJava: can't find retainObject and/or releaseObject methods in the %s class", name];
}

HObject *bridgeRetainJavaObject(HObject *object)
{
    long args[2];
    args[0] = (long)object;
    args[1] = (long)object;
    do_execute_java_method_vararg(EE(), (HObject *)runtimeClass, NULL, NULL,
        putMethod, TRUE, (va_list)args, NULL, FALSE);
    return object;
}

void bridgeReleaseJavaObject(HObject *object)
{
    do_execute_java_method_vararg(EE(), (HObject *)runtimeClass, NULL, NULL,
        removeMethod, TRUE, (va_list)&object, NULL, FALSE);
}
