#import <JavaVM/JavaVM.h>
#include <stdio.h>
int main(void)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    printf("%s\n", [NSStringFromClass([NSJavaObject class]) cString]);
    [pool release];
    return 0;
}
