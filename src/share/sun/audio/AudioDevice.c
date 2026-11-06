/* Sun mmedia_g.dll legacy stubs and audioWrite validation, 10001484. */
#include "StubPreamble.h"
#include "audio.h"
stack_item *Java_sun_audio_AudioDevice_audioOpen_stub(stack_item *sp, struct execenv *ee)
{
    sp[0].i=audioOpen(); return sp+1;
}
stack_item *Java_sun_audio_AudioDevice_audioClose_stub(stack_item *sp, struct execenv *ee)
{
    audioClose(); return sp;
}
stack_item *Java_sun_audio_AudioDevice_audioWrite_stub(stack_item *sp, struct execenv *ee)
{
    HArrayOfByte *buf=(HArrayOfByte *)sp[1].h;
    int len=sp[2].i;
    if(!buf) SignalError(0,"java/lang/NullPointerException",0);
    else if(len>0) {
        if((unsigned int)len>obj_length(buf)) len=obj_length(buf);
        audioWrite((signed char *)unhand(buf)->body,len);
    }
    return sp;
}
