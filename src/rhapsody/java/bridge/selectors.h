#import <Foundation/Foundation.h>
#import <objc/objc-runtime.h>

void bridgeInitializeSelectorMaps(void);
SEL JavaMethodNameToObjectiveCSelector(const char *name, int argumentCount, char *buffer, int capacity);
char *ObjectiveCSelectorToJavaMethodName(SEL selector, char *buffer, int capacity);
const char *NSSelectorToJavaMethodName(SEL selector);
void NSSetJavaMethodForObjcSelector(const char *name, SEL selector);
void NSSetOverloadedJavaMethodForObjcSelector(const char *name,
    const char *signature, SEL selector);
SEL NSLookupJavaMethodName(const char *name, const char *signature);
SEL NSJavaMethodNameToSelector(const char *name, const char *signature,
    Class superclass);
BOOL NSGetJavaMethodForSelector(SEL selector, const char **name,
    const char **signature);
