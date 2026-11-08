/* DR2 libObjCJava: 0x53d0ca4c–0x53d0cce0. */
#import "vm.h"
#include <stdlib.h>

static void booleanOption(id options, NSString *key, jint *value)
{
    id option = [options objectForKey:key];
    if (option) *value = [@"YES" isEqual:option] ? 1 : 0;
}

static void integerOption(id options, NSString *key, jint *value)
{
    id option = [options objectForKey:key];
    if (option) *value = [option intValue];
}

void *JAVALoadVirtualMachine(NSString *classpath, id options)
{
    JDK1_1InitArgs args;
    JavaVM *vm = NULL;
    JNIEnv *env = NULL;
    JavaBridgeVM *data;
    jint debugging;
    JNI_GetDefaultJavaVMInitArgs(&args);
    args.verifyMode = 0;
    args.classpath = malloc([classpath length] + 1);
    [classpath getCString:args.classpath];
    if (options) {
        booleanOption(options, @"NSJavaCheckSource", &args.checkSource);
        integerOption(options, @"NSJavaNativeStackSize", &args.nativeStackSize);
        integerOption(options, @"NSJavaStackSize", &args.javaStackSize);
        integerOption(options, @"NSJavaMinHeapSize", &args.minHeapSize);
        integerOption(options, @"NSJavaMaxHeapSize", &args.maxHeapSize);
        booleanOption(options, @"NSJavaVerifyMode", &args.verifyMode);
        booleanOption(options, @"NSJavaEnableClassGC", &args.enableClassGC);
        booleanOption(options, @"NSJavaEnableVerboseGC", &args.enableVerboseGC);
        booleanOption(options, @"NSJavaDisableAsyncGC", &args.disableAsyncGC);
        booleanOption(options, @"NSJavaVerbose", &args.verbose);
        debugging = args.debugging;
        booleanOption(options, @"NSJavaDebugging", &debugging);
        args.debugging = debugging;
        integerOption(options, @"NSJavaDebugPort", &args.debugPort);
    }
    args.enableClassGC = 0;
    JNI_CreateJavaVM(&vm, &env, &args);
    /* The runtime retains its own class path; the adapter's copy is temporary. */
    free(args.classpath);
    if (!vm) {
        NSLog(@"Couldn't load the Java virtual machine.");
        return NULL;
    }
    data = malloc(sizeof(*data));
    data->vm = vm;
    data->env = env;
    return data;
}

void JAVAUnloadVirtualMachine(void *opaque)
{
    free(opaque);
    /* The donor releases its adapter record, not the process-wide VM. */
    NSLog(@"JAVAUnloadVirtualMachine: sorry, don't know how.");
}

void JAVAAttachCurrentThread(void *opaque)
{
    JavaBridgeVM *data = opaque;
    if (data) {
        JNIEnv *env = data->env;
        (*data->vm)->AttachCurrentThread(data->vm, &env, NULL);
    }
}

void JAVADetachCurrentThread(void *opaque)
{
    JavaBridgeVM *data = opaque;
    if (data) (*data->vm)->DetachCurrentThread(data->vm);
}

jclass JAVAAddClassFile(void *opaque, const char *name, const void *bytes,
    unsigned length)
{
    JavaBridgeVM *data = opaque;
    return (*data->env)->DefineClass(data->env, name, NULL, bytes, length);
}
