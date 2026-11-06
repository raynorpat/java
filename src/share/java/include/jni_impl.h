/* Reconstructed JNI integration interfaces; current ExecEnv/GC adapter. */
#ifndef SUN_JNI_IMPL_H
#define SUN_JNI_IMPL_H
#include "jni.h"
#include "tree.h"
#include "interpreter.h"
JNIEnv *jni_Env(ExecEnv *);
void jni_FreeEnv(ExecEnv *);
void jni_EnumerateRoots(void (*)(void *));
bool_t invokeJNINativeMethod(JHandle *, struct methodblock *, int, ExecEnv *);
bool_t invokeSynchronizedJNINativeMethod(JHandle *, struct methodblock *, int, ExecEnv *);
bool_t dynoLinkJNI(struct methodblock *);
stack_item *sysInvokeNative(JNIEnv *, void *, stack_item *, char *, int, jobject);
extern const struct JNIInvokeInterface_ jni_InvokeInterface;
extern JavaVM jni_vm;
#endif
