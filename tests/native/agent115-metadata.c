/* Real VM objects and metadata, exercising the agent's legacy handle API. */
#include <stdio.h>
#include <string.h>
#include "StubPreamble.h"
#include "interpreter.h"
#include "javaString.h"
#include "sun_tools_debug_Agent.h"
#include "sun_tools_debug_Field.h"
#include "sun_tools_debug_LineNumber.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); (*env)->ExceptionDescribe(env); return 1; } } while(0)
int main(int argc,char **argv) {
    JavaVM *vm; JNIEnv *env; JDK1_1InitArgs args;
    ClassClass *cls,*agentclass; JavaFrame *caller; HArrayOfObject *fields,*methods,*classes;
    HObject *object; Hsun_tools_debug_Field *field;
    char name[128]; int i,slot=-1; HArrayOfInt *lines; Hsun_tools_debug_LineNumber *line; long pc;
    memset(&args,0,sizeof(args)); args.version=0x10001;
    if (argc!=2 || JNI_GetDefaultJavaVMInitArgs(&args)) return 2;
    args.classpath=argv[1]; if (JNI_CreateJavaVM(&vm,&env,&args)) return 3;
    agentclass=FindClass(NULL,"sun/tools/debug/Agent",TRUE); CHECK(agentclass);
    caller=(JavaFrame *)EE()->initial_stack->data;
    memset(caller,0,sizeof(*caller)); caller->current_method=cbMethods(agentclass);
    caller->javastack=EE()->initial_stack; caller->optop=caller->ostack;
    caller->constant_pool=cbConstantPool(agentclass); EE()->current_frame=caller;
    cls=FindClass(NULL,"java/lang/Integer",TRUE); CHECK(cls);
    fields=sun_tools_debug_Agent_getFields(NULL,(void*)cls); CHECK(fields);
    methods=sun_tools_debug_Agent_getMethods(NULL,(void*)cls); CHECK(methods && obj_length(methods)>0);
    classes=sun_tools_debug_Agent_getClasses(NULL); CHECK(classes && obj_length(classes)>5);
    for(i=0;i<obj_length(fields);++i) {
        field=(void*)unhand(fields)->body[i]; if(!field) continue;
        javaString2CString(unhand(field)->name,name,sizeof(name));
        if(!strcmp(name,"value")) slot=unhand(field)->slot;
    }
    CHECK(slot>=0);
    object=execute_java_constructor(NULL,NULL,cls,"(I)",1234); CHECK(object);
    CHECK(sun_tools_debug_Agent_getSlotInt(NULL,object,slot)==1234);
    sun_tools_debug_Agent_setSlotInt(NULL,object,slot,-9876);
    CHECK(sun_tools_debug_Agent_getSlotInt(NULL,object,slot)==-9876);
    CHECK(sun_tools_debug_Agent_getSlotSignature(NULL,(void*)cls,slot));
    CHECK(sun_tools_debug_Agent_getClassSourceName(NULL,(void*)cls));
    CHECK(sun_tools_debug_Agent_getLinenumbers(NULL,(void*)cls));
    cls=FindClass(NULL,"Agent115Values",TRUE); CHECK(cls);
    object=execute_java_constructor(NULL,NULL,cls,"()"); CHECK(object);
    fields=sun_tools_debug_Agent_getFields(NULL,(void*)cls); CHECK(fields);
    for(i=0;i<obj_length(fields);++i) {
        field=(void*)unhand(fields)->body[i]; if(!field) continue;
        javaString2CString(unhand(field)->name,name,sizeof(name)); slot=unhand(field)->slot;
        if(!strcmp(name,"flag")) {
            CHECK(sun_tools_debug_Agent_getSlotBoolean(NULL,object,slot));
            sun_tools_debug_Agent_setSlotBoolean(NULL,object,slot,0);
            CHECK(!sun_tools_debug_Agent_getSlotBoolean(NULL,object,slot));
        } else if(!strcmp(name,"wide") || !strcmp(name,"sharedWide")) {
            CHECK(sun_tools_debug_Agent_getSlotLong(NULL,object,slot)==(!strcmp(name,"wide") ? (int64_t)-1234567890123 : (int64_t)9876543210123));
            sun_tools_debug_Agent_setSlotLong(NULL,object,slot,(int64_t)-77777777777);
            CHECK(sun_tools_debug_Agent_getSlotLong(NULL,object,slot)==(int64_t)-77777777777);
        } else if(!strcmp(name,"fraction") || !strcmp(name,"precise") || !strcmp(name,"sharedDouble")) {
            CHECK(sun_tools_debug_Agent_getSlotDouble(NULL,object,slot)==(!strcmp(name,"fraction") ? 1.25 : (!strcmp(name,"precise") ? -9876.125 : 2.75)));
            sun_tools_debug_Agent_setSlotDouble(NULL,object,slot,-32.125);
            CHECK(sun_tools_debug_Agent_getSlotDouble(NULL,object,slot)==-32.125);
        } else if(!strcmp(name,"object")) {
            CHECK(sun_tools_debug_Agent_getSlotObject(NULL,object,slot));
        } else if(!strcmp(name,"array")) {
            CHECK(obj_length(sun_tools_debug_Agent_getSlotArray(NULL,object,slot))==2);
        } else {
            sun_tools_debug_Agent_setSlotInt(NULL,object,slot,23);
            CHECK(sun_tools_debug_Agent_getSlotInt(NULL,object,slot)==23);
        }
    }
    lines=sun_tools_debug_Agent_getLinenumbers(NULL,cls); CHECK(lines && obj_length(lines)>0);
    for(i=0;i<obj_length(lines);++i) {
        if(i) CHECK(unhand(lines)->body[i]>=unhand(lines)->body[i-1]);
        line=sun_tools_debug_Agent_lineno2pc(NULL,cls,unhand(lines)->body[i]); CHECK(line);
        CHECK(sun_tools_debug_Agent_pc2lineno(NULL,cls,unhand(line)->startPC)==unhand(lines)->body[i]);
    }
    for(i=0;i<cbMethodsCount(cls);++i) {
        if(!strcmp(fieldname(&cbMethods(cls)[i].fb),"<clinit>")) continue;
        pc=sun_tools_debug_Agent_method2pc(NULL,cls,i); CHECK(pc);
        CHECK(sun_tools_debug_Agent_getMethodLinenumber(NULL,cls,i)>0);
    }
    CHECK(!(*env)->ExceptionOccurred(env));
    puts("agent class enumeration, fields, methods, slots and line tables passed");
    return 0;
}
