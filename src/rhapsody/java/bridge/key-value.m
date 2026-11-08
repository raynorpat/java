/* DR2 libObjCJava key-value access, 0x53d06ce0-0x53d08294. */
#import "key-value.h"
#include "bridge-exceptions.h"
#include "invocation.h"
#include "class-map.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

extern id NSJavaHandleToId(HObject *);
extern HObject *NSIdToJavaHandle(id);
extern HObject *_NSStringToJavaString(id);

/* Methods take precedence; getters use the key itself, not a getKey prefix. */
static struct fieldblock *keyMember(HObject *object, const char *key,
    BOOL setting, BOOL *method, char *kind)
{
    ClassClass *clazz = obj_classblock(object);
    struct methodblock *mb;
    struct fieldblock *fb = NULL;
    unsigned i;
    char *name = NULL;
    if (setting) {
        name = malloc(strlen(key) + 4);
        strcpy(name, "set");
        strcpy(name + 3, key);
        name[3] = toupper((unsigned char)name[3]);
    }
    mb = _JAVAMethodLookup(EE(), clazz, setting ? name : key, NULL, TRUE, FALSE);
    if (name) free(name);
    if (mb && (setting || fieldsig(&mb->fb)[1] == ')')) {
        fb = &mb->fb;
        *method = YES;
        *kind = fieldsig(fb)[setting ? 1 : 2];
    } else {
        makeslottable(clazz);
        for (i = 0; i < cbSlotTableSize(clazz); ++i) {
            fb = cbSlotTable(clazz)[i];
            if (fb && !strcmp(fieldname(fb), key)) break;
        }
        if (i == cbSlotTableSize(clazz)) return NULL;
        *method = NO;
        *kind = fieldsig(fb)[0];
    }
    return strchr("BCISZFDJL", *kind) ? fb : NULL;
}

BOOL NSJavaValueForKey(HObject *object, NSString *key, id *value)
{
    struct fieldblock *fb;
    BOOL method;
    char kind, *address;
    union { long words[2]; long long integer; double real; float small; HObject *object; } bits;
    *value = nil;
    if (!object) [NSJavaException raise:@"java/lang/NullPointerException"
        format:@"attempt to get key %@ of null object", key];
    fb = keyMember(object, [key cString], NO, &method, &kind);
    if (!fb) return NO;
    if (method) {
        bits.words[0] = __JAVAMethodInvoke(EE(), object, (struct methodblock *)fb,
            FALSE, FALSE, NULL, &bits.words[1]);
        /* The invocation ABI returns the high word and writes the low word. */
        if (kind == 'D' || kind == 'J') {
            long word = bits.words[0]; bits.words[0] = bits.words[1]; bits.words[1] = word;
        }
        if (kind == 'B' || kind == 'Z') bits.words[0] = (signed char)bits.words[0];
        if (kind == 'C' || kind == 'S') bits.words[0] = (short)bits.words[0];
    } else {
        address = (char *)unhand(object) + fb->u.offset;
        memcpy(&bits, address, kind == 'D' || kind == 'J' ? 8 : 4);
    }
    switch (kind) {
    case 'L': *value = NSJavaHandleToId(bits.object); break;
    case 'F': *value = [NSNumber numberWithDouble:bits.small]; break;
    case 'D': *value = [NSNumber numberWithDouble:bits.real]; break;
    case 'J': *value = [NSNumber numberWithLongLong:bits.integer]; break;
    default: *value = [NSNumber numberWithInt:bits.words[0]]; break;
    }
    return YES;
}

static HObject *objectValue(id value, const char *signature, ClassClass **expected)
{
    const char *end = strchr(signature + 1, ';');
    char *name = malloc(end - signature);
    ClassClass *number;
    HObject *result;
    memcpy(name, signature + 1, end - signature - 1);
    name[end - signature - 1] = 0;
    *expected = NSJavaLookupClassName(name);
    free(name);
    if (![value isKindOfClass:[NSString class]]) return NSIdToJavaHandle(value);
    number = FindClass(EE(), "java/lang/Number", TRUE);
    if (*expected == number) {
        double real = [value doubleValue];
        int integer = [value intValue];
        if (real == integer)
            return JAVANewObject(EE(), FindClass(EE(), "java/lang/Integer", TRUE),
                "(I)", FALSE, (va_list)&integer);
        return JAVANewObject(EE(), FindClass(EE(), "java/lang/Double", TRUE),
            "(D)", FALSE, (va_list)&real);
    }
    result = _NSStringToJavaString(value);
    if (is_subclass_of(*expected, number, EE()))
        result = JAVANewObject(EE(), *expected, "(Ljava/lang/String;)", FALSE, (va_list)&result);
    return result;
}

BOOL NSJavaTakeValueForKey(HObject *object, id value, NSString *key)
{
    struct fieldblock *fb;
    ClassClass *expected;
    BOOL method;
    char kind;
    union { long integer; long long wide; double real; float small; HObject *object; } bits;
    unsigned size = 4;
    if (!object) [NSJavaException raise:@"java/lang/NullPointerException"
        format:@"attempt to set key %@ of null object", key];
    fb = keyMember(object, [key cString], YES, &method, &kind);
    if (!fb) return NO;
    switch (kind) {
    case 'L':
        bits.object = objectValue(value, fieldsig(fb) + (method ? 1 : 0), &expected);
        if (bits.object && !is_instance_of((JHandle *)bits.object, expected, EE()))
            [NSJavaException raise:@"java/lang/IllegalAccessError"
                format:@"attempt to set %s::%s %s with object of type %s",
                cbName(obj_classblock(object)), cbName(expected), [key cString],
                cbName(obj_classblock(bits.object))];
        break;
    case 'J': bits.wide = [value longLongValue]; size = 8; break;
    case 'D': bits.real = [value doubleValue]; size = 8; break;
    case 'F':
        if (method) { bits.real = [value doubleValue]; size = 8; }
        else bits.small = [value doubleValue];
        break;
    default: bits.integer = [value intValue]; break;
    }
    if (method)
        __JAVAMethodInvoke(EE(), object, (struct methodblock *)fb, FALSE, FALSE,
            (va_list)&bits, NULL);
    else memcpy((char *)unhand(object) + fb->u.offset, &bits, size);
    return YES;
}
