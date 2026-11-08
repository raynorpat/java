/* DR2 libmmedia.A.dylib: 55401130--55401c4b. */
#import "audio.h"
#import "../awt/awt_ThreadContext.h"
#import "../awt/awt_Exception.h"
#include <string.h>
#include <stdio.h>

static AudioDevice *audioDevice;
static int soundFormat(int encoding)
{
    switch (encoding) {
    case 600: return 3;
    case 601: return 2;
    case 602: return 1;
    case 603: return 27;
    case 604: return 28;
    default: return 0;
    }
}
static int soundEncoding(int format)
{
    switch (format) {
    case 1: return 602;
    case 2: return 601;
    case 3: return 600;
    case 27: return 603;
    case 28: return 604;
    default: return 0;
    }
}
@implementation AudioDevice
- (id)init
{
    self = [super init];
    _audioOut = [[NXSoundOut alloc] init];
    if (!_audioOut) { [self release]; return nil; }
    _tag = -1; _isClosed = YES;
    return self;
}
- (void)dealloc
{
    if (_audioSound) SNDFree(_audioSound);
    [_audioStream release]; [_audioOut release]; [_audioParams release];
    [super dealloc];
}
- (int)open
{
    if (!_audioStream) {
        NXSoundDeviceError error;
        if (!_audioParams) {
            const NXSoundParameterTag *encodings = NULL;
            unsigned int count = 0;
            int format = 0, index;
            [_audioOut getStreamDataEncodings:&encodings count:&count];
            for (index = (int)count - 1; index >= 0; index--) {
                format = soundFormat(encodings[index]);
                if (format == SND_FORMAT_MULAW_8) break;
            }
            _audioParams = [[NXSoundParameters alloc] init];
            [_audioParams setParameter:NX_SoundStreamSamplingRate toFloat:22050];
            [_audioParams setParameter:NX_SoundStreamChannelCount toInt:2];
            [_audioParams setParameter:NX_SoundStreamDataEncoding toInt:soundEncoding(format)];
            if (format != SND_FORMAT_MULAW_8) {
                int result = SNDAlloc(&_audioSound, 400, SND_FORMAT_MULAW_8, 8012, 1, 0);
                if (result) {
                    [_audioParams release]; _audioParams = nil;
                    [NSException raise:NSMallocException format:@"SNDAlloc: %s", SNDSoundError(result)];
                }
                _conversionRequired = YES; _deviceDataFormat = format;
            }
        }
        _audioStream = [[NXPlayStream alloc] initOnDevice:_audioOut withParameters:_audioParams];
        if (!_audioStream) return -1;
        _completedTag = -1;
        error = [_audioStream activate];
        if (error) {
            NSLog(error == NX_SoundDeviceErrorTimeout ? @"%@: stream activate transient failure (%d)" :
                @"%@: stream activate definitive failure (%d)", [self class], [_audioStream lastError]);
            [_audioStream release]; _audioStream = nil;
            return error == NX_SoundDeviceErrorTimeout ? 0 : -1;
        }
        [_audioStream setGainLeft:1 right:1];
        [_audioStream pause:nil];
        if ([_audioStream lastError]) NSLog(@"%@: stream pause failure (%d)", [self class], [_audioStream lastError]);
    }
    _isClosed = NO;
    return 1;
}
- (void)close { [_audioStream pause:nil]; _isClosed = YES; }
- (BOOL)isClosed { return _isClosed; }
- (void)write:(const char *)bytes size:(int)length
{
    SNDSoundStruct *volatile converted = NULL;
    SNDSoundStruct configuration = { SND_MAGIC, sizeof(SNDSoundStruct), 0, 0, 22050, 2, {0} };
    NS_DURING
        if (_conversionRequired) {
            int previousSize = _audioSound->dataSize, error;
            configuration.dataFormat = _deviceDataFormat;
            if (length > previousSize) {
                SNDSoundStruct *replacement = NULL;
                int format = _audioSound->dataFormat;
                error = SNDAlloc(&replacement, length, format, 8012, 1, 0);
                if (error) [NSException raise:NSMallocException format:@"SNDAlloc: %s", SNDSoundError(error)];
                SNDFree(_audioSound); _audioSound = replacement;
            } else _audioSound->dataSize = length;
            memcpy((char *)_audioSound + _audioSound->dataLocation, bytes, length);
            converted = &configuration;
            error = SNDConvertSound(_audioSound, (SNDSoundStruct **)&converted);
            if (length < previousSize) _audioSound->dataSize = previousSize;
            if (error) {
                if (converted && converted != &configuration) SNDFree(converted);
                converted = NULL;
                [NSException raise:NSGenericException format:@"SNDConvertSound: %s", SNDSoundError(error)];
            }
            bytes = (char *)converted + converted->dataLocation;
            length = converted->dataSize;
        }
        [_audioStream pause:nil];
        {
            int error = [_audioStream playBuffer:(void *)bytes size:length tag:++_tag];
            if (error) NSLog(@"%@: sound error after playBuffer of _audioStream: %s", [self class], SNDSoundError(error));
        }
        [_audioStream resume:nil];
    NS_HANDLER
        if (converted && converted != &configuration) SNDFree(converted);
        [localException raise];
    NS_ENDHANDLER
    if (converted) SNDFree(converted);
}
- (void)soundStream:(id)stream didCompleteBuffer:(int)tag
{ _completedTag = tag; fprintf(stderr, "completed buffer %d\n", tag); }
- (void)soundStreamDidUnderrun:(id)stream { NSLog(@"%@: sound underrun on stream %@\n", [self class], stream); }
@end

JNIEXPORT jint JNICALL Java_sun_audio_AudioDevice_audioOpen(JNIEnv *env, jobject peer)
{
    jint result = -1;
    AudioDevice *volatile candidate = nil;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        if (!audioDevice) {
            candidate = [AudioDevice alloc];
            candidate = [candidate init];
            audioDevice = candidate;
        }
        if (audioDevice) result = [audioDevice open];
    NS_HANDLER
        if (candidate && candidate != audioDevice) [candidate release];
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
    return result;
}
JNIEXPORT void JNICALL Java_sun_audio_AudioDevice_audioClose(JNIEnv *env, jobject peer)
{
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        [audioDevice close];
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (pool) NativeMethodExit(env, pool);
}
JNIEXPORT void JNICALL Java_sun_audio_AudioDevice_audioWrite(JNIEnv *env, jobject peer, jbyteArray samples, jint length)
{
    jbyte *volatile bytes = NULL;
    AwtAutoreleasePool *pool = NativeMethodEnter(env);
    NS_DURING
        if (!samples) [AwtException raise:env exceptionName:"java/lang/NullPointerException" reason:"null audio samples"];
        if (length > 0 && audioDevice && ![audioDevice isClosed]) {
            jsize size = (*env)->GetArrayLength(env, samples);
            if (length > size) length = size;
            bytes = (*env)->GetByteArrayElements(env, samples, NULL);
            if (!bytes) [AwtException raiseUsingEnv:env];
            [audioDevice write:(const char *)bytes size:length];
        }
    NS_HANDLER
        [AwtException throwToJava:env exception:localException];
    NS_ENDHANDLER
    if (bytes) (*env)->ReleaseByteArrayElements(env, samples, bytes, JNI_ABORT);
    if (pool) NativeMethodExit(env, pool);
}
