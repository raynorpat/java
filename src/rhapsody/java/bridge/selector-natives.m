#import "objects.h"
#import "value-conversions.h"
#import "bridge-exceptions.h"
#include "invocation.h"
#include <string.h>

extern id _JavaStringToNSString(HObject *string);

static HObject *selectorError(ExecEnv *env, const char *type, const char *message)
{
    SignalError(env, (char *)type, (char *)message);
    return NULL;
}

static HObject *invokeSelector(HObject *selectorObject, HObject *targetObject,
    HObject *arguments, ExecEnv *env)
{
    id target = NSJavaHandleToId(targetObject);
    SEL selector = _NSConvertSelFromJava(selectorObject);
    NSMethodSignature *signature = [target methodSignatureForSelector:selector];
    NSInvocation *call;
    unsigned count, i;
    char type;
    union { long word; long long wide; float single; double real; id object; SEL selector; } value;
    if (!signature) return selectorError(env, "java/lang/NullPointerException", "target doesn't implement selector");
    call = [NSInvocation invocationWithMethodSignature:signature];
    if (!call) return selectorError(env, "java/lang/NullPointerException", "can't create NSInvocation to invoke selector");
    [call setTarget:target];
    [call setSelector:selector];
    count = [signature numberOfArguments];
    if (count - 2 != (arguments ? obj_length(arguments) : 0))
        return selectorError(env, "java/lang/IllegalArgumentException", "invalid number of parameters");
    for (i = 2; i < count; ++i) {
        HObject *argument = ((HObject **)unhand(arguments))[i-2];
        const char *name = NULL, *javaSignature = NULL, *error = NULL;
        struct methodblock *method;
        union { long words[2]; double real; } number;
        type = *[signature getArgumentTypeAtIndex:i];
        switch (type) {
        case 'c': case 's': case 'l': name="intValue"; javaSignature="()I"; error="can't get int value from parameter"; break;
        case 'q': name="longValue"; javaSignature="()J"; error="can't get long value from parameter"; break;
        case 'f': name="floatValue"; javaSignature="()F"; error="can't get float value from parameter"; break;
        case 'd': name="doubleValue"; javaSignature="()D"; error="can't get double value from parameter"; break;
        case ':': value.selector = _NSConvertSelFromJava(argument); break;
        case '*': value.object = _JavaStringToNSString(argument); break;
        case '@': value.object = NSJavaHandleToId(argument); break;
        case '{': return selectorError(env,"java/lang/IllegalArgumentException","don't support passing types that are converted to Objective C structures.  Yet.");
        default: return selectorError(env,"java/lang/IllegalArgumentException","invalid/unsupported parameter type");
        }
        if (name) {
            method = _JAVAMethodLookup(env, obj_classblock(argument), name, javaSignature, TRUE, FALSE);
            if (!method) return selectorError(env,"java/lang/IllegalArgumentException",error);
            number.words[0] = 0;
            number.words[1] = __JAVAMethodInvoke(env, argument, method, FALSE, FALSE, NULL, &number.words[0]);
            /* The donor interprets every numeric accessor result as a double. */
            if (type == 'q') value.wide = (long long)number.real;
            else if (type == 'f') value.single = (float)number.real;
            else if (type == 'd') value.real = number.real;
            else value.word = (long)number.real;
        }
        [call setArgument:&value atIndex:i];
    }
    [call invoke];
    type = *[signature methodReturnType];
    if (!type || type == 'v') return NULL;
    /* Reject aggregates before copying their payload into scalar storage. */
    if (type == '{') return selectorError(env,"java/lang/IllegalArgumentException","don't support returning types that are converted from Objective C structures.  Yet.");
    [call getReturnValue:&value];
    switch (type) {
    case 'c': return NSIdToJavaHandle([NSNumber numberWithChar:(char)value.word]);
    case 's': return NSIdToJavaHandle([NSNumber numberWithShort:(short)value.word]);
    case 'l': return NSIdToJavaHandle([NSNumber numberWithLong:value.word]);
    case 'q': return NSIdToJavaHandle([NSNumber numberWithLongLong:value.wide]);
    case 'f': return NSIdToJavaHandle([NSNumber numberWithFloat:value.single]);
    case 'd': return NSIdToJavaHandle([NSNumber numberWithDouble:value.real]);
    case '@': case ':': return NSIdToJavaHandle(value.object);
    default: return selectorError(env,"java/lang/IllegalArgumentException","invalid/unsupported return type.  The method has been invoked, however.");
    }
}

stack_item *Java_com_apple_alpha_core_ObjectiveCSelector_invokeSelector_stub(stack_item *stack, ExecEnv *env)
{
    NS_DURING
        HObject *result = invokeSelector(stack[0].p, stack[1].p, stack[2].p, env);
        if (!exceptionOccurred(env)) stack[0].p = result;
    NS_HANDLER
        _NSRaiseExceptionInJava(env, localException);
    NS_ENDHANDLER
    return stack + 1;
}

stack_item *Java_com_apple_alpha_core_ObjectiveCSelector_selectorImplementedByClass_stub(stack_item *stack, ExecEnv *env)
{
    stack[0].i = 0;
    return stack + 1;
}
