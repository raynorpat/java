/* DR2 exception conversion and NSJavaException, 0x53d04d04-0x53d05448. */
#import "bridge-exceptions.h"
#import "diagnostics.h"
#include "invocation.h"
#include "constructor.h"
#include "retention.h"
#include "javaString.h"

extern id _JavaStringToNSString(Hjava_lang_String *);
extern Hjava_lang_String *_NSStringToJavaString(id);
extern id NSJavaHandleToId(HObject *);
extern HObject *NSIdToJavaHandle(id);
static ClassClass *nextExceptionClass, *nsExceptionClass;

void ExceptionInitialization(void)
{
    nextExceptionClass = FindClass(EE(), "com/apple/alpha/core/NativeException", TRUE);
    if (!nextExceptionClass)
        _BRIDGEComplain(0, @"can't find exception class %s", "com/apple/alpha/core/NativeException");
    nsExceptionClass = FindClass(EE(), "com/apple/yellow/foundation/NSException", TRUE);
    /* The donor tests NativeException again here. */
    if (!nextExceptionClass)
        _BRIDGEComplain(0, @"can't find exception class %s", "com/apple/yellow/foundation/NSException");
}

@implementation NSJavaException
- initWithName:(NSString *)exceptionName reason:(NSString *)exceptionReason
    userInfo:(NSDictionary *)info javaException:(HObject *)exception
{
    self = [super initWithName:exceptionName reason:exceptionReason userInfo:info];
    _javaException = exception ? NSRetainJavaObject(exception) : NULL;
    return self;
}
- (void)dealloc
{
    if (_javaException) NSReleaseJavaObject(_javaException);
    [super dealloc];
}
- initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    _javaException = NULL;
    return self;
}
- (void)encodeWithCoder:(NSCoder *)coder { [super encodeWithCoder:coder]; }
- (HObject *)javaException { return _javaException; }
- (NSString *)stackTrace
{
    Hjava_lang_String *stackString;
    if (!nextExceptionClass) return @"Not available.";
    stackString = (Hjava_lang_String *)execute_java_static_method(EE(), nextExceptionClass,
        "getStackTrace", "(Ljava/lang/Throwable;)Ljava/lang/String;", _javaException);
    return _JavaStringToNSString(stackString);
}
- (NSString *)description
{
    NSString *exceptionReason = [self reason];
    if (exceptionReason && ![@"" isEqual:exceptionReason])
        return [NSString stringWithFormat:@"%@: %@\nStack Trace:\n%@", [self name], exceptionReason, [self stackTrace]];
    return [NSString stringWithFormat:@"%@\nStack Trace:\n%@", [self name], [self stackTrace]];
}
@end

static HObject *safeInvoke(HObject *object, struct methodblock *method)
{
    HObject *exception;
    long result = __JAVAMethodInvokeNoExceptionConversion(EE(), object, method,
        FALSE, FALSE, NULL, NULL, &exception);
    if (exception) {
        _BRIDGEComplain(0, @"BRIDGEJavaExceptionToNSException: an exception was thrown while trying to convert another Java exception to an NSException.  It's too dangerous to try to print out that exception, since we may loop infinitely.  (Well, not infinitely - just until we run out of stack space.)");
        return NULL;
    }
    return (HObject *)result;
}

NSException *_JavaExceptionToNSException(HObject *exception)
{
    ClassClass *clazz, *parent;
    const char *className;
    NSString * volatile name = nil;
    NSString *reason;
    id volatile info = nil;
    HObject *value;
    struct methodblock *nameMethod, *infoMethod;
    if (!exception) return nil;
    clazz = obj_classblock(exception);
    className = clazz ? cbName(clazz) : "<unknown>";
    value = safeInvoke(exception, _JAVAMethodLookup(EE(), clazz, "getMessage",
        "()Ljava/lang/String;", TRUE, FALSE));
    reason = _JavaStringToNSString((Hjava_lang_String *)value);
    if ([@"*nil*" isEqual:reason]) reason = nil;
    for (parent = clazz; parent && parent != nextExceptionClass; parent = cbSuperclass(parent)) {}
    if (parent) {
        nameMethod = _JAVAMethodLookup(EE(), clazz, "name", "()Ljava/lang/String;", TRUE, FALSE);
        infoMethod = _JAVAMethodLookup(EE(), clazz, "userInfo", "()Ljava/lang/Object;", TRUE, FALSE);
        NS_DURING
            value = safeInvoke(exception, nameMethod);
            if (value) name = _JavaStringToNSString((Hjava_lang_String *)value);
            value = safeInvoke(exception, infoMethod);
            if (value) info = NSJavaHandleToId(value);
        NS_HANDLER
            NSLog(@"ObjCJava: fatal error: exception %@ raised while trying to convert java exception %s to NSException.", localException, className);
        NS_ENDHANDLER
    }
    if (!name) name = [NSString stringWithCString:className];
    return [[[NSJavaException allocWithZone:NULL] initWithName:name reason:reason
        userInfo:info javaException:exception] autorelease];
}

HObject *_NSExceptionToJavaException(NSException *exception)
{
    HObject *original, *args[3];
    NSString *name, *reason;
    id info;
    if (!exception) return NULL;
    name = [exception name]; reason = [exception reason]; info = [exception userInfo];
    if ([exception isKindOfClass:[NSJavaException class]]) {
        original = [(NSJavaException *)exception javaException];
        if (original) return original;
        _BRIDGEComplain(0, @"NSExceptionToJavaException: lost the Java exception of class %@.  Wrapping the exception in a com.apple.yellow.foundation.NSException.", name);
    }
    args[0] = (HObject *)_NSStringToJavaString(name);
    args[1] = (HObject *)_NSStringToJavaString(reason);
    args[2] = NSIdToJavaHandle(info);
    return JAVANewObject(EE(), nsExceptionClass,
        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/Object;)", FALSE, (va_list)args);
}

HObject *_NSRaiseExceptionInJava(ExecEnv *env, NSException *exception)
{
    HObject *object = _NSExceptionToJavaException(exception);
    if (object) exceptionThrow(env, object);
    return object;
}

void bridgeRaisePendingJavaException(ExecEnv *env)
{
    HObject *exception;
    if (!env) env = EE();
    if (!exceptionOccurred(env)) return;
    exception = env->exception.exc;
    exceptionClear(env);
    [_JavaExceptionToNSException(exception) raise];
}

HObject *NSRetainJavaObject(HObject *object)
{
    bridgeRetainJavaObject(object);
    bridgeRaisePendingJavaException(EE());
    return object;
}
void NSReleaseJavaObject(HObject *object)
{
    bridgeReleaseJavaObject(object);
    bridgeRaisePendingJavaException(EE());
}
HObject *JAVANewObject(ExecEnv *env, ClassClass *clazz, const char *signature,
    bool_t shortFloats, va_list args)
{
    HObject *object = bridgeNewJavaObject(env, clazz, signature, shortFloats, args);
    bridgeRaisePendingJavaException(env);
    return object;
}
long __JAVAMethodInvoke(ExecEnv *env, HObject *target, struct methodblock *method,
    bool_t isStatic, bool_t shortFloats, va_list args, long *highBits)
{
    HObject *exception;
    long result = __JAVAMethodInvokeNoExceptionConversion(env, target, method,
        isStatic, shortFloats, args, highBits, &exception);
    if (exception) [_JavaExceptionToNSException(exception) raise];
    return result;
}
