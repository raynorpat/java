#include <jni.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
JNIEXPORT jint JNICALL Java_FrameworkRuntime_callback(JNIEnv *env, jclass clazz, jint value)
{
    jmethodID method = (*env)->GetStaticMethodID(env, clazz, "twice", "(I)I");
    return (*env)->CallStaticIntMethod(env, clazz, method, value);
}
JNIEXPORT jint JNICALL Java_FrameworkRuntime_verifyImages(JNIEnv *env, jclass clazz, jstring root)
{
    const char *prefix = (*env)->GetStringUTFChars(env, root, NULL);
    unsigned i;
    int core = 0, awt = 0, audio = 0, valid = 1;
    for (i = 0; i < _dyld_image_count(); ++i) {
        const char *path = _dyld_get_image_name(i);
        char resolved[PATH_MAX];
        int isCore, isAwt, isAudio, extension;
        if (realpath(path, resolved)) path = resolved;
        isCore = strstr(path, "libjavai.dylib") != NULL;
        isAwt = strstr(path, "libawt.dylib") != NULL;
        isAudio = strstr(path, "libmmedia.dylib") != NULL;
        extension = strstr(path, "libnet.dylib") || strstr(path, "libzip.dylib")
            || strstr(path, "libmath.dylib") || strstr(path, "libjpeg.dylib")
            || strstr(path, "libagent.dylib") || strstr(path, "libsysresource.dylib")
            || strstr(path, "libObjCJava.dylib");
        if (strstr(path, "/usr/lib/java/") || strstr(path, "/System/Library/Frameworks/JavaVM.framework")) valid = 0;
        if (isCore || isAwt || isAudio || extension) {
            if (strncmp(path, prefix, strlen(prefix)) || path[strlen(prefix)] != '/') valid = 0;
            core += isCore; awt += isAwt; audio += isAudio;
        }
        if (!valid) fprintf(stderr, "Unexpected Java image: %s\n", path);
    }
    (*env)->ReleaseStringUTFChars(env, root, prefix);
    return valid && core == 1 && awt == 1 && audio == 1 ? 3 : 0;
}
