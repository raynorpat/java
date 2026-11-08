#import "objects.h"
#import "classgen.h"
#import "dispatch.h"
#import "bridge-exceptions.h"
#include "tree.h"
#include <stdlib.h>
#include <string.h>

/* Reflection metadata ABI. Each array and each string belongs to the caller. */
BOOL _BRIDGEFieldGoopForJavaClass(ExecEnv *env, ClassClass *clazz,
    unsigned *count, unsigned **flags, char ***names, char ***types)
{
    unsigned i, n = cbFieldsCount(clazz);
    *count = n; *flags = malloc(n * sizeof(**flags));
    *names = malloc(n * sizeof(**names)); *types = malloc(n * sizeof(**types));
    for (i = 0; i < n; ++i) {
        struct fieldblock *field = cbFields(clazz) + i;
        (*flags)[i] = field->access & ACC_WRITTEN_FLAGS;
        (*names)[i] = strdup(fieldname(field)); (*types)[i] = strdup(fieldsig(field));
    }
    return YES;
}
static BOOL methodGoop(ClassClass *clazz, BOOL constructors, unsigned *count,
    unsigned **flags, char ***names, char ***types)
{
    unsigned i, n = 0;
    for (i = 0; i < cbMethodsCount(clazz); ++i) {
        const char *name = fieldname(&cbMethods(clazz)[i].fb);
        if (constructors ? !strcmp(name,"<init>") : name[0] != '<') ++n;
    }
    *count = n; *flags = malloc(n * sizeof(**flags));
    if (names) *names = malloc(n * sizeof(**names));
    *types = malloc(n * sizeof(**types)); n = 0;
    for (i = 0; i < cbMethodsCount(clazz); ++i) {
        struct fieldblock *method = &cbMethods(clazz)[i].fb;
        const char *name = fieldname(method);
        if (!(constructors ? !strcmp(name,"<init>") : name[0] != '<')) continue;
        (*flags)[n] = method->access & ACC_WRITTEN_FLAGS;
        if (names) (*names)[n] = strdup(name);
        (*types)[n++] = strdup(fieldsig(method));
    }
    return YES;
}
BOOL _BRIDGEMethodGoopForJavaClass(ExecEnv *env, ClassClass *clazz,
    unsigned *count, unsigned **flags, char ***names, char ***types)
{ return methodGoop(clazz, NO, count, flags, names, types); }
BOOL _BRIDGEConstructorGoopForJavaClass(ExecEnv *env, ClassClass *clazz,
    unsigned *count, unsigned **flags, char ***types)
{ return methodGoop(clazz, YES, count, flags, NULL, types); }
BOOL _BRIDGEInterestingGoopForJavaClass(ExecEnv *env, ClassClass *clazz,
    unsigned *fc, unsigned **ff, char ***fn, char ***ft,
    unsigned *mc, unsigned **mf, char ***mn, char ***mt,
    unsigned *cc, unsigned **cf, char ***ct)
{
    return _BRIDGEFieldGoopForJavaClass(env,clazz,fc,ff,fn,ft)
        && _BRIDGEMethodGoopForJavaClass(env,clazz,mc,mf,mn,mt)
        && _BRIDGEConstructorGoopForJavaClass(env,clazz,cc,cf,ct);
}
Class _BRIDGECreateObjectiveCClassForJavaClass(ClassClass *clazz, const char *name)
{
    Class result = bridgeCreateNamedJavaClass(name, clazz,
        NSJavaClassToObjcClass(cbSuperclass(clazz), YES), bridgeImplementationForMethod);
    return result;
}
stack_item *Java_com_apple_yellow_foundation_NSRuntime_nextRootPath_stub(stack_item *args, ExecEnv *env)
{
    const char *root = getenv("NEXT_ROOT");
    args[0].h = NSIdToJavaHandle([[NSString allocWithZone:NULL] initWithCString:root ? root : ""]);
    return args + 1;
}
