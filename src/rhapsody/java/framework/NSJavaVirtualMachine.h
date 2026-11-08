/* Reconstructed Rhapsody DR2 embedding interface. */
#import <Foundation/Foundation.h>
#include <stdarg.h>

@interface NSJavaVirtualMachine : NSObject
{
    void *_vmdata;
    void *_vtable;
}
+ (NSArray *)allVirtualMachines;
+ (id)defaultVirtualMachine;
+ (NSString *)defaultClassPath;
- initWithClassPath:(NSString *)classPath;
- (Class)findClass:(NSString *)javaClassName;
/* Present in the donor executable, with BOOL return encoding. */
- (BOOL)defineClass:(NSData *)data withName:(NSString *)name;
@end

@interface NSObject (InstantiatingJavaObjects)
+ (id)newWithSignature:(NSString *)signature, ...;
+ (id)newWithSignature:(NSString *)signature arguments:(va_list)args;
@end

extern NSString *NSInvalidJavaClassException;
