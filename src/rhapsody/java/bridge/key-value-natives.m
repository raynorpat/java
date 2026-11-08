/* DR2 alpha.core.keyValue stack natives, 0x53d0777c-0x53d07f70. */
#import "key-value.h"
#import "bridge-exceptions.h"
#import "bootstrap.h"
#include "javaString.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static stack_item *keyAccess(stack_item *args, ExecEnv *env, int group, BOOL setting)
{
    HObject *object = args[0].h;
    struct fieldblock *fb;
    BOOL method;
    int actual, state = 0;
    char key[256], error[512], kind;
    union { long words[2]; long long wide; double real; float small; HObject *object; } bits;
    if (!object) { SignalError(env, "java/lang/NullPointerException", "get value for null object"); return args + (setting ? 0 : group == 4 ? 2 : 1); }
    javaString2CString((Hjava_lang_String *)args[1].h, key, sizeof(key));
    actual = NSHasKey(object, key, setting, &fb, &method);
    if (!group) { args[0].i = actual != 0; return args + 1; }
    if (actual != group) {
        sprintf(error, "%s has no %s key named %s", cbName(obj_classblock(object)),
            group == 1 ? "object" : group == 2 ? (setting ? "int" : "integer") : group == 4 || setting ? "long" : "float", key);
        SignalError(env, "java/lang/IllegalArgumentException", error);
        return args + (setting ? 0 : group == 4 ? 2 : 1);
    }
    kind = fieldsig(fb)[method ? (setting ? 1 : 2) : 0];
    if (setting && group == 1) state = _NSJavaStubEnter();
    NS_DURING
        if (setting) {
            bits.words[0] = args[2].i; if (group == 4) bits.words[1] = args[3].i;
            if (group == 1 && bits.object) {
                const char *signature = fieldsig(fb) + (method ? 1 : 0);
                const char *end = strchr(signature, ';');
                char *name = malloc(end - signature);
                ClassClass *expected;
                memcpy(name, signature + 1, end - signature - 1); name[end - signature - 1] = 0;
                expected = FindClass(env, name, TRUE); free(name);
                if (!is_instance_of((JHandle *)bits.object, expected, env))
                    [NSJavaException raise:@"java/lang/IllegalAccessError" format:@"attempt to set %s::%s %s with object of type %s", cbName(obj_classblock(object)), cbName(expected), key, cbName(obj_classblock(bits.object))];
            }
            if (group == 3 && (kind == 'D' || method)) bits.real = args[2].f;
            if (method) __JAVAMethodInvoke(env, object, (struct methodblock *)fb, FALSE, FALSE, (va_list)&bits, NULL);
            else memcpy((char *)unhand(object) + fb->u.offset, &bits, kind == 'D' || kind == 'J' ? 8 : 4);
        } else {
            if (method) {
                bits.words[1] = __JAVAMethodInvoke(env, object, (struct methodblock *)fb, FALSE, FALSE, NULL, &bits.words[0]);
                if (group != 4 && kind != 'D') bits.words[0] = bits.words[1];
                if (kind == 'B' || kind == 'Z') bits.words[0] = (signed char)bits.words[0];
                if (kind == 'C' || kind == 'S') bits.words[0] = (short)bits.words[0];
            } else memcpy(&bits, (char *)unhand(object) + fb->u.offset, kind == 'D' || kind == 'J' ? 8 : 4);
            if (group == 3) args[0].f = kind == 'D' ? bits.real : bits.small;
            else { args[0].i = bits.words[0]; if (group == 4) args[1].i = bits.words[1]; }
        }
    NS_HANDLER
        _NSRaiseExceptionInJava(env, localException);
    NS_ENDHANDLER
    if (setting && group == 1) _NSJavaStubExit(state);
    return args + (setting ? 0 : group == 4 ? 2 : 1);
}
#define KEY_NATIVE(name,group,setting) stack_item *Java_com_apple_alpha_core_keyValue_##name##_stub(stack_item *args, ExecEnv *env) { return keyAccess(args, env, group, setting); }
KEY_NATIVE(hasKey,0,NO)
KEY_NATIVE(getValue,1,NO)
KEY_NATIVE(getIntValue,2,NO)
KEY_NATIVE(getLongValue,4,NO)
KEY_NATIVE(getFloatValue,3,NO)
KEY_NATIVE(setValue,1,YES)
KEY_NATIVE(setIntValue,2,YES)
KEY_NATIVE(setLongValue,4,YES)
KEY_NATIVE(setFloatValue,3,YES)
