#import <Foundation/Foundation.h>
#import <objc/objc-runtime.h>

void bridgeInitializeSelectorMaps(void);
void NSSetJavaMethodForObjcSelector(const char *name, SEL selector);
void NSSetOverloadedJavaMethodForObjcSelector(const char *name,
    const char *signature, SEL selector);
SEL NSLookupJavaMethodName(const char *name, const char *signature);
SEL NSJavaMethodNameToSelector(const char *name, const char *signature,
    Class superclass);
BOOL NSGetJavaMethodForSelector(SEL selector, const char **name,
    const char **signature);
