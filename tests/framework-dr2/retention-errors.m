#import <Foundation/Foundation.h>
#import "retention.h"
#import "vm.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    ExecEnv *env;
    if (argc != 2) return 2;
    if (!JAVALoadVirtualMachine([NSString stringWithCString:argv[1]], nil)) return 1;
    env = EE();
    JavaGCInitialization();
    bridgeRetainJavaObject(NULL);
    if (!exceptionOccurred(env) || !env->exception.exc) return 1;
    if (strcmp(cbName(obj_classblock(env->exception.exc)), "java/lang/NullPointerException")) return 1;
    exceptionClear(env);
    bridgeReleaseJavaObject(NULL);
    if (exceptionOccurred(env)) return 1;
    puts("PASS raw retention exception and recovery");
    [pool release];
    return 0;
}
