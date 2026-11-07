/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/*
 * @(#)class.c	1.44 95/11/15  
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

/*-
 *      Implementation of class Class
 *
 *      former threadruntime.c, Sun Sep 22 12:09:39 1991
 */

#include <stdio.h>
#include <signal.h>

#include "tree.h"
#include "javaString.h"

#include "monitor.h"
#include "threads.h"
#include "exceptions.h"

#include "java_lang_Class.h"
#include "java_lang_ClassLoader.h"

extern ClassClass *allocClassClass(void);
extern void lock_classes(void), unlock_classes(void);
/* Private legacy helper, retained outside the original 1.1.5 header. */
void java_lang_ClassLoader_checkInitialized(Hjava_lang_ClassLoader *);

ClassClass *
java_lang_Class_forName(ClassClass *this, HString *classname)
{
    char clname[256], *p;
    ClassClass *cb;
    ClassClass *h;
    int c;

    javaString2CString(classname, clname, sizeof(clname));
    for (p = clname ; (c = *p); p++) {
	if (c == '.') {
	    *p = '/';
	}
    }
    if ((cb = FindClass(EE(), clname, TRUE)) == 0) {
        /* Reconstructed: Sun forName preserves pending linkage errors. */
        if (!exceptionOccurred(EE()))
            SignalError(0, JAVAPKG "ClassNotFoundException", clname);
	return 0;
    }
    h = cbHandle(cb);
    return h;
}

Hjava_lang_String *
java_lang_Class_getName(ClassClass *this)
{
    ClassClass *cb;
    char clname[256];

    if ((this == NULL) || ((cb = this) == NULL)) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }

    classname2string(cbName(cb), clname, sizeof(clname));
    return makeJavaString(clname, strlen(clname));
}

ClassClass *
java_lang_Class_getSuperclass(ClassClass *this)
{
    ClassClass *cb;
    if ((this == NULL) || ((cb = this) == NULL)) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    return cbIsPrimitive(cb) || cbIsInterface(cb) ? NULL : cbSuperclass(cb);
}

HArrayOfObject *
java_lang_Class_getInterfaces(ClassClass *this)
{
    int i;
    ClassClass *cb;
    HArrayOfObject *list;
    ExecEnv *ee = EE();
    ClassClass **listdata;

    if ((this == NULL) || ((cb = this) == NULL)) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }

    list = (HArrayOfObject *)ArrayAlloc(T_CLASS, cbImplementsCount(cb));
    if (list == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return 0;
    }
    listdata = (ClassClass **)unhand(list)->body;
    listdata[cbImplementsCount(cb)] = (void *)classJavaLangClass;

    /* See if any of the interfaces that we implement possibly implement icb */
    for (i = 0 ; i < (int)cbImplementsCount(cb) ; i++) {
	ResolveClassConstant(cbConstantPool(cb), cbImplements(cb)[i], ee, 1 << CONSTANT_Class);

	listdata[i] = cbHandle((ClassClass *)(cbConstantPool(cb)[cbImplements(cb)[i]].p));

    }

    return list;
}

Hjava_lang_ClassLoader *
java_lang_Class_getClassLoader(ClassClass *this)
{
    ClassClass *cb;
    if ((this == NULL) || ((cb = this) == NULL)) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    return (Hjava_lang_ClassLoader *)cbLoader(cb);
}

HObject *
java_lang_Class_newInstance(ClassClass *this)
{
    ExecEnv *ee = EE();
    ClassClass *cb;
    char *detail = 0, *error;

    if ((this == NULL) || ((cb = this) == NULL)) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    /* Sun javai_g 1000B81A resolves classes returned by loadClass(false)
     * before constructor lookup; defineClass alone has not assigned IDs. */
    if (!CCIs(cb, Resolved)) {
        error = ResolveClass(cb, &detail);
        if (error) {
            if (!exceptionOccurred(ee)) SignalError(ee, error, detail);
            return 0;
        }
    }
    return execute_java_constructor(ee, NULL, cb, "()");
}

long 
java_lang_Class_isInterface(ClassClass *this) 
{
    ClassClass *cb;
    if ((this == NULL) || ((cb = this) == NULL)) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    return (cbAccess(cb) & ACC_INTERFACE) ? 1 : 0;
}

/**
 * Class loader
 */

extern ClassClass *(*class_loader_func)(HObject *loader, char *name, long resolve);

static ClassClass *
class_loader(HObject *loader, char *name, long resolve) 
{
    char buf[256], *p;
    HObject *c;

    jio_snprintf(buf, sizeof(buf), "%s", name); /* XXX this should be a *MUCH* more efficient strncpy() */

    for (p = buf ; *p ; p++) {
	if (*p == '/') {
	    *p = '.';
	}
    }
    
    /* XXX: Should we throw an exception when we see any illegal characters here??
     This note was identified by David Hopwood.*/

    c = (HObject *)execute_java_dynamic_method(0, loader, "loadClass", 
					      "(L" JAVAPKG "String;Z)L" JAVAPKG "Class;", 
					      makeJavaString(buf, strlen(buf)), resolve);
    return c ? (ClassClass *)c : NULL;
}

ClassClass *
java_lang_ClassLoader_defineClass(Hjava_lang_ClassLoader *this, Hjava_lang_String * name, HArrayOfByte *data, long offset, long length) 
{
    ClassClass	    *cb;
    unsigned char   *body;
    int32_t	    body_len;
    char	    *detail;
    char	    *ename;
    ClassClass	    *h;
    char	    *namep;
    int		    ret_val;


    /* XXX: Get a boolean return from the following, and it will not be necessary to create an EE() */
    java_lang_ClassLoader_checkInitialized(this);
    if (exceptionOccurred(EE()))
      return 0;

    if (data == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    cb = allocClassClass();
    if (cb == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return 0;
    }
    cbLoader(cb) = this;
    body = (unsigned char *) unhand(data)->body;
    body_len = (int32_t) obj_length(data);
    if (offset < 0 || offset >= body_len || length < 0
		   || offset + length > body_len) {
	SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	return 0;
    }

    sysAssert(!cbName(cb));
    if (name) {                        /* Expected class name is optional */
        char *p;
        namep = allocCString(name);
        if (!namep)                    /* out of memory was already signaled */
            return 0;
        for (p = namep; *p; p++)       /* Translate from '.' to '/' separators */
            if ('.' == *p) 
                *p = '/';  
        /* The parser checks the expected name against the classfile. */
    } else {
        namep = NULL;       /* Indicate no expectations */
    }

    /* We need to lock the classes while creating the class and adding
     * the class to the list of classes
    */
    lock_classes();
    ret_val = createInternalClass(body + offset, body + offset + length, cb, this, namep, &detail);
    if (namep) {
        sysFree(namep);
        namep = 0;
    }
    if (!ret_val) {
	/*	If there was no exception already (OutOfMemory), then 
		throw a class format error. */
	if (!exceptionOccurred(EE()))
		SignalError(0, JAVAPKG "ClassFormatError", 0);
	unlock_classes();
	return 0;
    }
    unlock_classes();

    /*
    ** InitializeClass will lock the class and initialize it if
    ** necessary. Note that we need to hold the CCF_Locked across the
    ** call to prevent the class unloader from unloading this currently
    ** unreferenced class!
    */
    if ((ename = InitializeClass(cb, &detail)) != 0) {
	SignalError(0, ename, detail);
	return 0;
    }
    h = cbHandle(cb);		/* now there is a reference from C */
    return h;
}

void
java_lang_ClassLoader_resolveClass(Hjava_lang_ClassLoader *this, ClassClass *c)
{
    ClassClass  *cb;
    char	*detail;
    char	*ename;

    java_lang_ClassLoader_checkInitialized(this);
    if (exceptionOccurred(EE()))
      return;

    if ((this == NULL) || ((cb = c) == NULL)) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }

    /* ResolveClass will lock the class and resolve it if necessary */
    if ((ename = ResolveClass(cb, &detail)) != 0) {
	SignalError(0, ename, detail);
    }
}

ClassClass *
java_lang_ClassLoader_findSystemClass(Hjava_lang_ClassLoader *this, HString *classname)
{
    char clname[256], *p;
    ClassClass *cb;
    ClassClass *h;
    int c;

    java_lang_ClassLoader_checkInitialized(this);
    if (exceptionOccurred(EE()))
      return 0;

    javaString2CString(classname, clname, sizeof(clname));
    for (p = clname ; (c = *p); p++) {
	if (c == '.') {
	    *p = '/';
	}
    }
    if ((cb = FindClassFromClass(0, clname, TRUE, 0)) == 0) {
	SignalError(0, JAVAPKG "ClassNotFoundException", clname);
	return 0;
    }
    h = cbHandle(cb);
    return h;
}

void
java_lang_ClassLoader_init(Hjava_lang_ClassLoader *this)
{
    /* make sure the runtime can do upcalls */
    class_loader_func = class_loader;
}

void 
java_lang_ClassLoader_checkInitialized(Hjava_lang_ClassLoader *this) {
    if (this == NULL) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        return;
    }

    if (!unhand(this)->initialized) {
        SignalError(0, JAVAPKG "SecurityException", "Attempted use of uninitialized ClassLoader");
        return;
    }
}

/* Transitional adapters: expose the original 1.1.5 C entry names while the
 * parser/resolver implementation still uses the older class-body layout. */
ClassClass *java_lang_ClassLoader_defineClass0(Hjava_lang_ClassLoader *loader,
    HString *name, HArrayOfByte *bytes, long offset, long length)
{
    return java_lang_ClassLoader_defineClass(loader, name, bytes, offset, length);
}

void java_lang_ClassLoader_resolveClass0(Hjava_lang_ClassLoader *loader, ClassClass *type)
{
    java_lang_ClassLoader_resolveClass(loader, type);
}

ClassClass *java_lang_ClassLoader_findSystemClass0(Hjava_lang_ClassLoader *loader, HString *name)
{
    return java_lang_ClassLoader_findSystemClass(loader, name);
}
