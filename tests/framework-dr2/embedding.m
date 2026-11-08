/* Donor oracle for the facade; startup mode also enters the real ObjC bridge. */
#import <Foundation/Foundation.h>
#ifdef RECONSTRUCTED
#import "JavaVM.h"
#else
#import <JavaVM/JavaVM.h>
#endif
#include <stdio.h>
#include <string.h>

@interface NSJavaVirtualMachine (ClassDefinition)
- (BOOL)defineClass:(NSData *)data withName:(NSString *)name;
@end

int main(int argc, char **argv)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    NSJavaVirtualMachine *vm;
    printf("initial-count=%u\n", [[NSJavaVirtualMachine allVirtualMachines] count]);
    fflush(stdout);
    if (argc > 1 && !strcmp(argv[1], "missing-bridge")) {
        NS_DURING
            [NSJavaVirtualMachine defaultVirtualMachine];
            printf("unexpected-bridge-loaded\n");
            return 1;
        NS_HANDLER
            if (![[localException name] isEqual:NSInternalInconsistencyException]
                || [[NSJavaVirtualMachine allVirtualMachines] count] != 0)
                return 1;
            printf("private-bridge-required=1\n");
        NS_ENDHANDLER
    } else if (argc > 1 && !strcmp(argv[1], "startup")) {
        vm = [NSJavaVirtualMachine defaultVirtualMachine];
        printf("vm=%d\n", vm != nil);
        printf("count=%u\n", [[NSJavaVirtualMachine allVirtualMachines] count]);
        printf("same-vm=%d\n", vm == [NSJavaVirtualMachine defaultVirtualMachine]);
        printf("foundation-class=%d\n", [vm findClass:@"NSObject"] == [NSObject class]);
        printf("java-class=%d\n", [vm findClass:@"java.lang.Object"] != Nil);
        printf("missing-class=%d\n", [vm findClass:@"framework.fixture.Missing"] == Nil);
        NS_DURING
            [[[NSJavaVirtualMachine alloc] init] release];
            printf("second-vm=no-exception\n");
        NS_HANDLER
            printf("second-vm=%s\n", [[localException name] cString]);
        NS_ENDHANDLER
    } else {
        printf("classpath=%s\n", [[NSJavaVirtualMachine defaultClassPath] cString]);
        NS_DURING
            [NSObject newWithSignature:@"()V"];
            printf("non-java=no-exception\n");
        NS_HANDLER
            printf("non-java=%s\n", [[localException name] cString]);
            printf("reason=%s\n", [[localException reason] cString]);
        NS_ENDHANDLER
    }
    fflush(stdout);
    [pool release];
    return 0;
}
