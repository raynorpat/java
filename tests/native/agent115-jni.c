/* The same JNI-only probe runs with either original or reconstructed libraries. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jni.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); if(env) (*env)->ExceptionDescribe(env); return 1; } } while(0)
static int exception_is(JNIEnv *env,const char *name)
{
    jthrowable exception=(*env)->ExceptionOccurred(env); jclass clazz;
    if(!exception) return 0;
    (*env)->ExceptionClear(env); clazz=(*env)->FindClass(env,name);
    return clazz && (*env)->IsInstanceOf(env,exception,clazz);
}
int main(int argc,char **argv)
{
    JavaVM *vm; JNIEnv *env=NULL; JDK1_1InitArgs args;
    jint (JNICALL *defaults)(void *); jint (JNICALL *create)(JavaVM **,JNIEnv **,void *);
    jclass access_class,system_class,agent_class,values_class,field_class; jmethodID load_library; jobject agent,values,field;
    jobjectArray fields,methods,classes; jfieldID field_name,field_slot;
    jmethodID get_fields,get_methods,get_classes,get_int,set_int,get_long,set_long,get_double,set_double,get_bool,set_bool,get_object,get_array;
    jmethodID constructor,source,lines,trace,signature; jintArray signature_values; jint tag; int i,slot,intslot=-1; const char *name; jstring string;
#ifdef _WIN32
    HMODULE library; char directory[MAX_PATH],*slash,*search_path; DWORD path_size;
    CHECK(argc==3); strcpy(directory,argv[1]); slash=strrchr(directory,'/'); if(!slash) slash=strrchr(directory,'\\'); CHECK(slash); *slash=0;
    path_size=GetEnvironmentVariableA("PATH",NULL,0);
    search_path=(char *)malloc(strlen(directory)+path_size+2); CHECK(search_path);
    strcpy(search_path,directory); strcat(search_path,";");
    GetEnvironmentVariableA("PATH",search_path+strlen(search_path),path_size);
    CHECK(SetEnvironmentVariableA("PATH",search_path)); CHECK(!_putenv_s("PATH",search_path)); free(search_path);
    CHECK(SetDllDirectoryA(directory)); library=LoadLibraryExA(argv[1],NULL,LOAD_WITH_ALTERED_SEARCH_PATH); CHECK(library);
    defaults=(void*)GetProcAddress(library,"JNI_GetDefaultJavaVMInitArgs"); create=(void*)GetProcAddress(library,"JNI_CreateJavaVM");
#else
    void *library; CHECK(argc==3); library=dlopen(argv[1],RTLD_NOW|RTLD_GLOBAL); CHECK(library);
    defaults=dlsym(library,"JNI_GetDefaultJavaVMInitArgs"); create=dlsym(library,"JNI_CreateJavaVM");
#endif
    CHECK(defaults && create); memset(&args,0,sizeof(args)); args.version=0x10001; CHECK(!defaults(&args)); args.classpath=argv[2]; CHECK(!create(&vm,&env,&args));
    system_class=(*env)->FindClass(env,"java/lang/System"); CHECK(system_class);
    load_library=(*env)->GetStaticMethodID(env,system_class,"loadLibrary","(Ljava/lang/String;)V"); CHECK(load_library);
    (*env)->CallStaticVoidMethod(env,system_class,load_library,(*env)->NewStringUTF(env,"agent")); CHECK(!(*env)->ExceptionOccurred(env));
    agent_class=(*env)->FindClass(env,"sun/tools/debug/Agent"); CHECK(agent_class);
    agent=(*env)->AllocObject(env,agent_class); CHECK(agent);
    values_class=(*env)->FindClass(env,"Agent115Values"); CHECK(values_class);
    constructor=(*env)->GetMethodID(env,values_class,"<init>","()V"); CHECK(constructor);
    values=(*env)->NewObject(env,values_class,constructor); CHECK(values);
#define METHOD(var,name,sig) var=(*env)->GetMethodID(env,agent_class,name,sig); CHECK(var)
    access_class=(*env)->FindClass(env,"sun/tools/debug/Agent115Access"); CHECK(access_class);
    get_fields=(*env)->GetStaticMethodID(env,access_class,"fields","(Ljava/lang/Object;Ljava/lang/Class;)[Ljava/lang/Object;"); CHECK(get_fields);
    get_methods=(*env)->GetStaticMethodID(env,access_class,"methods","(Ljava/lang/Object;Ljava/lang/Class;)[Ljava/lang/Object;"); CHECK(get_methods);
    METHOD(get_classes,"getClasses","()[Ljava/lang/Class;");
    METHOD(get_int,"getSlotInt","(Ljava/lang/Object;I)I"); METHOD(set_int,"setSlotInt","(Ljava/lang/Object;II)V");
    METHOD(get_long,"getSlotLong","(Ljava/lang/Object;I)J"); METHOD(set_long,"setSlotLong","(Ljava/lang/Object;IJ)V");
    METHOD(get_double,"getSlotDouble","(Ljava/lang/Object;I)D"); METHOD(set_double,"setSlotDouble","(Ljava/lang/Object;ID)V");
    METHOD(get_bool,"getSlotBoolean","(Ljava/lang/Object;I)Z"); METHOD(set_bool,"setSlotBoolean","(Ljava/lang/Object;IZ)V");
    METHOD(get_object,"getSlotObject","(Ljava/lang/Object;I)Ljava/lang/Object;");
    METHOD(get_array,"getSlotArray","(Ljava/lang/Object;I)[Ljava/lang/Object;");
    METHOD(signature,"getSlotSignature","(Ljava/lang/Class;I)[I");
    METHOD(source,"getClassSourceName","(Ljava/lang/Class;)Ljava/lang/String;"); METHOD(lines,"getLinenumbers","(Ljava/lang/Class;)[I");
    fields=(jobjectArray)(*env)->CallStaticObjectMethod(env,access_class,get_fields,agent,values_class); CHECK(fields);
    methods=(jobjectArray)(*env)->CallStaticObjectMethod(env,access_class,get_methods,agent,values_class); CHECK(methods && (*env)->GetArrayLength(env,methods)==3);
    classes=(jobjectArray)(*env)->CallObjectMethod(env,agent,get_classes); CHECK(classes && (*env)->GetArrayLength(env,classes)>5);
    field_class=(*env)->FindClass(env,"sun/tools/debug/Field"); CHECK(field_class);
    field_name=(*env)->GetFieldID(env,field_class,"name","Ljava/lang/String;"); field_slot=(*env)->GetFieldID(env,field_class,"slot","I"); CHECK(field_name && field_slot);
    for(i=0;i<(*env)->GetArrayLength(env,fields);++i) {
        field=(*env)->GetObjectArrayElement(env,fields,i); if(!field) continue;
        string=(jstring)(*env)->GetObjectField(env,field,field_name); name=(*env)->GetStringUTFChars(env,string,NULL); CHECK(name);
        slot=(*env)->GetIntField(env,field,field_slot); if(!strcmp(name,"number")) intslot=slot;
        signature_values=(jintArray)(*env)->CallObjectMethod(env,agent,signature,values_class,slot); CHECK(signature_values);
        (*env)->GetIntArrayRegion(env,signature_values,0,1,&tag);
        if(!strcmp(name,"flag")) CHECK(tag==0);
        else if(!strcmp(name,"small")) CHECK(tag==1);
        else if(!strcmp(name,"letter")) CHECK(tag==2);
        else if(!strcmp(name,"medium")) CHECK(tag==3);
        else if(!strcmp(name,"wide") || !strcmp(name,"sharedWide")) CHECK(tag==5);
        else if(!strcmp(name,"fraction")) CHECK(tag==6);
        else if(!strcmp(name,"precise") || !strcmp(name,"sharedDouble")) CHECK(tag==7);
        else if(!strcmp(name,"array")) CHECK(tag==9);
        else if(!strcmp(name,"object")) CHECK(tag==17);
        else CHECK(tag==4);
        if(!strcmp(name,"flag")) {
            CHECK((*env)->CallBooleanMethod(env,agent,get_bool,values,slot)); (*env)->CallVoidMethod(env,agent,set_bool,values,slot,0); CHECK(!(*env)->CallBooleanMethod(env,agent,get_bool,values,slot));
        } else if(!strcmp(name,"wide") || !strcmp(name,"sharedWide")) {
            CHECK((*env)->CallLongMethod(env,agent,get_long,values,slot)==(!strcmp(name,"wide") ? (jlong)-1234567890123 : (jlong)9876543210123));
            (*env)->CallVoidMethod(env,agent,set_long,values,slot,(jlong)-77777777777); CHECK((*env)->CallLongMethod(env,agent,get_long,values,slot)==(jlong)-77777777777);
        } else if(!strcmp(name,"fraction") || !strcmp(name,"precise") || !strcmp(name,"sharedDouble")) {
            CHECK((*env)->CallDoubleMethod(env,agent,get_double,values,slot)==(!strcmp(name,"fraction") ? 1.25 : (!strcmp(name,"precise") ? -9876.125 : 2.75)));
            (*env)->CallVoidMethod(env,agent,set_double,values,slot,-32.125); CHECK((*env)->CallDoubleMethod(env,agent,get_double,values,slot)==-32.125);
        } else if(!strcmp(name,"object")) { CHECK((*env)->CallObjectMethod(env,agent,get_object,values,slot));
        } else if(!strcmp(name,"array")) { jobject array=(*env)->CallObjectMethod(env,agent,get_array,values,slot); CHECK(array && (*env)->GetArrayLength(env,(jarray)array)==2);
        } else { (*env)->CallVoidMethod(env,agent,set_int,values,slot,23); CHECK((*env)->CallIntMethod(env,agent,get_int,values,slot)==23); }
        CHECK(!(*env)->ExceptionOccurred(env)); (*env)->ReleaseStringUTFChars(env,string,name);
    }
    CHECK((*env)->CallObjectMethod(env,agent,source,values_class)); CHECK((*env)->CallObjectMethod(env,agent,lines,values_class));
    CHECK(intslot>=0);
    CHECK((*env)->CallIntMethod(env,agent,get_int,values,-1)==0); CHECK(!(*env)->ExceptionOccurred(env));
    (*env)->CallIntMethod(env,agent,get_int,NULL,intslot); CHECK(exception_is(env,"java/lang/NullPointerException"));
    (*env)->CallLongMethod(env,agent,get_long,values,-1); CHECK(exception_is(env,"java/lang/IllegalAccessError"));
    (*env)->CallBooleanMethod(env,agent,get_bool,values,intslot); CHECK(exception_is(env,"java/lang/IllegalAccessError"));
    (*env)->CallObjectMethod(env,agent,get_object,values,intslot); CHECK(exception_is(env,"java/lang/IllegalArgumentException"));
    (*env)->CallObjectMethod(env,agent,get_array,values,intslot); CHECK(exception_is(env,"java/lang/IllegalArgumentException"));
    trace=(*env)->GetStaticMethodID(env,access_class,"trace","(Ljava/lang/Object;)Ljava/lang/String;"); CHECK(trace);
    string=(jstring)(*env)->CallStaticObjectMethod(env,access_class,trace,agent); CHECK(string);
    name=(*env)->GetStringUTFChars(env,string,NULL);
    CHECK(name && strstr(name,"java.lang.RuntimeException agent probe\n") && strstr(name,"sun.tools.debug.Agent115Access.trace(Agent115Access.java:11)"));
    (*env)->ReleaseStringUTFChars(env,string,name);
    CHECK(!(*env)->ExceptionOccurred(env)); puts("agent original-compatible metadata and slot operations passed"); return 0;
}
