#ifndef JDK115_BRIDGE_EXCEPTIONS_H
#define JDK115_BRIDGE_EXCEPTIONS_H
#import <Foundation/Foundation.h>
#include "oobj.h"
#include "interpreter.h"

@interface NSJavaException : NSException {
    HObject *_javaException;
}
- initWithName:(NSString *)name reason:(NSString *)reason
    userInfo:(NSDictionary *)info javaException:(HObject *)exception;
- (HObject *)javaException;
- (NSString *)stackTrace;
@end

void ExceptionInitialization(void);
NSException *_JavaExceptionToNSException(HObject *exception);
HObject *_NSExceptionToJavaException(NSException *exception);
HObject *_NSRaiseExceptionInJava(ExecEnv *env, NSException *exception);
void bridgeRaisePendingJavaException(ExecEnv *env);
HObject *NSRetainJavaObject(HObject *object);
void NSReleaseJavaObject(HObject *object);
HObject *JAVANewObject(ExecEnv *env, ClassClass *clazz, const char *signature,
    bool_t shortFloats, va_list args);
long __JAVAMethodInvoke(ExecEnv *env, HObject *target, struct methodblock *method,
    bool_t isStatic, bool_t shortFloats, va_list args, long *highBits);
#endif
