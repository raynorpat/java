/* Oracle-only: private capacity RVA belongs to the pinned Sun Debug DLL. */
#include <windows.h>
#include "jni.h"
JNIEXPORT jint JNICALL Java_MonitorCapacity115_capacity(JNIEnv *env, jclass type)
{
    HMODULE module = GetModuleHandleA("javai_g.dll");
    return module ? *(int *)((char *)module + 0x73ea8) : -1;
}
