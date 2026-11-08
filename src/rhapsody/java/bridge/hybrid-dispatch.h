#ifndef JDK115_BRIDGE_HYBRID_DISPATCH_H
#define JDK115_BRIDGE_HYBRID_DISPATCH_H
#import <Foundation/Foundation.h>
#import <objc/objc-runtime.h>
id JavaCopyImp(id object, SEL selector);
id JavaRetainImp(id object, SEL selector);
void JavaReleaseImp(id object, SEL selector);
#endif
