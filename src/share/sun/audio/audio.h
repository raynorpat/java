/* Reconstructed mmedia internal interface. */
#ifndef SUN_AUDIO_H
#define SUN_AUDIO_H
int ulaw2linear(signed char sample);
int audioOpen(void);
void audioClose(void);
void audioWrite(const signed char *data, int len);
#endif
