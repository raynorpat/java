/* DR2 NSRuntime native methods, 0x53d09dfc and 0x53d09f48.
 * Search paths are private: no /usr/lib/java donor fallback. */
#import "bootstrap.h"
#import "bridge-exceptions.h"
#import "diagnostics.h"
#include "javaString.h"
#include "typecodes.h"
/* dyld and the VM both declare TRUE/FALSE enumerators. */
#define ENUM_BOOL
enum bool { DYLD_FALSE, DYLD_TRUE };
#include <mach-o/dyld.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Locate this implementation, not another library with a matching basename. */
const char *bridgeLoadedLibraryPath(void)
{
    unsigned i, j;
    unsigned long address = (unsigned long)bridgeLoadedLibraryPath;
    for (i = 0; i < _dyld_image_count(); ++i) {
        const struct mach_header *header = _dyld_get_image_header(i);
        const struct load_command *command = (const struct load_command *)(header + 1);
        long slide = _dyld_get_image_vmaddr_slide(i);
        for (j = 0; j < header->ncmds; ++j) {
            if (command->cmd == LC_SEGMENT) {
                const struct segment_command *segment = (const struct segment_command *)command;
                unsigned long start = segment->vmaddr + slide;
                if (address >= start && address - start < segment->vmsize)
                    return _dyld_get_image_name(i);
            }
            command = (const struct load_command *)((const char *)command + command->cmdsize);
        }
    }
    return NULL;
}
static void *initializerAddress(const char *name)
{
    char *symbol = malloc(strlen(name) + 2);
    void *result = NULL;
    symbol[0] = '_';
    strcpy(symbol + 1, name);
    if (NSIsSymbolNameDefined(symbol)) result = NSAddressOfSymbol(NSLookupAndBindSymbol(symbol));
    free(symbol);
    return result;
}

/* DR2's runtime-registration hook is empty; symbols use the flat namespace. */
void _NSInitializeObjcRuntimeInLibrary(void *library) {}
void *_NSAddressOfFunctionInLibrary(void *library, const char *name)
{
    return initializerAddress(name);
}

stack_item *Java_com_apple_yellow_foundation_NSRuntime_initialSearchPaths_stub(stack_item *stack, ExecEnv *env)
{
    ClassClass *clazz = FindClass(env, "java/lang/String", TRUE);
    const char *paths = getenv("LD_LIBRARY_PATH");
    const char *ownPath = bridgeLoadedLibraryPath();
    const char *slash = ownPath ? strrchr(ownPath, '/') : NULL;
    const char *cursor, *end;
    HArrayOfObject *array;
    unsigned count = 1, index = 1;
    if (!clazz) {
        SignalError(env, "java/lang/NullPointerException", "can't find java.lang.String class");
        stack[0].p = NULL;
        return stack + 1;
    }
    if (slash) ++count;
    if (paths && *paths) {
        ++count;
        for (cursor = paths; *cursor; ++cursor) if (*cursor == ':') ++count;
    }
    array = (HArrayOfObject *)ArrayAlloc(T_CLASS, count);
    if (!array) { SignalError(env, "java/lang/OutOfMemoryError", NULL); return stack + 1; }
    unhand(array)->body[count] = (HObject *)clazz;
    unhand(array)->body[0] = (HObject *)makeJavaString("./", 2);
    if (slash) unhand(array)->body[index++] = (HObject *)makeJavaString((char *)ownPath, slash - ownPath + 1);
    cursor = paths;
    while (cursor && *cursor) {
        char *path;
        unsigned length;
        end = strchr(cursor, ':');
        length = end ? end - cursor : strlen(cursor);
        path = malloc(length + 3);
        memcpy(path, cursor, length);
        if (!length) path[length++] = '.';
        if (path[length - 1] != '/') path[length++] = '/';
        path[length] = 0;
        unhand(array)->body[index++] = (HObject *)makeJavaString(path, length);
        free(path);
        cursor = end ? end + 1 : NULL;
    }
    /* Empty final search components denote the current directory. */
    while (index < count) unhand(array)->body[index++] = (HObject *)makeJavaString("./", 2);
    stack[0].p = array;
    return stack + 1;
}

stack_item *Java_com_apple_yellow_foundation_NSRuntime_initializeObjcLibrary_stub(stack_item *stack, ExecEnv *env)
{
    Hjava_lang_String *base = (Hjava_lang_String *)stack[0].p;
    Hjava_lang_String *path = (Hjava_lang_String *)stack[1].p;
    unsigned baseLength = javaStringLength(base), pathLength = javaStringLength(path);
    char *baseName = malloc(baseLength + 1), *pathName = malloc(pathLength + 1);
    char *symbol = malloc(baseLength + 16), *cursor;
    void (*initialize)(void);
    int state;
    NSString *banner;
    javaString2CString(base, baseName, baseLength + 1);
    javaString2CString(path, pathName, pathLength + 1);
    sprintf(symbol, "%sInitialization", baseName);
    for (cursor = symbol; *cursor; ++cursor) if (*cursor == '.') *cursor = '_';
    state = _NSJavaStubEnter();
    banner = [[NSString alloc] initWithCString:pathName];
    _BRIDGEResetComplaints(banner);
    [banner release];
    initialize = initializerAddress(symbol);
    if (!initialize) {
        strcpy(symbol, baseName);
        cursor = strrchr(symbol, '.');
        if (cursor) *cursor = 0;
        strcat(symbol, "Initialization");
        initialize = initializerAddress(symbol);
    }
    if (initialize) {
        NS_DURING
            initialize();
        NS_HANDLER
            _BRIDGEComplain(0, @"while trying to initialize wrapper library %s:", pathName);
            _BRIDGEComplain(0, @"caught exception %@: %@", [localException name], [localException reason]);
            _NSRaiseExceptionInJava(env, localException);
        NS_ENDHANDLER
    } else {
        char *message = malloc(strlen(symbol) + 56);
        sprintf(message, "can't find wrapper library initialization function %s", symbol);
        SignalError(env, "java/lang/UnsatisfiedLinkError", message);
        free(message);
    }
    _BRIDGEResetComplaints(nil);
    free(symbol);
    free(pathName);
    free(baseName);
    _NSJavaStubExit(state);
    return stack;
}
