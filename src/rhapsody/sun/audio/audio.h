#ifndef RHAPSODY_AUDIO_H
#define RHAPSODY_AUDIO_H
#import <SoundKit/SoundKit.h>
#import <SoundKit/sound.h>
@interface AudioDevice : NSObject {
    NXSoundOut *_audioOut;
    NXSoundParameters *_audioParams;
    NXPlayStream *_audioStream;
    SNDSoundStruct *_audioSound;
    BOOL _conversionRequired;
    int _deviceDataFormat;
    int _tag;
    int _completedTag;
    BOOL _isClosed;
}
- (int)open;
- (void)close;
- (BOOL)isClosed;
- (void)write:(const char *)bytes size:(int)length;
@end
#endif
