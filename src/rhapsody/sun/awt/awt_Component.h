#ifndef RHAPSODY_AWT_COMPONENT_H
#define RHAPSODY_AWT_COMPONENT_H

#import "awt_View.h"
AwtView *realViewFromPeer(JNIEnv *env, jobject peer);
id viewFromPeer(JNIEnv *env, jobject peer);
void setViewFromPeer(JNIEnv *env, jobject peer, AwtView *view);
id windowFromPeer(JNIEnv *env, jobject peer);

#endif
