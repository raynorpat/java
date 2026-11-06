/*
 * @(#)security.c	1.10 95/11/29 Jonathan Payne
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

#include "native.h"
#include "java_lang_SecurityManager.h"
#include "java_lang_ClassLoader.h"

/* Reconstructed shared stack walker: javai_g.dll 0x10040113. */
static HArrayOfObject *
getClassContext(void)
{
    struct javaframe *frame, frame_buf;
    HArrayOfObject *ctx;
    HObject **ctxdata;
    int n = 0;

    /* count the classes on the execution stack */
    for (frame = EE()->current_frame ; frame != 0 ; ) {
	if (frame->current_method != 0) {
	    n++;
            frame = frame->current_method->fb.access & ACC_MACHINE_COMPILED ?
		CompiledFramePrev(frame, &frame_buf) : frame->prev;
	} else {
	    frame = frame->prev;
	} 
    }

    /* allocate the array */
    ctx = (HArrayOfObject *)ArrayAlloc(T_CLASS, n);
    if (ctx == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return 0;
    }
    ctxdata = (HObject **)unhand(ctx)->body;
    ctxdata[n] = (HObject *)classJavaLangClass;

    /* fill the array */
    for (n = 0, frame = EE()->current_frame ; frame != 0 ; ) {
	if (frame->current_method != 0) {
	    ctxdata[n++] = (HObject *)(fieldclass(&frame->current_method->fb));
            frame = frame->current_method->fb.access & ACC_MACHINE_COMPILED ?
		CompiledFramePrev(frame, &frame_buf) : frame->prev;
	} else {
	    frame = frame->prev;
	}
    }

    return ctx;
}

HArrayOfObject *
java_lang_SecurityManager_getClassContext(Hjava_lang_SecurityManager *this)
{
    if (!java_lang_SecurityManager_checkInitialized(this))
        return 0;
    return getClassContext();
}

/* Reconstructed from javai_g.dll 0x1004027c: no SecurityManager check. */
HArrayOfObject *
java_util_ResourceBundle_getClassContext(void *unused)
{
    return getClassContext();
}

Hjava_lang_ClassLoader *
java_lang_SecurityManager_currentClassLoader(Hjava_lang_SecurityManager *this)
{
    struct javaframe *frame, frame_buf;
    ClassClass	*cb;

    if (!java_lang_SecurityManager_checkInitialized(this)) {
        /* exceptionOccurred */
        return 0;
    }

    for (frame = EE()->current_frame ; frame != 0 ; ) {
	if (frame->current_method != 0) {
	    cb = fieldclass(&frame->current_method->fb);
	    if (cb && cbLoader(cb)) {
		return (Hjava_lang_ClassLoader *)cbLoader(cb);
	    }
            frame = frame->current_method->fb.access & ACC_MACHINE_COMPILED ?
		CompiledFramePrev(frame, &frame_buf) : frame->prev;
	} else {
	    frame = frame->prev;
	} 
    }

    return 0;
}

long
java_lang_SecurityManager_classDepth(Hjava_lang_SecurityManager *this, HString *name)
{
    struct javaframe *frame, frame_buf;
    ClassClass	*cb;
    char buf[128], *p;
    int depth = 0;

    if (!java_lang_SecurityManager_checkInitialized(this)) {
        /* exceptionOccurred */
        return 0;
    }

    javaString2CString(name, buf, sizeof(buf));
    for (p = buf ; *p ; p++) {
	if (*p == '.') {
	    *p = '/';
	}
    }

    for (frame = EE()->current_frame ; frame != 0 ; ) {
	if (frame->current_method != 0) {
	    cb = fieldclass(&frame->current_method->fb);
	    if (cb && !strcmp(cbName(cb), buf)) {
		return depth;
	    }
            frame = frame->current_method->fb.access & ACC_MACHINE_COMPILED ?
		CompiledFramePrev(frame, &frame_buf) : frame->prev;
            depth++;
	} else {
	    frame = frame->prev;
	} 
    }
    return -1;
}

 
long /*boolean*/
java_lang_SecurityManager_checkClassLoader(Hjava_lang_SecurityManager *this, long arg_depth)
{
    struct javaframe *frame, frame_buf;
    ClassClass	*cb;
    int depth = 0;

    if (!java_lang_SecurityManager_checkInitialized(this)) {
        /* exceptionOccurred */
        return FALSE;
    }

    for (frame = EE()->current_frame ; (frame != 0 && depth <= arg_depth) ;) {
	if (frame->current_method != 0) {
            if (depth == arg_depth) {
                cb = fieldclass(&frame->current_method->fb);
                return (cb && cbLoader(cb));
	    }
            frame = frame->current_method->fb.access & ACC_MACHINE_COMPILED ?
		CompiledFramePrev(frame, &frame_buf) : frame->prev;
            depth++;
	} else {
	    frame = frame->prev;
	} 
    }
    return FALSE;
}

long /*boolean*/
java_lang_SecurityManager_checkInitialized(Hjava_lang_SecurityManager *this) {
    if (this == NULL) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        return (FALSE);
    }

    if (!unhand(this)->initialized) {
        SignalError(0, JAVAPKG "SecurityException", "Attempted use of uninitialized Security Manager");
        return (FALSE);
    }
    return (TRUE);
}




long
java_lang_SecurityManager_classLoaderDepth(Hjava_lang_SecurityManager *this)
{
    struct javaframe *frame, frame_buf;
    ClassClass	*cb;
    int depth = 0;

    if (!java_lang_SecurityManager_checkInitialized(this)) {
        /* exceptionOccurred */
        return 0;
    }

    for (frame = EE()->current_frame ; frame != 0 ;) {
	if (frame->current_method != 0) {
	    cb = fieldclass(&frame->current_method->fb);
	    if (cb && cbLoader(cb)) {
		return depth;
	    }
            frame = frame->current_method->fb.access & ACC_MACHINE_COMPILED ?
		CompiledFramePrev(frame, &frame_buf) : frame->prev;
            depth++;
	} else {
	    frame = frame->prev;
	} 
    }
    return -1;
}


