/* Real VM helper checks; SUN_ORACLE calls the primary DLL's private RVAs. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "javaString.h"
#include "jni.h"
#ifdef SUN_ORACLE
#include <windows.h>
#else
extern char *prof_file;
extern void javamon(int);
extern void java_mon_dump(void);
#endif

static HString *(*new_utf)(char *);
static int (*utf_length)(HString *);
static char *(*to_utf)(HString *, char *, int);
static HString *(*from_platform)(char *, int);
static char *(*to_platform)(HString *);
static int (*string_length)(HString *);
static unicode *(*to_unicode)(HString *, unicode *, int);

static void hexbytes(char *label, char *bytes, int count)
{
    int i;
    printf("%s=", label);
    for (i = 0; i < count; ++i) printf("%02x", (unsigned char)bytes[i]);
    printf("\n");
}

int main(int argc, char **argv)
{
    JavaVM *vm;
    JNIEnv *env;
    JDK1_1InitArgs args;
    char property[128], *properties[2], buffer[32], *allocated;
    HString *text, *platform;
    HString slice;
    Classjava_lang_String slice_body;
    unicode chars[8];
    jint (JNICALL *get_args)(void *);
    jint (JNICALL *create_vm)(JavaVM **, JNIEnv **, void *);
    int count, i;
    char **profile_filename;
    void (*enable_profile)(int);
    void (*dump_profile)(void);
#ifdef SUN_ORACLE
    HMODULE dll;
#endif
    if (argc != 5) return 2;
#ifdef SUN_ORACLE
    dll = LoadLibraryA(argv[1]);
    if (!dll) return 3;
    get_args = (void *)GetProcAddress(dll, "JNI_GetDefaultJavaVMInitArgs");
    create_vm = (void *)GetProcAddress(dll, "JNI_CreateJavaVM");
    new_utf = (void *)((char *)dll + 0x4b3b1);
    utf_length = (void *)((char *)dll + 0x4b283);
    to_utf = (void *)((char *)dll + 0x4b30d);
    from_platform = (void *)((char *)dll + 0x4aee0);
    to_platform = (void *)((char *)dll + 0x4b10c);
    string_length = (void *)GetProcAddress(dll, "javaStringLength");
    to_unicode = (void *)GetProcAddress(dll, "javaString2unicode");
    profile_filename = (char **)((char *)dll + 0x71878);
    enable_profile = (void *)((char *)dll + 0x398d5);
    dump_profile = (void *)((char *)dll + 0x39a31);
#else
    get_args = JNI_GetDefaultJavaVMInitArgs;
    create_vm = JNI_CreateJavaVM;
    new_utf = makeJavaStringUTF;
    utf_length = javaStringUTFLength;
    to_utf = javaString2UTF;
    from_platform = makeJavaStringFromPlatformCString;
    to_platform = makePlatformCString;
    string_length = javaStringLength;
    to_unicode = javaString2unicode;
    profile_filename = &prof_file;
    enable_profile = javamon;
    dump_profile = java_mon_dump;
#endif
    if (get_args(&args)) return 4;
    args.classpath = argv[2];
    sprintf(property, "file.encoding=%s", argv[3]);
    properties[0] = property; properties[1] = NULL;
    args.properties = properties;
    if (create_vm(&vm, &env, &args)) return 5;
#ifdef SUN_ORACLE
    /* Oracle-only setup: Sun's invocation startup already caches 8859_1
     * before applying properties. Reset to exercise helper first-use branches. */
    *(char **)((char *)dll + 0x7ee64) = NULL;
#endif
    {
        jclass system = (*env)->FindClass(env, "java/lang/System");
        jmethodID get = (*env)->GetStaticMethodID(env, system, "getProperty",
            "(Ljava/lang/String;)Ljava/lang/String;");
        jstring key = (*env)->NewStringUTF(env, "file.encoding");
        jstring value = (jstring)(*env)->CallStaticObjectMethod(env, system, get, key);
        const char *value_bytes = (*env)->GetStringUTFChars(env, value, NULL);
        printf("property=%s\n", value_bytes);
        (*env)->ReleaseStringUTFChars(env, value, value_bytes);
    }
    buffer[0] = 'x';
    to_utf(NULL, buffer, sizeof(buffer));
    printf("null=%d/%d\n", utf_length(NULL), buffer[0]);
    text = new_utf("A\xc0\x80\xc3\xa9\xe2\x82\xac");
    if (!text) return 6;
    printf("utf=%d/%d\n", string_length(text), utf_length(text));
    allocated = to_utf(text, NULL, 0);
    hexbytes("bytes", allocated, utf_length(text));
    /* Oracle memory is owned by its CRT; deliberately retain this small buffer. */
    to_utf(text, buffer, 5);
    hexbytes("bounded", buffer, strlen(buffer));
    slice = *text; slice_body = *unhand(text);
    slice.obj = &slice_body; slice_body.offset = 1; slice_body.count = 2;
    to_utf(&slice, buffer, sizeof(buffer));
    hexbytes("slice", buffer, utf_length(&slice));
    platform = from_platform("\x80\xe9", 2);
    if (!platform) return 7;
    count = string_length(platform);
    to_unicode(platform, chars, count);
    printf("platform=");
    for (i = 0; i < count; ++i) printf("%04x", chars[i]);
    printf("\n");
    platform = new_utf("\xe2\x82\xac");
    allocated = to_platform(platform);
    if (!allocated) return 8;
    hexbytes("encoded", allocated, strlen(allocated));
    if ((*env)->ExceptionOccurred(env)) { (*env)->ExceptionDescribe(env); return 9; }
    {
        FILE *report;
        remove(argv[4]);
        *profile_filename = argv[4];
        enable_profile(1);
        dump_profile();
        report = fopen(argv[4], "r");
        if (!report || fgetc(report) == EOF) return 10;
        fclose(report);
        remove(argv[4]);
        printf("profile=custom-path\n");
    }
    (*vm)->DestroyJavaVM(vm);
    return 0;
}
