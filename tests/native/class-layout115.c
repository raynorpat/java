/* Captured from Sun JDK 1.1.5 Win32 headers using MSVC x86.
 * See class-layout115-{release,debug}.layout and class allocation evidence.
 * Compile separately with no JCOV, and with DEBUG+JCOV. Intentionally not
 * registered until Task 3 adopts the ABI; current legacy headers must fail.
 */
#include <stddef.h>
#include "oobj.h"
#include "interpreter.h"
#define JOIN_(a,b) a##b
#define JOIN(a,b) JOIN_(a,b)
#define CHECK(e) typedef char JOIN(abi_check_,__LINE__)[(e) ? 1 : -1]
#define SIZE(t,n) CHECK(sizeof(t) == (n))
#define OFFSET(t,f,n) CHECK(offsetof(t,f) == (n))

/* Conflicting declarations are a hard compile error if ClassClass is body. */
static ClassClass *class_identity(Hjava_lang_Class *value);
static Hjava_lang_Class *class_identity(ClassClass *value) { return value; }

SIZE(ClassClass, 8);
SIZE(Hjava_lang_Class, 8);
#ifdef JCOV
SIZE(Classjava_lang_Class, 112);
#else
SIZE(Classjava_lang_Class, 100);
#endif
OFFSET(Classjava_lang_Class, major_version, 0);
OFFSET(Classjava_lang_Class, minor_version, 2);
OFFSET(Classjava_lang_Class, name, 4);
OFFSET(Classjava_lang_Class, super_name, 8);
OFFSET(Classjava_lang_Class, source_name, 12);
OFFSET(Classjava_lang_Class, superclass, 16);
OFFSET(Classjava_lang_Class, HandleToSelf, 20);
OFFSET(Classjava_lang_Class, loader, 24);
OFFSET(Classjava_lang_Class, finalizer, 28);
OFFSET(Classjava_lang_Class, constantpool, 32);
OFFSET(Classjava_lang_Class, methods, 36);
OFFSET(Classjava_lang_Class, fields, 40);
OFFSET(Classjava_lang_Class, implements, 44);
OFFSET(Classjava_lang_Class, methodtable, 48);
OFFSET(Classjava_lang_Class, methodtable_mem, 52);
OFFSET(Classjava_lang_Class, slottable, 56);
OFFSET(Classjava_lang_Class, classname, 60);
OFFSET(Classjava_lang_Class, hashinfo, 64);
OFFSET(Classjava_lang_Class, constantpool_count, 72);
OFFSET(Classjava_lang_Class, methods_count, 74);
OFFSET(Classjava_lang_Class, fields_count, 76);
OFFSET(Classjava_lang_Class, implements_count, 78);
OFFSET(Classjava_lang_Class, methodtable_size, 80);
OFFSET(Classjava_lang_Class, slottbl_size, 82);
OFFSET(Classjava_lang_Class, instance_size, 84);
OFFSET(Classjava_lang_Class, access, 86);
OFFSET(Classjava_lang_Class, flags, 88);
OFFSET(Classjava_lang_Class, signers, 92);
OFFSET(Classjava_lang_Class, imethodtable, 96);
#ifdef JCOV
SIZE(struct methodblock, 100);
#else
SIZE(struct methodblock, 92);
#endif
OFFSET(struct methodblock, fb, 0);
OFFSET(struct methodblock, code, 24);
OFFSET(struct methodblock, exception_table, 28);
OFFSET(struct methodblock, line_number_table, 32);
OFFSET(struct methodblock, localvar_table, 36);
OFFSET(struct methodblock, code_length, 40);
OFFSET(struct methodblock, exception_table_length, 44);
OFFSET(struct methodblock, line_number_table_length, 48);
OFFSET(struct methodblock, localvar_table_length, 52);
OFFSET(struct methodblock, invoker, 56);
OFFSET(struct methodblock, args_size, 60);
OFFSET(struct methodblock, maxstack, 62);
OFFSET(struct methodblock, nlocals, 64);
OFFSET(struct methodblock, CompiledCode, 68);
OFFSET(struct methodblock, CompiledCodeInfo, 72);
OFFSET(struct methodblock, CompiledCodeFlags, 76);
OFFSET(struct methodblock, inlining, 80);
OFFSET(struct methodblock, nexceptions, 84);
OFFSET(struct methodblock, exceptions, 88);
SIZE(JavaFrame, 44);
OFFSET(JavaFrame, constant_pool, 0);
OFFSET(JavaFrame, returnpc, 4);
OFFSET(JavaFrame, optop, 8);
OFFSET(JavaFrame, vars, 12);
OFFSET(JavaFrame, prev, 16);
OFFSET(JavaFrame, javastack, 20);
OFFSET(JavaFrame, lastpc, 24);
OFFSET(JavaFrame, current_method, 28);
OFFSET(JavaFrame, monitor, 32);
OFFSET(JavaFrame, mon_starttime, 36);
OFFSET(JavaFrame, ostack, 40);
SIZE(ExecEnv, 76);
OFFSET(ExecEnv, initial_stack, 0);
OFFSET(ExecEnv, current_frame, 4);
OFFSET(ExecEnv, thread, 8);
OFFSET(ExecEnv, exceptionKind, 12);
OFFSET(ExecEnv, exception, 16);
OFFSET(ExecEnv, nativeInterface, 20);
OFFSET(ExecEnv, seenclasses, 52);
OFFSET(ExecEnv, alloc_cache, 60);
#ifdef JCOV
OFFSET(Classjava_lang_Class, absolute_source_name, 100);
OFFSET(Classjava_lang_Class, timestamp, 104);
OFFSET(struct methodblock, coverage_table, 92);
OFFSET(struct methodblock, coverage_table_length, 96);
#endif

int main(void)
{
    return class_identity(NULL) != NULL;
}
