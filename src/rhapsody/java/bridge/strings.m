/* DR2 libObjCJava string conversion, 0x53d0c3a0 and 0x53d0c4a8. */
#import <Foundation/Foundation.h>
#include "oobj.h"
#include "interpreter.h"
#include "javaString.h"
#include "typecodes.h"

Hjava_lang_String *_NSStringToJavaString(id string)
{
    ExecEnv *env;
    ClassClass *stringClass;
    HArrayOfChar *characters;
    Hjava_lang_String *result;
    if (!string) return NULL;
    env = EE();
    stringClass = FindClass(env, "java/lang/String", TRUE);
    if (!stringClass)
        [NSException raise:NSInternalInconsistencyException
            format:@"NSStringToJavaString: can't find java.lang.String"];
    characters = (HArrayOfChar *)ArrayAlloc(T_CHAR, [string length]);
    if (!characters)
        [NSException raise:NSMallocException format:@"NSStringToJavaString: can't allocate java string"];
    [string getCharacters:(unichar *)unhand(characters)->body];
    result = (Hjava_lang_String *)execute_java_constructor(env, NULL,
        stringClass, "([C)", characters);
    return result;
}

id _JavaStringToNSString(Hjava_lang_String *string)
{
    int length;
    unichar *characters;
    NSString *result;
    if (!string) return nil;
    length = javaStringLength(string);
    if (!length) return @"";
    characters = NSZoneMalloc(NULL, (length + 1) * sizeof(*characters));
    if (!characters)
        [NSException raise:NSMallocException format:@"_JavaStringToNSString: malloc failure"];
    javaString2unicode(string, (unicode *)characters, length);
    result = [[NSString allocWithZone:NULL] initWithCharacters:characters length:length];
    NSZoneFree(NULL, characters);
    return [result autorelease];
}
