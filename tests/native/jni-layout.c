#include <stddef.h>
#include "jni.h"

/* JDK 1.1.5 is a 32-bit ABI, including its invocation structures. */
#define CHECK(name, expression) typedef char name[(expression) ? 1 : -1]
CHECK(pointer_width, sizeof(void *) == 4);
CHECK(jbyte_width, sizeof(jbyte) == 1);
CHECK(jbyte_signed, (jbyte)-1 < 0);
CHECK(jboolean_width, sizeof(jboolean) == 1);
CHECK(jchar_width, sizeof(jchar) == 2);
CHECK(jshort_width, sizeof(jshort) == 2);
CHECK(jint_width, sizeof(jint) == 4);
CHECK(jlong_width, sizeof(jlong) == 8);
CHECK(jfloat_width, sizeof(jfloat) == 4);
CHECK(jdouble_width, sizeof(jdouble) == 8);
CHECK(init_args_size, sizeof(JDK1_1InitArgs) == 72);
CHECK(java_stack_offset, offsetof(JDK1_1InitArgs, javaStackSize) == 16);
CHECK(class_gc_offset, offsetof(JDK1_1InitArgs, enableClassGC) == 48);
CHECK(debugging_offset, offsetof(JDK1_1InitArgs, debugging) == 64);
CHECK(debug_port_offset, offsetof(JDK1_1InitArgs, debugPort) == 68);
CHECK(invoke_table_size, sizeof(struct JNIInvokeInterface_) == 24);
CHECK(attach_offset, offsetof(struct JNIInvokeInterface_, AttachCurrentThread) == 16);

int main(void)
{
    return 0;
}
