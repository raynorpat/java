/* DR2 libObjCJava numeric and selector value callbacks. */
#import "value-conversions.h"
#import "class-map.h"
#import "selectors.h"
#import "bridge-exceptions.h"
#import "diagnostics.h"
#include "javaString.h"
#include <stdlib.h>
#include <string.h>

static ClassClass *integerClass, *floatClass, *doubleClass, *longClass;
static ClassClass *objectiveCSelectorClass, *nsObjectiveCSelectorClass;

id _JavaIntegerToNSNumber(HObject *number)
{
    int value;
    memcpy(&value, unhand(number), sizeof(value));
    return [NSNumber numberWithInt:value];
}
id _JavaFloatToNSNumber(HObject *number)
{
    float value;
    memcpy(&value, unhand(number), sizeof(value));
    return [NSNumber numberWithFloat:value];
}
id _JavaDoubleToNSNumber(HObject *number)
{
    double value;
    memcpy(&value, unhand(number), sizeof(value));
    return [NSNumber numberWithDouble:value];
}
id _JavaLongToNSNumber(HObject *number)
{
    long long value;
    memcpy(&value, unhand(number), sizeof(value));
    return [NSNumber numberWithLongLong:value];
}

HObject *_NSNumberToJavaNumber(id number)
{
    char kind;
    if (!number) return NULL;
    kind = *[number objCType];
    switch (kind) {
    case 'J': case 'q':
        return execute_java_constructor(NULL, "java/lang/Long", NULL,
            "(J)", [number longLongValue]);
    case 'f':
        return execute_java_constructor(NULL, "java/lang/Float", NULL,
            "(D)", (double)[number floatValue]);
    case 'd':
        return execute_java_constructor(NULL, "java/lang/Double", NULL,
            "(D)", [number doubleValue]);
    default:
        if (!strchr("CILScils", kind))
            NSLog(@"ObjCJava: invalid ObjC type '%c' for number %@", kind, number);
        return execute_java_constructor(NULL, "java/lang/Integer", NULL,
            "(I)", [number intValue]);
    }
}

id _JavaNumberToNSNumber(HObject *number)
{
    ClassClass *clazz;
    union { int integer; float small; double real; long long wide; } value;
    if (!number) return nil;
    clazz = obj_classblock(number);
    if (!integerClass) {
        integerClass = FindClass(EE(), "java/lang/Integer", TRUE);
        floatClass = FindClass(EE(), "java/lang/Float", TRUE);
        doubleClass = FindClass(EE(), "java/lang/Double", TRUE);
        longClass = FindClass(EE(), "java/lang/Long", TRUE);
    }
    if (!integerClass) {
        NSLog(@"ObjCJava: can't find the java.lang.Number classes");
        return nil;
    }
    if (clazz == integerClass) {
        memcpy(&value.integer, unhand(number), 4);
        return [NSNumber numberWithInt:value.integer];
    }
    if (clazz == floatClass) {
        memcpy(&value.small, unhand(number), 4);
        return [NSNumber numberWithFloat:value.small];
    }
    if (clazz == doubleClass) {
        memcpy(&value.real, unhand(number), 8);
        return [NSNumber numberWithDouble:value.real];
    }
    if (clazz == longClass) {
        memcpy(&value.wide, unhand(number), 8);
        return [NSNumber numberWithLongLong:value.wide];
    }
    NSLog(@"ObjCJava: unknown java number class %s", cbName(clazz));
    return nil;
}

SEL _NSConvertSelFromJava(HObject *selector)
{
    HObject **fields;
    Hjava_lang_String *name;
    char *text;
    unsigned length, arguments;
    SEL result;
    if (!selector) return NULL;
    fields = (HObject **)unhand(selector);
    name = (Hjava_lang_String *)fields[0];
    if (!name) return NULL;
    length = javaStringLength(name);
    if (is_instance_of((JHandle *)selector, objectiveCSelectorClass, EE())) {
        if (!length) return NULL;
        text = malloc(length + 1);
        result = javaString2CString(name, text, length + 1) ? sel_registerName(text) : NULL;
    } else {
        arguments = fields[1] ? obj_length(fields[1]) : 0;
        text = malloc(length + arguments + 1);
        result = NULL;
        if (javaString2CString(name, text, length + 1)) {
            result = NSLookupJavaMethodName(text, NULL);
            if (!result) {
                memset(text + length, ':', arguments);
                text[length + arguments] = 0;
                result = sel_registerName(text);
            }
        }
    }
    free(text);
    return result;
}

HObject *_NSConvertSelToJava(SEL selector)
{
    Hjava_lang_String *name;
    if (!selector) return NULL;
    name = makeJavaString((char *)sel_getName(selector), strlen(sel_getName(selector)));
    return JAVANewObject(EE(), nsObjectiveCSelectorClass, "(Ljava/lang/String;)",
        FALSE, (va_list)&name);
}

HObject *_NSConvertSelToJavaFromStack(SEL *selector, unsigned *size)
{
    *size = sizeof(SEL);
    return selector ? _NSConvertSelToJava(*selector) : NULL;
}

static ClassClass *selectorClass(const char *name)
{
    ClassClass *clazz = FindClass(EE(), (char *)name, TRUE);
    if (!clazz) {
        NSString *reason = [NSString stringWithFormat:@"ObjCJava: can't find %s", name];
        _BRIDGEComplain(0, reason);
        [NSException raise:NSInternalInconsistencyException format:@"%@", reason];
    }
    return clazz;
}

void SelectorInitialization(void)
{
    bridgeInitializeSelectorMaps();
    objectiveCSelectorClass = selectorClass("com/apple/alpha/core/ObjectiveCSelector");
    nsObjectiveCSelectorClass = selectorClass("com/apple/yellow/foundation/_private/NSObjectiveCSelector");
    selectorClass("com/apple/alpha/core/Selector");
    BRIDGESetJavaTypeMapping("com/apple/alpha/core/ObjectiveCSelector",
        _NSConvertSelFromJava, _NSConvertSelToJavaFromStack, 0);
    BRIDGESetJavaTypeMapping("com/apple/alpha/core/Selector",
        _NSConvertSelFromJava, _NSConvertSelToJavaFromStack, 0);
}
