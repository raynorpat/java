/* New Linux OSS backend permitted by the reconstruction design. */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "audio.h"
static int audio_fd=-1;
int audioOpen(void)
{
    int format=AFMT_S16_LE, channels=1, rate=8000;
    signed char silence[640];
    if(audio_fd>=0) return 1;
    audio_fd=open("/dev/dsp",O_WRONLY);
    if(audio_fd<0) { perror("audioOpen /dev/dsp"); return -1; }
    if(ioctl(audio_fd,SNDCTL_DSP_SETFMT,&format)<0 || format!=AFMT_S16_LE ||
       ioctl(audio_fd,SNDCTL_DSP_CHANNELS,&channels)<0 || channels!=1 ||
       ioctl(audio_fd,SNDCTL_DSP_SPEED,&rate)<0 || rate!=8000) {
        fprintf(stderr,"audioOpen: 8000Hz mono signed 16-bit PCM unavailable\n");
        close(audio_fd); audio_fd=-1; return -1;
    }
    memset(silence,127,sizeof(silence)); audioWrite(silence,sizeof(silence));
    return 1;
}
void audioWrite(const signed char *data, int len)
{
    unsigned char pcm[1280];
    int count,i,value,offset,bytes;
    ssize_t written;
    if(audio_fd<0) return;
    while(len>0) {
        count=len<640 ? len : 640;
        for(i=0;i<count;i++) {
            value=ulaw2linear(data[i]);
            pcm[2*i]=(unsigned char)value; pcm[2*i+1]=(unsigned char)((unsigned int)value>>8);
        }
        bytes=count*2; offset=0;
        while(offset<bytes) {
            written=write(audio_fd,pcm+offset,bytes-offset);
            if(written<0 && errno==EINTR) continue;
            if(written<=0) { perror("audioWrite /dev/dsp"); return; }
            offset+=written;
        }
        data+=count; len-=count;
    }
}
void audioClose(void)
{
    if(audio_fd>=0) { ioctl(audio_fd,SNDCTL_DSP_RESET,0); close(audio_fd); audio_fd=-1; }
}
