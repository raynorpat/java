/* Real-device smoke test. A missing endpoint is an explicit CTest skip. */
#include <stdio.h>
#include <string.h>
#include "StubPreamble.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
typedef stack_item *(*audio_stub)(stack_item *, struct execenv *);
int main(int argc,char **argv)
{
    audio_stub open_audio,write_audio,close_audio;
    stack_item sp[3];
    HArrayOfByte array;
    signed char samples[16000];
    int cycle;
#ifdef _WIN32
    HMODULE library;
    library=argc>1 ? LoadLibraryA(argv[1]) : 0;
    if(!library) { fprintf(stderr,"cannot load mmedia\n"); return 1; }
    open_audio=(audio_stub)GetProcAddress(library,"Java_sun_audio_AudioDevice_audioOpen_stub");
    write_audio=(audio_stub)GetProcAddress(library,"Java_sun_audio_AudioDevice_audioWrite_stub");
    close_audio=(audio_stub)GetProcAddress(library,"Java_sun_audio_AudioDevice_audioClose_stub");
#else
    void *library;
    library=argc>1 ? dlopen(argv[1],RTLD_NOW) : 0;
    if(!library) { fprintf(stderr,"cannot load mmedia: %s\n",dlerror()); return 1; }
    *(void **)(&open_audio)=dlsym(library,"Java_sun_audio_AudioDevice_audioOpen_stub");
    *(void **)(&write_audio)=dlsym(library,"Java_sun_audio_AudioDevice_audioWrite_stub");
    *(void **)(&close_audio)=dlsym(library,"Java_sun_audio_AudioDevice_audioClose_stub");
#endif
    if(!open_audio || !write_audio || !close_audio) return 2;
    memset(samples,127,sizeof(samples));
    array.obj=(ClassArrayOfByte *)samples;
    array.methods=(struct methodtable *)(sizeof(samples)<<5);
    for(cycle=0;cycle<2;cycle++) {
        sp[0].p=0;
        if(open_audio(sp,0)!=sp+1) return 3;
        if(sp[0].i==-1) { puts("SKIP: audio device unavailable; playback unverified"); return 77; }
        if(sp[0].i!=1) return 4;
        sp[0].p=0; open_audio(sp,0); if(sp[0].i!=1) return 5;
        sp[0].p=0; sp[1].p=&array; sp[2].i=sizeof(samples)+1;
        if(write_audio(sp,0)!=sp) return 6;
        sp[2].i=0; write_audio(sp,0); sp[2].i=-1; write_audio(sp,0);
        close_audio(sp,0);
    }
    puts("audio device accepted two silence streams; audible playback unverified");
    return 0;
}
