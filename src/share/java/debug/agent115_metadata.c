/*
 * @(#)agent.c	1.44 96/02/16 Arthur van Huff, Thomas Ball
 *
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
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

/* Adapted against agent_g.dll 10001475--10003575; see tests/native/agent115-evidence.md. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "oobj.h"
#include "tree.h"
#include "interpreter.h"
#include "threads.h"
#include "signature.h"
#include "javaString.h"
#include "typecodes.h"
#include "java_lang_Thread.h"
#include "java_lang_Throwable.h"
#include "sun_tools_java_Constants.h"
#include "sun_tools_debug_Agent.h"
#include "sun_tools_debug_AgentConstants.h"
#include "sun_tools_debug_Field.h"
#include "sun_tools_debug_StackFrame.h"
#include "sun_tools_debug_LocalVariable.h"
#include "sun_tools_debug_LineNumber.h"
#define get_classblock(o) ((obj_classblock(o) == get_classClass()) ? (ClassClass *)(o) : obj_classblock(o))
#define obj_getoffset(o, off) (*(OBJECT *)((char *)unhand(o)+(off)))
#define dprintf if (0) printf
extern JavaFrame *agent115_stack_frame(Hjava_lang_Thread *,long);
static void
RPI_decode_stack_frame(JavaFrame	*frame,
		       ClassClass		**clazz,
		       HString		**className,
		       HString		**methodName,
		       HString **methodSignature,
                       long *absolutePC,
                       long		*lineno,
		       long		*pc,
		       HArrayOfObject	**localVars)
{
    struct methodblock	*mb = frame->current_method;
    if (mb != 0) {
	char		buf[128];
	HObject		**localVarsData;
	int		i;
	int		nLocalVars;
	struct localvar	*lv;
	Hsun_tools_debug_LineNumber *ln;

	/* a normal method */
	ClassClass *cb = fieldclass(&mb->fb);

	*clazz = cbHandle(cb);
	classname2string(cbName(cb), buf, sizeof(buf));
	*className = makeJavaString(buf, strlen(buf));
	*methodName = makeJavaString(fieldname(&mb->fb), strlen(fieldname(&mb->fb)));
	*methodSignature = makeJavaString(fieldsig(&mb->fb), strlen(fieldsig(&mb->fb)));
        *absolutePC = (long)frame->lastpc;
	*pc = frame->lastpc - mb->code;
	*lineno = (long)pc2lineno(mb, *pc);

	nLocalVars = (int)mb->localvar_table_length;
	*localVars = (HArrayOfObject *) ArrayAlloc(T_CLASS, nLocalVars);
        if (*localVars == 0) {
            SignalError(0, JAVAPKG "OutOfMemoryError", 0);
            return;
        }
	localVarsData = (HObject **)unhand(*localVars);
	localVarsData[nLocalVars] = (HObject *)get_classObject();

	lv = mb->localvar_table;
	for (i = 0; i < nLocalVars; i++, lv++) {
	    long currentPC = (long)frame->lastpc - (long)mb->code;
	    Hsun_tools_debug_LocalVariable *hlocalvar;
	    char *p;

	    hlocalvar = (Hsun_tools_debug_LocalVariable*)
		execute_java_constructor(0,
                    "sun/tools/debug/LocalVariable", 0, "()");

	    if (!hlocalvar) {
	        dprintf("agent.c: hlocalvar==0\n");
	        SignalError(0, JAVAPKG "NullPointerException", 0);
	    }
            unhand(hlocalvar)->methodArgument = 0;

	    if (currentPC >= lv->pc0 &&
		currentPC < (lv->pc0 + lv->length)) {
		unhand(hlocalvar)->slot      = lv->slot;
                if (lv->slot < mb->args_size) {
                    unhand(hlocalvar)->methodArgument = TRUE;
                }
	    } else {
		unhand(hlocalvar)->slot      = -1;
	    }
		p = frame->constant_pool[lv->nameoff].cp;
		unhand(hlocalvar)->name      = makeJavaString(p, strlen(p));
		p = frame->constant_pool[lv->sigoff].cp;
		unhand(hlocalvar)->signature = makeJavaString(p, strlen(p));

	    localVarsData[i] = (HObject *)hlocalvar;
	}

    } else {
	/* a foreign function */
	HObject		**localVarsData;
	*absolutePC = -1;
	*lineno = -1;
	*pc = -1;
	*localVars = (HArrayOfObject *) ArrayAlloc(T_CLASS, 0);
        if (*localVars == 0) {
            SignalError(0, JAVAPKG "OutOfMemoryError", 0);
            return;
        }
	localVarsData = (HObject **)unhand(*localVars);
	localVarsData[0] = (HObject *)get_classObject();
	dprintf("native method stack frame\n");
    }
}

Hsun_tools_debug_StackFrame *
sun_tools_debug_Agent_getStackFrame(Hsun_tools_debug_Agent *hagent,
				     Hjava_lang_Thread *t,
				     long framenum)
{
    Hsun_tools_debug_StackFrame	*hframe;
    struct javaframe			*jframe;
    ExecEnv				*ee;

    if (unhand(t)->eetop == 0) {
	return NULL;
    }

    jframe = agent115_stack_frame(t, framenum);
    if (!jframe) {
	dprintf("agent.c: jframe==0 framenum=%ld\n", framenum);
	SignalError(0, JAVAPKG "NullPointerException", 0);
        return NULL;
    }

    hframe = (Hsun_tools_debug_StackFrame*)
	     execute_java_constructor(0, "sun/tools/debug/StackFrame",
				      0, "()");
    if (!hframe) {
	dprintf("agent.c: hframe==0\n");
	SignalError(0, JAVAPKG "NullPointerException", 0);
        return NULL;
    }

    RPI_decode_stack_frame(jframe,
			   &(unhand(hframe)->clazz),
			   &(unhand(hframe)->className),
			   &(unhand(hframe)->methodName),
			   &(unhand(hframe)->methodSignature),
                           &(unhand(hframe)->pcAbsolute),
                           &(unhand(hframe)->lineno),
			   &(unhand(hframe)->pc),
			   &(unhand(hframe)->localVariables));

    return hframe;
}


HObject *
sun_tools_debug_Agent_getSlotObject(Hsun_tools_debug_Agent *this,
				     HObject *o, long slot)
{
    ClassClass *cb;
    struct fieldblock *fb, **slots;
    int nslots;

    if (o == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return NULL;
    }

    cb = get_classblock(o);
    if (makeslottable(cb) == -5) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return NULL;
    }
    nslots = cbSlotTableSize(cb);
    slots = cbSlotTable(cb);
    if (slot >= nslots || slot < 0 || (fb = slots[slot]) == 0
	    || fieldsig(fb)[0] != SIGNATURE_CLASS) {
	SignalError(0, JAVAPKG "IllegalArgumentException", "not an object");
	return NULL;
    }
    return (fb->access & ACC_STATIC
	    ? *(HObject **) normal_static_address(fb)
	    : (HObject *) obj_getoffset(o, fb->u.offset));
}

static struct fieldblock *
getFieldBlock (HObject *o, long slot) {
    ClassClass *cb;
    struct fieldblock *fb, **slots;
    int nslots;

    if (o == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return NULL;
    }

    cb = get_classblock(o);
    if (makeslottable(cb) == -5) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return NULL;
    }
    nslots = cbSlotTableSize(cb);
    slots = cbSlotTable(cb);
    if (slot >= nslots || slot < 0 || (fb = slots[slot]) == 0) {
	return NULL;
    }

    return fb;
}

static int signature2Type(int signature) {
    static struct {
	int sig;
	int type;
    } table[] = {
	SIGNATURE_ANY, -1,
	SIGNATURE_ARRAY, sun_tools_java_Constants_TC_ARRAY,
	SIGNATURE_BYTE, sun_tools_java_Constants_TC_BYTE,
	SIGNATURE_CHAR, sun_tools_java_Constants_TC_CHAR,
	SIGNATURE_CLASS, sun_tools_debug_AgentConstants_TP_OBJECT,
	SIGNATURE_ENUM, -1,
	SIGNATURE_FLOAT, sun_tools_java_Constants_TC_FLOAT,
	SIGNATURE_DOUBLE, sun_tools_java_Constants_TC_DOUBLE,
	SIGNATURE_FUNC, -1,
	SIGNATURE_INT, sun_tools_java_Constants_TC_INT,
	SIGNATURE_LONG, sun_tools_java_Constants_TC_LONG,
	SIGNATURE_SHORT, sun_tools_java_Constants_TC_SHORT,
	SIGNATURE_VOID, sun_tools_java_Constants_TC_VOID,
	SIGNATURE_BOOLEAN, sun_tools_java_Constants_TC_BOOLEAN,
	0
    };

    int i;

    for (i = 0; table[i].sig != 0; i++) {
	if (signature == table[i].sig) {
	    return table[i].type;
	}
    }
    return -1;
}


HArrayOfInt *
sun_tools_debug_Agent_getSlotSignature(Hsun_tools_debug_Agent * this,
					ClassClass * c, long slot)
{
    OBJECT 		*v;
    struct fieldblock	*fb;
    HArrayOfInt		*ret;
    ArrayOfInt		*val;
    int			i;
    char		*sig;

    fb = getFieldBlock((HObject *)c, slot);
    if (fb == 0) {
	return NULL;
    }

    sig = fieldsig(fb);
    ret = (HArrayOfInt *) ArrayAlloc(T_INT, strlen(sig));
    if (ret == 0) {
        SignalError(0, JAVAPKG "OutOfMemoryError", 0);
        return NULL;
    }
    val = (ArrayOfInt *)unhand(ret);
    for (i = 0; i < (int)strlen(sig); i++) {
	val->body[i] = signature2Type(sig[i]);
    }
    return ret;
}

long
sun_tools_debug_Agent_getSlotBoolean(Hsun_tools_debug_Agent * this,
				      HObject * o, long slot)
{
    OBJECT *v;
    struct fieldblock *fb;
    int sig;

    fb = getFieldBlock(o, slot);
    if (fb == 0) {
	return 0;
    }

    if (fieldsig(fb)[0] != SIGNATURE_BOOLEAN) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return 0;
    }
    v = (fb->access & ACC_STATIC
	 ? normal_static_address(fb)
	 : &obj_getoffset(o, fb->u.offset));
    return (long)*v;
}


long
sun_tools_debug_Agent_getSlotInt(Hsun_tools_debug_Agent * this,
				  HObject * o, long slot)
{
    OBJECT *v;
    struct fieldblock *fb;
    int sig;

    fb = getFieldBlock(o, slot);
    if (fb == 0) {
	return 0;
    }

    sig = fieldsig(fb)[0];
    v = (fb->access & ACC_STATIC
	 ? (sig == SIGNATURE_DOUBLE || sig == SIGNATURE_LONG
	    ? twoword_static_address(fb)
	    : normal_static_address(fb))
	 : &obj_getoffset(o, fb->u.offset));

    switch (sig) {
      case SIGNATURE_BOOLEAN:
      case SIGNATURE_BYTE:
      case SIGNATURE_CHAR:
      case SIGNATURE_SHORT:
      case SIGNATURE_INT:
	return (long)*v;
      case SIGNATURE_VOID:
	return 0;

      default:
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return 0;
    }
}

double
sun_tools_debug_Agent_getSlotDouble(Hsun_tools_debug_Agent * this,
				    HObject * o, long slot)
{
    OBJECT *v;
    struct fieldblock *fb;
    int sig;
    Java8 t;

    fb = getFieldBlock(o, slot);
    if (fb == 0) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return (float)0.0;
    }

    sig = fieldsig(fb)[0];
    v = (fb->access & ACC_STATIC
	 ? (sig == SIGNATURE_DOUBLE
	    ? twoword_static_address(fb)
	    : normal_static_address(fb))
	 : &obj_getoffset(o, fb->u.offset));

    if (sig == SIGNATURE_FLOAT) {
	return *(float *)v;
    } else if (sig == SIGNATURE_DOUBLE) {
	return GET_DOUBLE(t, v);
    } else {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return 0;
    }
}

int64_t
sun_tools_debug_Agent_getSlotLong(Hsun_tools_debug_Agent * this,
				   HObject * o, long slot)
{
    OBJECT *v;
    struct fieldblock *fb;
    int sig;
    Java8 t;

    fb = getFieldBlock(o, slot);
    if (fb == 0) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return ll_zero_const;
    }

    if (fieldsig(fb)[0] != SIGNATURE_LONG) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return ll_zero_const;
    }

    v = (fb->access & ACC_STATIC
	 ? twoword_static_address(fb)
	 : &obj_getoffset(o, fb->u.offset));
    return GET_INT64(t, v);
}

HArrayOfObject *
sun_tools_debug_Agent_getSlotArray(Hsun_tools_debug_Agent *this,
				    HObject *o, long slot)
{
    ClassClass *cb;
    struct fieldblock *fb, **slots;
    int nslots;

    if (o == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return NULL;
    }

    cb = get_classblock(o);
    if (makeslottable(cb) == -5) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return NULL;
    }
    nslots = cbSlotTableSize(cb);
    slots = cbSlotTable(cb);
    if (slot >= nslots || slot < 0 || (fb = slots[slot]) == 0
	    || fieldsig(fb)[0] != SIGNATURE_ARRAY) {
	SignalError(0, JAVAPKG "IllegalArgumentException", "not an object");
	return NULL;
    }
    return (HArrayOfObject *)(fb->access & ACC_STATIC
	    ? *(HObject **) normal_static_address(fb)
	    : (HObject *) obj_getoffset(o, fb->u.offset));
}

HArrayOfObject *
sun_tools_debug_Agent_getMethods(Hsun_tools_debug_Agent *this, ClassClass *c)
{
    int nmethods;
    struct methodblock *mb;
    HArrayOfObject *ret;
    HObject **retData;
    int i;
    Hsun_tools_debug_Field *hfield;
    struct fieldblock *fb;

    if (c == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return NULL;
    }

    nmethods = unhand(c)->methods_count;
    mb = cbMethods(c);
    ret = (HArrayOfObject *)ArrayAlloc(T_CLASS, nmethods);
    if (ret == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return NULL;
    }
    retData = (HObject **)unhand(ret)->body;

    retData[nmethods] = (HObject *)FindClass(0, "sun/tools/debug/Field", TRUE);
    for (i = 0; i < nmethods; i++, mb++) {
	fb = &(mb->fb);
	hfield = (Hsun_tools_debug_Field*)
	    execute_java_constructor(0, "sun/tools/debug/Field", 0, "()");
	if (!hfield) {
	    dprintf("agent.c: hfield==0\n");
	    SignalError(0, JAVAPKG "NullPointerException", 0);
            return NULL;
	}

	unhand(hfield)->slot = i;
	unhand(hfield)->name = makeJavaString(fieldname(fb),
					      strlen(fieldname(fb)));
	unhand(hfield)->signature = makeJavaString(fieldsig(fb),
						   strlen(fieldsig(fb)));
	unhand(hfield)->access = fb->access;
	unhand(hfield)->clazz = (HObject *)cbHandle(fieldclass(fb));
	retData[i] = (HObject *)hfield;
    }
    return ret;
}

HArrayOfObject *
sun_tools_debug_Agent_getFields(Hsun_tools_debug_Agent *this, ClassClass *c)
{
    int nslots = 0;
    int i;
    HArrayOfObject *ret;
    HObject **retData;
    struct fieldblock **slots;
    Hsun_tools_debug_Field *hfield;

    if (c == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return NULL;
    }

    if (makeslottable(c) == -5) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return NULL;
    }
    nslots = cbSlotTableSize(c);
    slots = cbSlotTable(c);
    ret = (HArrayOfObject *)ArrayAlloc(T_CLASS, nslots);
    if (ret == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return NULL;
    }
    retData = (HObject **)unhand(ret)->body;
    retData[nslots] = (HObject *)FindClass(0, "sun/tools/debug/Field", TRUE);

    for (i = 0; i < nslots; i++)
	if (slots[i]) {
	    hfield = (Hsun_tools_debug_Field*)
		execute_java_constructor(0, "sun/tools/debug/Field", 0, "()");
	    if (!hfield) {
		dprintf("agent.c: hfield==0\n");
		SignalError(0, JAVAPKG "NullPointerException", 0);
                return NULL;
	    }

	    unhand(hfield)->slot = i;
	    unhand(hfield)->name =
		makeJavaString(fieldname(slots[i]),
			       strlen(fieldname(slots[i])));
	    unhand(hfield)->signature =
		makeJavaString(fieldsig(slots[i]),
			       strlen(fieldsig(slots[i])));
	    unhand(hfield)->access = slots[i]->access;
	    unhand(hfield)->clazz = (HObject *)cbHandle(fieldclass(slots[i]));
	    retData[i] = (HObject *)hfield;
	}
    return ret;
}

HArrayOfObject *
sun_tools_debug_Agent_getClasses(Hsun_tools_debug_Agent *this)
{
    int i;
    ClassClass **pcb, *cb;
    HArrayOfObject *ret;
    HObject **retData;
    long nClasses;

    nClasses = get_nbinclasses();
    ret = (HArrayOfObject *)ArrayAlloc(T_CLASS, nClasses);
    if (ret == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return NULL;
    }
    retData = (HObject **)unhand(ret)->body;
    retData[nClasses] = (HObject *)get_classClass();

    pcb = get_binclasses();
    for (i = 0; i < nClasses; i++, pcb++) {
	cb = *pcb;
	retData[i] = (HObject *)cbHandle(cb);
    }
    return ret;
}

Hsun_tools_debug_LineNumber *
sun_tools_debug_Agent_lineno2pc(Hsun_tools_debug_Agent *self,ClassClass *clazz,long number)
{
    struct methodblock *mb; struct lineno *line;
    unsigned long start=(unsigned long)-1,end=0,last;
    Hsun_tools_debug_LineNumber *result; int i,j;
    if(!clazz) { SignalError(NULL,"java/lang/NullPointerException",NULL); return NULL; }
    result=(void*)execute_java_constructor(NULL,"sun/tools/debug/LineNumber",NULL,"()");
    if(!result) { SignalError(NULL,"java/lang/NullPointerException",NULL); return NULL; }
    mb=cbMethods(clazz);
    for(i=0;i<cbMethodsCount(clazz);++i,++mb) {
        if(!strcmp(fieldname(&mb->fb),"<clinit>")) continue;
        line=mb->line_number_table;
        for(j=0;j<mb->line_number_table_length;++j,++line)
            if(line->line_number==number) {
                last=(unsigned long)mb->code+(j+1<mb->line_number_table_length ? line[1].pc-1 : mb->code_length);
                if((unsigned long)(mb->code+line->pc)<start) start=(unsigned long)(mb->code+line->pc);
                if(last>end) end=last;
            }
    }
    if(start==(unsigned long)-1) return NULL;
    unhand(result)->clazz=clazz; unhand(result)->line_number=number;
    unhand(result)->startPC=start; unhand(result)->endPC=end;
    return result;
}

long
sun_tools_debug_Agent_method2pc(Hsun_tools_debug_Agent * this,
				 ClassClass * c, long method_slot)
{
    ClassClass *cls = (ClassClass *)c;
    struct methodblock *mb;

    if (method_slot < 0 || method_slot >= (long)(cbMethodsCount(cls))) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot index");
	return 0;
    }
    mb = cbMethods(cls) + method_slot;
    return (long)mb->code;
}

long
sun_tools_debug_Agent_pc2lineno(Hsun_tools_debug_Agent * this,
				ClassClass * c, long pc)
{
    ClassClass *cls = (ClassClass *)c;
    struct methodblock *mb = cbMethods(cls);
    struct lineno *ln;
    int nMethods, nLines;
    int i, j;
    Hsun_tools_debug_LineNumber *hline;

    nMethods = (int)(cbMethodsCount(cls));
    while (nMethods > 0) {
	if (pc >= (long)mb->code && pc < (long)(mb->code + mb->code_length) && strcmp(fieldname(&mb->fb),"<clinit>")) {
	    nLines = mb->line_number_table_length;
	    ln = mb->line_number_table;
	    while (nLines > 0) {
	        if (pc >= (long)(mb->code + ln->pc)) {
		    if (nLines > 1) {
		        if (pc < (long)(mb->code +(ln+1)->pc)) {
			    return ln->line_number;
                        }
		    } else {
		        return ln->line_number;
		    }
	        }
	        nLines--;
	        ln++;
	    }
        }
	nMethods--;
	mb++;
    }
    return -1;
}

HString *
sun_tools_debug_Agent_exceptionStackTrace(Hsun_tools_debug_Agent *this,
					   Hjava_lang_Throwable *exc) {
    int         n;
    HString *hstr, *hret;
    char *p;
    char buf[128];
    HString *hstring;
    char outbuf[2048];
    struct HArrayOfInt *backtrace = (HArrayOfInt *)(unhand(exc)->backtrace);
    int32_t *real_data = unhand(backtrace)->body;
    unsigned char **data = (unsigned char **)(real_data);
    unsigned char **enddata =
	(unsigned char **)(real_data + obj_length(backtrace));

    p = classname2string(cbName(obj_classblock(exc)), buf, sizeof(buf));
    strcpy(outbuf, p);
    hstr = unhand(exc)->detailMessage;
    if (hstr) {
	strcat(outbuf, " ");
	strcat(outbuf, javaString2CString(hstr, buf, sizeof(buf)));
        strcat(outbuf, "\n");
    }
    for (; data < enddata; data++) {
	if (*data != 0) {
	    char        where[100];
	    pc2string(*data, 0, where, where + sizeof where);
            if ((int)(strlen(outbuf) + strlen(where) + 5) >= 2048) {
                break;	/* out of buffer space, return what's here */
            }
	    strcat(outbuf, "\tat ");
	    strcat(outbuf, where);
            strcat(outbuf, "\n");
	}
    }
    return makeJavaString(outbuf, strlen(outbuf));
}

HString *
sun_tools_debug_Agent_getClassSourceName(Hsun_tools_debug_Agent *this, ClassClass *c) {
    char *name = unhand(c)->source_name;
    if (name == 0) {
        name = "";
    }
    return makeJavaString(name, strlen(name));
}

void
sun_tools_debug_Agent_setSlotBoolean(Hsun_tools_debug_Agent *this,
                                     HObject *o, long slot, long value)
{
    long *v;
    struct fieldblock *fb;
    int sig;

    fb = getFieldBlock(o, slot);
    if (fb == 0 || fieldsig(fb)[0] != SIGNATURE_BOOLEAN) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return;
    }
    v = (long*)(fb->access & ACC_STATIC
	 ? normal_static_address(fb)
	 : &obj_getoffset(o, fb->u.offset));
    *v = value;
}

void
sun_tools_debug_Agent_setSlotInt(Hsun_tools_debug_Agent *this,
                                     HObject *o, long slot, long value)
{
    long *v;
    struct fieldblock *fb;
    int sig;

    fb = getFieldBlock(o, slot);
    if (fb == 0) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot");
	return;
    }
    sig =  fieldsig(fb)[0];
    if (sig != SIGNATURE_BYTE && sig != SIGNATURE_SHORT &&
        sig != SIGNATURE_INT && sig != SIGNATURE_CHAR) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return;
    }
    v = (long*)(fb->access & ACC_STATIC
	 ? normal_static_address(fb)
	 : &obj_getoffset(o, fb->u.offset));
    *v = value;
}

void
sun_tools_debug_Agent_setSlotLong(Hsun_tools_debug_Agent *this,
                                     HObject *o, long slot, int64_t value)
{
    OBJECT *v;
    struct fieldblock *fb;
    int sig;
    Java8 t;

    fb = getFieldBlock(o, slot);
    if (fb == 0 || fieldsig(fb)[0] != SIGNATURE_LONG) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return;
    }

    v = (fb->access & ACC_STATIC
	 ? twoword_static_address(fb)
	 : &obj_getoffset(o, fb->u.offset));
    SET_INT64(t, v, value);
}

void
sun_tools_debug_Agent_setSlotDouble(Hsun_tools_debug_Agent *this,
                                    HObject *o, long slot, double value)
{
    OBJECT *v;
    struct fieldblock *fb;
    int sig;
    Java8 t;

    fb = getFieldBlock(o, slot);
    if (fb == 0) {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return;
    }

    sig = fieldsig(fb)[0];
    v = (fb->access & ACC_STATIC
	 ? (sig == SIGNATURE_DOUBLE
	    ? twoword_static_address(fb)
	    : normal_static_address(fb))
	 : &obj_getoffset(o, fb->u.offset));

    if (sig == SIGNATURE_FLOAT) {
	*(float *)v = (float)value;
    } else if (sig == SIGNATURE_DOUBLE) {
        SET_DOUBLE(t, v, value);
    } else {
	SignalError(0, JAVAPKG "IllegalAccessError","invalid slot type");
	return;
    }
}

static int compare_lines(const void *a,const void *b)
{
    long x=*(const long*)a,y=*(const long*)b;
    return x<y ? -1 : x>y;
}
HArrayOfInt *sun_tools_debug_Agent_getLinenumbers(Hsun_tools_debug_Agent *self,ClassClass *clazz)
{
    struct methodblock *mb; struct lineno *line;
    HArrayOfInt *result; long count=0,previous=-1; int pass,i,j;
    for(pass=0;pass<2;++pass) {
        count=0; previous=-1; mb=cbMethods(clazz);
        for(i=0;i<cbMethodsCount(clazz);++i,++mb) {
            if(!strcmp(fieldname(&mb->fb),"<clinit>")) continue;
            line=mb->line_number_table;
            for(j=0;j<mb->line_number_table_length;++j,++line)
                if(line->line_number!=previous) {
                    if(pass) unhand(result)->body[count]=line->line_number;
                    ++count; previous=line->line_number;
                }
        }
        if(!pass) {
            result=(HArrayOfInt*)ArrayAlloc(T_INT,count);
            if(!result) { SignalError(NULL,"java/lang/OutOfMemoryError",NULL); return NULL; }
        }
    }
    qsort(unhand(result)->body,count,sizeof(long),compare_lines);
    return result;
}
long sun_tools_debug_Agent_getMethodLinenumber(Hsun_tools_debug_Agent *self,ClassClass *clazz,long slot)
{
    struct methodblock *mb; unsigned long minpc=(unsigned long)-1; long result=-1; int i;
    if(!clazz) { SignalError(NULL,"java/lang/NullPointerException",NULL); return 0; }
    if(slot<0 || slot>=cbMethodsCount(clazz)) return -1;
    mb=cbMethods(clazz)+slot;
    for(i=0;i<mb->line_number_table_length;++i)
        if(mb->line_number_table[i].pc<minpc) {
            minpc=mb->line_number_table[i].pc;
            result=mb->line_number_table[i].line_number;
        }
    return result;
}
