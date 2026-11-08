#import "../../src/rhapsody/sun/audio/audio.h"
#include "jni.h"
#include <stdio.h>
static int formatMode, activationError, liveDevices, liveStreams, pauses, resumes, buffers;
static unsigned int lastSize, lastHash;
static int lastTag;
static NXSoundParameters *parameters;
static BOOL failWrite;
@interface SoundOutProbe : NXSoundOut
@end
@implementation SoundOutProbe
- (id)init { liveDevices++; return self; }
- (void)dealloc { liveDevices--; [super dealloc]; }
- (NXSoundDeviceError)getStreamDataEncodings:(const NXSoundParameterTag **)encodings count:(unsigned int *)count
{
    static NXSoundParameterTag direct[] = { NX_SoundStreamDataEncoding_Linear16, NX_SoundStreamDataEncoding_Mulaw8 };
    static NXSoundParameterTag converted[] = { NX_SoundStreamDataEncoding_Linear16 };
    *encodings = formatMode ? converted : direct; *count = formatMode ? 1 : 2;
    return 0;
}
@end
@interface PlayStreamProbe : NXPlayStream
@end
@implementation PlayStreamProbe
- (id)initOnDevice:(id)device withParameters:(id<NXSoundParameters>)params
{ liveStreams++; parameters = (NXSoundParameters *)params; return self; }
- (void)dealloc { liveStreams--; [super dealloc]; }
- (NXSoundDeviceError)activate { return activationError; }
- (NXSoundDeviceError)lastError { return activationError; }
- (NXSoundDeviceError)setGainLeft:(float)left right:(float)right
{
    if (left != 1 || right != 1) [NSException raise:@"AudioProbe" format:@"Wrong gain"];
    return 0;
}
- (void)pause:(id)sender { pauses++; }
- (void)resume:(id)sender { resumes++; }
- (NXSoundDeviceError)playBuffer:(void *)data size:(unsigned int)size tag:(int)tag
{
    unsigned int i, hash = 0;
    if (failWrite) [NSException raise:@"AudioProbe" format:@"injected audio write"];
    for (i = 0; i < size; i++) hash = hash * 31 + ((unsigned char *)data)[i];
    lastSize = size; lastHash = hash; lastTag = tag; buffers++;
    return 0;
}
@end
#define CHECK(value) do { if (!(value)) [NSException raise:@"AudioProbe" format:@"check failed at %d", __LINE__]; } while (0)
JNIEXPORT jint JNICALL Java_AudioBackend115_buffers(JNIEnv *env, jclass cls) { return buffers; }
JNIEXPORT jint JNICALL Java_AudioBackend115_lastSize(JNIEnv *env, jclass cls) { return lastSize; }
JNIEXPORT void JNICALL Java_AudioBackend115_failWrite(JNIEnv *env, jclass cls, jboolean fail) { failWrite = fail; }
JNIEXPORT void JNICALL Java_AudioBackend115_exercise(JNIEnv *env, jclass cls)
{
    id pool = [[NSAutoreleasePool alloc] init];
    int mode, cycle, index;
    char samples[800];
    [SoundOutProbe poseAsClass:[NXSoundOut class]];
    [PlayStreamProbe poseAsClass:[NXPlayStream class]];
    for (index = 0; index < 800; index++) samples[index] = (char)(index * 17 + 3);
    NS_DURING
        for (mode = 0; mode < 2; mode++) {
            AudioDevice *device;
            int sizes[] = {400, 800, 80};
            formatMode = mode; activationError = 0;
            device = [[AudioDevice alloc] init]; CHECK(device && [device isClosed]);
            CHECK([device open] == 1); CHECK([device open] == 1); CHECK(![device isClosed]);
            CHECK([parameters intValueForParameter:NX_SoundStreamChannelCount] == 2);
            CHECK([parameters floatValueForParameter:NX_SoundStreamSamplingRate] == 22050);
            CHECK([parameters intValueForParameter:NX_SoundStreamDataEncoding] == (mode ? 600 : 602));
            for (cycle = 0; cycle < 3; cycle++) {
                int oldBuffers = buffers, oldPauses = pauses, oldResumes = resumes;
                [device write:samples size:sizes[cycle]];
                CHECK(buffers == oldBuffers + 1 && pauses == oldPauses + 1 && resumes == oldResumes + 1 && lastTag == cycle);
                printf("audio %d:%d:%u:%u\n", mode, cycle, lastSize, lastHash);
                [device close]; CHECK([device isClosed]); CHECK([device open] == 1);
            }
            [device close]; [device release];
            CHECK(liveDevices == 0 && liveStreams == 0);
        }
        for (mode = 0; mode < 2; mode++) {
            AudioDevice *device;
            formatMode = 0; activationError = mode ? 300 : 301;
            device = [[AudioDevice alloc] init];
            CHECK([device open] == (mode ? -1 : 0)); CHECK([device isClosed]); CHECK(liveStreams == 0);
            activationError = 0; CHECK([device open] == 1);
            [device close]; [device release]; CHECK(liveDevices == 0 && liveStreams == 0);
        }
    NS_HANDLER
        jclass error = (*env)->FindClass(env, "java/lang/RuntimeException");
        (*env)->ThrowNew(env, error, [[localException reason] cString]);
    NS_ENDHANDLER
    [pool release];
}
