#include <stdio.h>
#include <string.h>
#include "StubPreamble.h"
#include "interpreter.h"
#include "java_lang_Thread.h"
#include "sun_tools_debug_Agent.h"
#ifdef _WIN32
#include <windows.h>
#endif
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static int errors;
void SignalError(ExecEnv *ee, char *name, char *message) { ++errors; }
#ifdef _WIN32
static int oracle_stack(const char *path)
{
    HMODULE dll; char absolute[MAX_PATH];
    const char *names[]={"setStackInt","getStackInt","setStackBoolean","getStackBoolean","setStackLong","getStackLong","setStackDouble","getStackDouble","getStackFloat","getStackObject","getStackArray"};
    typedef stack_item *(*stub_fn)(stack_item *,ExecEnv *);
    stub_fn call[11]; char symbol[128]; int i;
    Classjava_lang_Thread body; Hjava_lang_Thread thread;
    ExecEnv ee; JavaFrame frame,foreign; struct methodblock method;
    stack_item args[7],vars[8]; Java8 temp;
    CHECK(GetFullPathNameA(path,sizeof(absolute),absolute,NULL)<sizeof(absolute));
    dll=LoadLibraryExA(absolute,NULL,LOAD_WITH_ALTERED_SEARCH_PATH); CHECK(dll);
    for(i=0;i<11;++i) {
        sprintf(symbol,"Java_sun_tools_debug_Agent_%s_stub",names[i]);
        call[i]=(stub_fn)GetProcAddress(dll,symbol); CHECK(call[i]);
    }
    /* Both runtimes share the offsets used here: Thread.eetop=16,
     * ExecEnv.current_frame=4, frame.vars=12, prev=16, method=28. */
    memset(&body,0,sizeof(body)); memset(&ee,0,sizeof(ee));
    memset(&frame,0,sizeof(frame)); memset(&foreign,0,sizeof(foreign));
    memset(&method,0,sizeof(method)); memset(vars,0,sizeof(vars));
    thread.obj=&body; body.eetop=(long)&ee; ee.current_frame=&foreign;
    foreign.prev=&frame; frame.current_method=&method; frame.vars=vars;
#define ARGS(slot) args[0].p=NULL; args[1].p=&thread; args[2].i=0; args[3].i=slot; args[6].i=0x12345678
    ARGS(0); args[4].i=-1234; CHECK(call[0](args,NULL)==args);
    ARGS(0); CHECK(call[1](args,NULL)==args+1 && args[0].i==-1234);
    ARGS(1); args[4].i=1; CHECK(call[2](args,NULL)==args);
    ARGS(1); CHECK(call[3](args,NULL)==args+1 && args[0].i==1);
    ARGS(2); SET_INT64(temp,args+4,(int64_t)-1234567890123); CHECK(call[4](args,NULL)==args);
    ARGS(2); CHECK(call[5](args,NULL)==args+2 && GET_INT64(temp,args)==(int64_t)-1234567890123);
    ARGS(4); SET_DOUBLE(temp,args+4,-123.75); CHECK(call[6](args,NULL)==args);
    ARGS(4); CHECK(call[7](args,NULL)==args+2 && GET_DOUBLE(temp,args)==-123.75);
    vars[6].f=7.25f; ARGS(6); CHECK(call[8](args,NULL)==args+1 && args[0].f==7.25f);
    vars[7].p=&thread; ARGS(7); CHECK(call[9](args,NULL)==args+1 && args[0].p==&thread);
    ARGS(7); CHECK(call[10](args,NULL)==args+1 && args[0].p==&thread);
    CHECK(args[6].i==0x12345678);
    FreeLibrary(dll); puts("agent original-compatible stack wrappers passed"); return 0;
#undef ARGS
}
#endif
int main(int argc,char **argv) {
    Classjava_lang_Thread body;
    Hjava_lang_Thread thread;
    ExecEnv ee;
    JavaFrame frame, nativeframe;
    struct methodblock method;
    stack_item vars[8];
    Java8 wide;
#ifdef _WIN32
    if(argc==2) return oracle_stack(argv[1]);
#endif
    memset(&body,0,sizeof(body)); memset(&ee,0,sizeof(ee));
    memset(&frame,0,sizeof(frame)); memset(&nativeframe,0,sizeof(nativeframe));
    memset(&method,0,sizeof(method)); memset(vars,0,sizeof(vars));
    thread.obj=&body; body.eetop=(long)&ee;
    ee.current_frame=&nativeframe; nativeframe.prev=&frame;
    frame.current_method=&method; frame.vars=vars;
    sun_tools_debug_Agent_setStackInt(NULL,&thread,0,0,-1234);
    CHECK(sun_tools_debug_Agent_getStackInt(NULL,&thread,0,0)==-1234);
    sun_tools_debug_Agent_setStackBoolean(NULL,&thread,0,1,1);
    CHECK(sun_tools_debug_Agent_getStackBoolean(NULL,&thread,0,1)==1);
    sun_tools_debug_Agent_setStackDouble(NULL,&thread,0,2,-123.75);
    CHECK(sun_tools_debug_Agent_getStackDouble(NULL,&thread,0,2)==-123.75);
    sun_tools_debug_Agent_setStackLong(NULL,&thread,0,4,(int64_t)-1234567890123);
    CHECK(sun_tools_debug_Agent_getStackLong(NULL,&thread,0,4)==(int64_t)-1234567890123);
    vars[6].f=7.25f;
    CHECK(sun_tools_debug_Agent_getStackFloat(NULL,&thread,0,6)==7.25f);
    vars[7].p=&thread;
    CHECK((void*)sun_tools_debug_Agent_getStackObject(NULL,&thread,0,7)==&thread);
    CHECK((void*)sun_tools_debug_Agent_getStackArray(NULL,&thread,0,7)==&thread);
    CHECK(!sun_tools_debug_Agent_getStackInt(NULL,&thread,0,-1)); CHECK(!errors);
    CHECK(!sun_tools_debug_Agent_getStackInt(NULL,&thread,1,0)); CHECK(errors==1);
    body.eetop=0;
    CHECK(!sun_tools_debug_Agent_getStackInt(NULL,&thread,0,0)); CHECK(errors==2);
    puts("agent stack locals passed"); return 0;
}
