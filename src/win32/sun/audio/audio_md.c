/* Reconstructed from Sun JDK 1.1.5 mmedia_g.dll. */
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "audio.h"
#define SAMPLES 640
#define MAX_BUFFERS 10
typedef struct audio_buffer {
    WAVEHDR header;
    short samples[SAMPLES];
    struct audio_buffer *next;
} audio_buffer;
static HWAVEOUT wave;
static CRITICAL_SECTION lock;
static HANDLE event;
static audio_buffer *free_buffers;
static int allocated, completed;

static void reportError(const char *operation, MMRESULT error)
{
    char text[80];
    if(waveOutGetErrorTextA(error,text,sizeof(text))!=MMSYSERR_NOERROR)
        sprintf(text,"error %u",error);
    fprintf(stderr,"Error: %s: %s\n",operation,text);
}
static void CALLBACK waveCallback(HWAVEOUT device, UINT message, DWORD_PTR instance,
                                  DWORD_PTR parameter, DWORD_PTR unused)
{
    audio_buffer *buf;
    if(message!=WOM_DONE) return;
    buf=(audio_buffer *)parameter;
    EnterCriticalSection(&lock);
    buf->next=free_buffers; free_buffers=buf; completed++;
    SetEvent(event);
    LeaveCriticalSection(&lock);
}
int audioOpen(void)
{
    WAVEFORMATEX format;
    MMRESULT error;
    signed char silence[SAMPLES];
    if(wave) return 1;
    memset(&format,0,sizeof(format));
    format.wFormatTag=WAVE_FORMAT_PCM; format.nChannels=1;
    format.nSamplesPerSec=8000; format.nAvgBytesPerSec=16000;
    format.nBlockAlign=2; format.wBitsPerSample=16;
    error=waveOutOpen(&wave,WAVE_MAPPER,&format,(DWORD_PTR)waveCallback,0,CALLBACK_FUNCTION);
    if(error) { wave=0; reportError("waveOutOpen",error); return -1; }
    waveOutReset(wave);
    free_buffers=0; allocated=completed=0;
    InitializeCriticalSection(&lock);
    event=CreateEventA(0,FALSE,FALSE,0);
    if(!event) { waveOutClose(wave); wave=0; DeleteCriticalSection(&lock); return -1; }
    memset(silence,127,sizeof(silence)); audioWrite(silence,sizeof(silence));
    return 1;
}
void audioWrite(const signed char *data, int len)
{
    audio_buffer *buf;
    int count,i;
    MMRESULT error;
    /* Safety difference: do not submit to an unopened device. */
    if(!wave) return;
    while(len>0) {
        EnterCriticalSection(&lock);
        while(!free_buffers && allocated>=MAX_BUFFERS) {
            LeaveCriticalSection(&lock); WaitForSingleObject(event,INFINITE);
            EnterCriticalSection(&lock);
        }
        buf=free_buffers;
        if(buf) { free_buffers=buf->next; completed--; }
        LeaveCriticalSection(&lock);
        if(buf) {
            error=waveOutUnprepareHeader(wave,&buf->header,sizeof(WAVEHDR));
            if(error) { reportError("waveOutUnprepareHeader",error); free(buf); allocated--; return; }
        } else {
            buf=(audio_buffer *)malloc(sizeof(audio_buffer));
            if(!buf) return;
            allocated++;
        }
        count=len<SAMPLES ? len : SAMPLES;
        memset(&buf->header,0,sizeof(WAVEHDR));
        buf->header.lpData=(LPSTR)buf->samples;
        buf->header.dwBufferLength=buf->header.dwBytesRecorded=count*2;
        buf->header.dwUser=(DWORD_PTR)buf;
        error=waveOutPrepareHeader(wave,&buf->header,sizeof(WAVEHDR));
        if(error) { reportError("waveOutPrepareHeader",error); free(buf); allocated--; return; }
        for(i=0;i<count;i++) buf->samples[i]=(short)ulaw2linear(data[i]);
        error=waveOutWrite(wave,&buf->header,sizeof(WAVEHDR));
        if(error) {
            reportError("waveOutWrite",error);
            waveOutUnprepareHeader(wave,&buf->header,sizeof(WAVEHDR));
            free(buf); allocated--; return;
        }
        data+=count; len-=count;
    }
}
void audioClose(void)
{
    audio_buffer *buf,*next;
    MMRESULT error;
    if(!wave) return;
    waveOutReset(wave);
    EnterCriticalSection(&lock);
    while(completed<allocated) {
        LeaveCriticalSection(&lock); WaitForSingleObject(event,INFINITE);
        EnterCriticalSection(&lock);
    }
    LeaveCriticalSection(&lock);
    for(buf=free_buffers;buf;buf=buf->next)
        waveOutUnprepareHeader(wave,&buf->header,sizeof(WAVEHDR));
    error=waveOutClose(wave);
    if(error) { reportError("waveOutClose",error); return; }
    for(buf=free_buffers;buf;buf=next) { next=buf->next; free(buf); }
    wave=0; free_buffers=0; allocated=completed=0;
    /* Release handles leaked by the primary binary on repeated opens. */
    CloseHandle(event); event=0; DeleteCriticalSection(&lock);
}
