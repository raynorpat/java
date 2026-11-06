/* Native half of NativeHello, written against the JDK 1.0 native interface. */
#include "NativeHello.h"
#include "javaString.h"

long NativeHello_add(struct HNativeHello *self, long a, long b)
{
    return a + b;
}

struct Hjava_lang_String *NativeHello_greet(struct HNativeHello *unused, struct Hjava_lang_String *who)
{
    char buf[64];
    char name[32];

    javaString2CString(who, name, sizeof name);
    sprintf(buf, "hello %s", name);
    return makeJavaString(buf, strlen(buf));
}
