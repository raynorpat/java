/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/*
 * @(#)classloader.c	1.79 95/11/16  
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
 *      Routines for loading and resolving class definitions.
 *      These routines should not depending up on the interpreter or
 *      garbage collector.
 *
 */

#include <string.h>
#include <sys/types.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <setjmp.h>

#include "oobj.h"
#include "path.h"
#include "interpreter.h"
#include "monitor.h"
#include "tree.h"
#include "bool.h"
#include "signature.h"

#ifdef XP_MAC
#include "sysmacros_md.h"
#endif

#include "sys_api.h"
#include "byteorder_md.h"
#include "io_md.h"


int verbose_class_loading;
extern char *CompiledCodeAttribute;
extern ClassClass *allocClassClass(void);

ClassClass *LoadFile(char *fn, char *dir, char *SourceHint, char *expectedName);
ClassClass *DoImport(char *name, char *SourceHint);
char *stat_source(ClassClass *cb, struct stat *s, char *pathbuf, int maxlen);

/* XXX: Should move declaration to interpreter.h... and include that header file */
extern ClassClass * FindPreviouslyLoadedClass(char *name, struct Hjava_lang_ClassLoader *loader);


/* Primary 1000d670/1000d797 serialize registry mutations with GC. */
void AddBinClass(ClassClass *cb)
{
    long i = -1, capacity;
    ClassClass **classes;
    BINCLASS_LOCK();
    if (cbLoader(cb) == NULL) {
        for (i = nbinclasses - 1; i >= 0; --i)
            if (binclasses[i] && !strcmp(cbName(binclasses[i]), cbName(cb)))
                break;
    }
    if (i < 0) {
        if (nbinclasses >= sizebinclasses) {
            capacity = binclasses ? nbinclasses * 2 : 50;
            classes = sysRealloc(binclasses, capacity * sizeof(*classes));
            if (!classes) {
                BINCLASS_UNLOCK();
                SignalError(EE(), JAVAPKG "OutOfMemoryError", NULL);
                return;
            }
            binclasses = classes;
            sizebinclasses = capacity;
        }
        i = nbinclasses++;
    }
    binclasses[i] = cb;
    BINCLASS_UNLOCK();
}

void DelBinClass(ClassClass *cb)
{
    long i;
    BINCLASS_LOCK();
    for (i = nbinclasses - 1; i >= 0; --i) {
        if (binclasses[i] == cb) {
            binclasses[i] = binclasses[--nbinclasses];
            break;
        }
    }
    BINCLASS_UNLOCK();
}

/* LoadFile is shared by the compiler and the interpreter.  It does
   different things for each.

   When the compiler askes to load a file for a particular class,
   in the end all it's looking for is a classinfo structure to be
   created for that class.  It can get that either by loading a
   .class file, and creating the classinfo from that, OR, by
   reading a source file and compiling it.

   When the interpreter askes for a file, in the end it's looking
   for a classblock structure to be created.  The only way it can
   get one of those is by loading a compiled class.

   OpenCode tries to open a .class file first.  -2 means it failed
   because it couldn't open the file, -1 means the requested file
   is being compiled.  Of course, in the case of the interpreter,
   OpenCode never returns -1, only a valid fd or -2.

   If OpenCode returns a valid fd, that means it is a .class file.
   If SkipCheckSource is false (meaning we're checking source
   files), and the source file as specified in the compiled .class
   file is more recent than the .class file, then the loaded class
   is thrown away, and OpenCode is called again, only this time,
   with arguments which will cause it to recompile the source.  In
   the case of java, it forks javac to do this.  When this happens,
   it has to reload the resulting .class file.

   Returns 1 when it succeeds, 0 when it fails. */

extern int OpenCode(char *, char *, char *, struct stat*);
/* Primary 1000d9a8/1000dc66/1000d836 pass the managed handle back to
 * the resolver; a boolean success would leave a collectible registry-only gap. */
ClassClass *
LoadFile(char *fn, char *dir, char *SourceHint, char *expectedName)
{
    struct stat st;
    ClassClass *cb = 0;
    int codefd = -1;
    int	retryCount = 0;
    unsigned char *external_class;
    char *detail;

    codefd = OpenCode(fn, SourceHint, dir, &st);

again:
    switch (codefd) {
      case -2:		    /* open failed */
	return 0;

      case -1:
	return NULL;    /* no class handle supplied by the legacy source hook */
    }

    /* Snarf the file into memory. */
    external_class = (unsigned char *)sysMalloc(((size_t)st.st_size));
    if (external_class == 0)
        goto failed;
    if (sysRead(codefd, (char *)external_class, st.st_size) != st.st_size)
        goto failed;
    sysClose(codefd);
    codefd = -1;
    cb = allocClassClass();
    if (!cb || !createInternalClass(external_class, external_class + st.st_size,
                                    cb, NULL, expectedName, &detail)) {
	sysFree(external_class);
	goto failed;
    }
    sysFree(external_class);

    
    /* Now, if we're checking sources, and there's a source file
       name in the .class file we just loaded, check to see if we
       should recompile this class.  If so, call try again. */

    if (!SkipSourceChecks && cbSourceName(cb)) {
	struct stat st2;
	char pathbuf[255];

	if (cbMajorVersion(cb) != JAVA_VERSION) {
	    fprintf(stderr, "Warning: Not using %s because it was\n"
		    "\t compiled with version %d.%d of javac.  "
		    "Current version is %d.%d.\n",
		    cbName(cb),
		    cbMajorVersion(cb), cbMinorVersion(cb), 
		    JAVA_VERSION, JAVA_MINOR_VERSION);
	}
	if (stat_source(cb, &st2, pathbuf, sizeof(pathbuf)) != 0
	    && (st2.st_mtime > st.st_mtime || cbMajorVersion(cb) != JAVA_VERSION)
	    && ++retryCount < 2) {
	    if (cbMajorVersion(cb) != JAVA_VERSION) {
		fprintf(stderr, "Warning: Attempting to recompile from %s...\n",
			cbSourceName(cb));
		unlink(fn);
	    }
	    /* remove the class we just defined - we're going again;  */
	    codefd = OpenCode(fn, pathbuf, dir, &st);
	    DelBinClass(cb);
            FreeClass(cb);
	    goto again;
	} else if (cbMajorVersion(cb) != JAVA_VERSION) {
	    /* remove the class we just defined - we're going
	       to try again (once) */
	    DelBinClass(cb);
            FreeClass(cb);
	    goto failed;
	}
    }
#ifdef DEBUG
    if (verbose) {
	fprintf(stderr, "[Loaded %s]\n", fn);
    } else if (verbose_class_loading) {
	fprintf(stderr, "Loaded %s\n", fn);
    }
#endif
    return cb;
failed:
    if (codefd >= 0)
	sysClose(codefd);
    return 0;
}

ClassClass *
LoadFileZip(char *fn, zip_t *zip, char *expectedName)
{
    struct stat st;
    ClassClass *cb;
    unsigned char *clazz;
    char *detail;

    if (!zip_stat(zip, fn, &st)) {
		return 0;
    }
    if ((clazz = sysMalloc((size_t)st.st_size)) == 0) {
		return 0;
    }
    if (!zip_get(zip, fn, clazz, st.st_size)) {
	sysFree(clazz);
		return 0;
    }
    if ((cb = allocClassClass()) == 0) {
		sysFree(clazz);
		return 0;
    }
    if (!createInternalClass(clazz, clazz + st.st_size, cb, NULL, expectedName, &detail)) {
		sysFree(clazz);
		/* sysFree(cb); */
		return 0;
    }
    sysFree(clazz);
#ifdef DEBUG
    if (verbose) {
	fprintf(stderr, "[Loaded %s from %s]\n", fn, zip->fn);
    } else if (verbose_class_loading) {
	fprintf(stderr, "ZipLoaded %s #%ld\n", fn, nbinclasses-1);
    }
#endif
    return cb;
}

ClassClass *
DoImport(char *name, char *SourceHint)
{
    cpe_t **cpp;
    ClassClass *cb;

    if (name[0] == SIGNATURE_ARRAY) {
        /* Compatibility entry for old local-load callers; the resolver
         * normally supplies these decoded arguments directly. */
        char *part = name, *component;
        int depth = 0, base_type;
        size_t length;
        ClassClass *inner = NULL;
        while (*part == SIGNATURE_ARRAY) { ++depth; ++part; }
        switch (*part) {
        case SIGNATURE_BOOLEAN: base_type = T_BOOLEAN; break;
        case SIGNATURE_BYTE: base_type = T_BYTE; break;
        case SIGNATURE_CHAR: base_type = T_CHAR; break;
        case SIGNATURE_SHORT: base_type = T_SHORT; break;
        case SIGNATURE_INT: base_type = T_INT; break;
        case SIGNATURE_LONG: base_type = T_LONG; break;
        case SIGNATURE_FLOAT: base_type = T_FLOAT; break;
        case SIGNATURE_DOUBLE: base_type = T_DOUBLE; break;
        case SIGNATURE_CLASS:
            base_type = T_CLASS;
            length = strlen(++part);
            if (!length || part[length - 1] != SIGNATURE_ENDCLASS) return 0;
            component = sysMalloc(length);
            if (!component) return 0;
            memcpy(component, part, length - 1);
            component[length - 1] = 0;
            inner = FindClassFromClass(EE(), component, TRUE, NULL);
            sysFree(component);
            if (!inner) return 0;
            break;
        default: return 0;
        }
        return createFakeArrayClass(name, base_type, depth, inner, NULL);
    }
    if (name[0] == DIR_SEPARATOR) {
	return 0;
    }
    for (cpp = sysGetClassPath(); cpp && *cpp != 0; cpp++) {
	cpe_t *cpe = *cpp;
	char path[255];
	if (cpe->type == CPE_DIR) {
	    if (jio_snprintf(path, sizeof(path), "%s%c%s." JAVAOBJEXT, cpe->u.dir,
			 LOCAL_DIR_SEPARATOR, name) == -1) {
		return 0;
	    }
	    if ((cb = LoadFile(sysNativePath(path), cpe->u.dir, SourceHint, name)) != NULL) {
		return cb;
	    }
	} else if (cpe->type == CPE_ZIP) {
	    if (jio_snprintf(path, sizeof(path), "%s." JAVAOBJEXT, name) == -1) {
		return 0;
	    }
	    if ((cb = LoadFileZip(path, cpe->u.zip, name)) != NULL) {
		return cb;
	    }
	}
    }
    return 0;
}

char *
stat_source(ClassClass *cb, struct stat *s, char *pathbuf, int maxlen)
{
#define NAMEBUFLEN 255
    char nm[NAMEBUFLEN];
    char *p, *q, *lp;
    cpe_t **cpp;

    /* don't bother searching if absolute */
    /* REMIND: only here for compatibility */
    if (sysIsAbsolute(cbSourceName(cb))) {
	if (sysStat(cbSourceName(cb), s) == 0) {
	    if (jio_snprintf(pathbuf, maxlen, "%s", cbSourceName(cb)) == -1) {
		return 0;
	    }
	    return pathbuf;
	} else {
	    return 0;
	}
    }

    /* parse the package name */
    p = cbName(cb);
    if (strlen(p) > NAMEBUFLEN - 1) {
	return 0;
    }
    for (q = lp = nm ; *p ; p++) {
	if (*p == DIR_SEPARATOR) {
	    *q++ = LOCAL_DIR_SEPARATOR;
	    lp = q;
	} else {
	    *q++ = *p;
	}
    }

    /* append the source file name */
    p = cbSourceName(cb);
    if (strlen(p) + (lp - nm) > NAMEBUFLEN - 1) {
	return 0;
    }
    for (; *p ; p++) {
	*lp++ = (*p == DIR_SEPARATOR ? LOCAL_DIR_SEPARATOR : *p);
    }
    *lp = '\0';

    /* search the class path */
    for (cpp = sysGetClassPath() ; cpp && *cpp != 0 ; cpp++) {
	cpe_t *cpe = *cpp;
	if (cpe->type == CPE_DIR) {
	    if (jio_snprintf(pathbuf, maxlen, "%s%c%s",
			 cpe->u.dir, LOCAL_DIR_SEPARATOR, nm) == -1) {
		return 0;
	    }
	    if (sysStat(pathbuf, s) == 0) {
		return pathbuf;
	    }
	}
    }
    return 0;
}

struct CICcontext {
    unsigned char *ptr;
    unsigned char *end_ptr;
    ClassClass *cb;
    jmp_buf jump_buffer;
    char **detail;
    int pass;
    size_t main_size, clinit_size;
    int in_clinit;
    struct CICallocation *temporary;
    unsigned char *main_base, *main_next;
    unsigned char *clinit_base, *clinit_next;
};

/* First-pass allocations have an eight-byte prefix on the target ABI. */
struct CICallocation {
    struct CICallocation *next;
    unsigned long spare;
};

typedef struct CICcontext CICcontext;

static unsigned long get1byte(CICcontext *);
static unsigned long get2bytes(CICcontext *);
static unsigned long get4bytes(CICcontext *);
#define get1byteI8(x)\
	((char)(get1byte(x)))
#define get1byteU8(x)\
	((unsigned char)(get1byte(x)))
#define get2bytesI16(x)\
	((short)(get2bytes(x)))
#define get2bytesU16(x)\
	((unsigned short)(get2bytes(x)))
#define get4bytesI32(x)\
	((int32_t)(get4bytes(x)))
static char *getAsciz(CICcontext *);
static char *getAscizFromClass(CICcontext *, unsigned int i);
static void ValidateClassName(CICcontext *context, char *name) ;
static void getNbytes(CICcontext *, int32_t count, char *buffer);
static bool_t LoadConstantPool(CICcontext *);
static bool_t ReadInCode(CICcontext *, struct methodblock *);
static bool_t ReadLineTable(CICcontext *, struct methodblock *mb);
static bool_t ReadLocalVars(CICcontext *, struct methodblock *mb);
#ifdef JCOV
static void ReadCoverageTable(CICcontext *, struct methodblock *);
#endif
static bool_t InitializeStaticVar(struct fieldblock *fb, CICcontext *context);


#undef  ERROR		/* Avoid conflict on PC */
#if defined(DEBUG_jar) && defined(XP_UNIX)
#define ERROR(name) { *(context->detail) = name; \
                        printf("classloader.c ERROR: %s\n", name );\
                        longjmp(context->jump_buffer, 1); }
#else
#define ERROR(name) { *(context->detail) = name; \
                        longjmp(context->jump_buffer, 1); }
#endif

static void CICfree(CICcontext *context)
{
    struct CICallocation *item, *next;
    if (context->pass == 1) {
        for (item = context->temporary; item; item = next) {
            next = item->next;
            sysFree(item);
        }
        context->temporary = NULL;
    } else {
        sysFree(context->main_base);
        sysFree(context->clinit_base);
        context->main_base = context->clinit_base = NULL;
    }
}

static void *CICalloc(CICcontext *context, size_t size)
{
    struct CICallocation *item;
    unsigned char *result;
    size_t aligned;
    /* deviation: reject size overflow rather than corrupting the slabs. */
    if (size > (size_t)-1 - 8) ERROR("Out of memory");
    aligned = (size + 7) & ~(size_t)7;
    if (context->pass == 1) {
        item = sysCalloc(1, sizeof(*item) + size);
        if (!item) ERROR("Out of memory");
        item->next = context->temporary;
        context->temporary = item;
        if (context->in_clinit) {
            if (context->clinit_size > (size_t)-1 - aligned) ERROR("Out of memory");
            context->clinit_size += aligned;
        } else {
            if (context->main_size > (size_t)-1 - aligned) ERROR("Out of memory");
            context->main_size += aligned;
        }
        return item + 1;
    }
    if (context->in_clinit) {
        result = context->clinit_next;
        context->clinit_next += aligned;
        sysAssert(context->clinit_next <= context->clinit_base + context->clinit_size);
    } else {
        result = context->main_next;
        context->main_next += aligned;
        sysAssert(context->main_next <= context->main_base + context->main_size);
    }
    return result;
}

static bool_t ParseInternalClass(CICcontext *, ClassClass *,
                                struct Hjava_lang_ClassLoader *, char *);

bool_t createInternalClass(unsigned char *bytes, unsigned char *limit,
                          ClassClass *cb, struct Hjava_lang_ClassLoader *loader,
                          char *utfname, char **detail)
{
    CICcontext context_block;
    CICcontext *context = &context_block;
    memset(context, 0, sizeof(*context));
    context->ptr = bytes;
    context->end_ptr = limit;
    context->cb = cb;
    context->detail = detail;
    context->pass = 1;
    *detail = NULL;
    if (setjmp(context->jump_buffer)) {
        CICfree(context);
        /* Do not leave dangling native metadata in the managed class body. */
        cbConstantPool(cb) = NULL;
        cbFields(cb) = NULL;
        cbMethods(cb) = NULL;
        cbImplements(cb) = NULL;
        cbName(cb) = cbSuperName(cb) = cbSourceName(cb) = NULL;
        cbConstantPoolCount(cb) = cbFieldsCount(cb) = cbMethodsCount(cb) = 0;
        cbImplementsCount(cb) = 0;
        return FALSE;
    }
    if (!ParseInternalClass(context, cb, loader, utfname)) ERROR("Out of memory");
    cbConstantPool(cb) = NULL;
    cbFields(cb) = NULL;
    cbMethodsCount(cb) = 0;
    CICfree(context);
    context->ptr = bytes;
    context->pass = 2;
    context->main_base = sysCalloc(1, context->main_size);
    if (!context->main_base) ERROR("Out of memory");
    if (context->clinit_size) {
        context->clinit_base = sysCalloc(1, context->clinit_size);
        if (!context->clinit_base) ERROR("Out of memory");
    }
    context->main_next = context->main_base;
    context->clinit_next = context->clinit_base;
    if (!ParseInternalClass(context, cb, loader, utfname)) ERROR("Out of memory");
    AddBinClass(cb);
    return TRUE;
}

static bool_t
ParseInternalClass(CICcontext *context, ClassClass *cb,
                   struct Hjava_lang_ClassLoader *loader, char *expected_name)
{
    int32_t i, j;
    union cp_item_type *constant_pool;
    unsigned char *type_table;
    unsigned short attribute_count;
    unsigned fields_count;
    struct methodblock *mb;
    struct fieldblock *fb;
    cbLoader(cb) = loader;

    if (get4bytes(context) != JAVA_CLASSFILE_MAGIC) 
	ERROR("Bad magic number");

    cbMinorVersion(cb) = get2bytesU16(context);
    cbMajorVersion(cb) = get2bytesU16(context);
    if (cbMajorVersion(cb) != JAVA_VERSION) 
	ERROR("Bad major version number");

    if (!LoadConstantPool(context)) {
 	SignalError(EE(), JAVAPKG "OutOfMemoryError", NULL);
  	return FALSE;
    }
    constant_pool = cbConstantPool(cb);
    type_table = constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p;

    cbAccess(cb) = get2bytesU16(context) & ACC_WRITTEN_FLAGS;

    /* Get the name of the class */
    i = get2bytesU16(context);	/* index in constant pool of class */
    cbName(cb) = getAscizFromClass(context, i);
    if (expected_name && strcmp(expected_name, cbName(cb))) {
            /* printf("Mismatch: Inner name: %s, file name %s\n", cbName(cb), expected_name); */
            ERROR("Internal/External class name mismatch");
    }
    ValidateClassName(context, cbName(cb));

    if (cbLoader(cb) && FindPreviouslyLoadedClass(cbName(cb), cbLoader(cb))) {
            ERROR("Class already loaded");  /* internal bug (unicode? race?) + (probable) security attack */
    }

    constant_pool[i].p = cb;
    CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED(type_table, i);

    /* Get the super class name. */
    i = get2bytesU16(context);	/* index in constant pool of class */
    if (i > 0) {
	cbSuperName(cb) = getAscizFromClass(context, i);
        /* printf("%s super class  = %s\n", cbName(cb), 
               cbSuperName(cb)? cbSuperName(cb) : "Missing name"); */
        if (cbSuperName(cb))
          ValidateClassName(context, cbSuperName(cb));
    }
    else
      /* should be strcmp cbName(cb) with java/lang/Object?? */
      ;

    i = cbImplementsCount(cb) = get2bytesU16(context);
    if (i > 0) {
	uint32_t j;
	uint32_t sizeOfAllocation = i * sizeof(short);
	
	cbImplements(cb) = CICalloc(context, sizeOfAllocation);
	if (cbImplements(cb) == NULL) {
	    SignalError(EE(), JAVAPKG "OutOfMemoryError", NULL);
  	    return FALSE;
   	}
   	else {
   	    memset(cbImplements(cb), 0, sizeOfAllocation);
   	}

	for (j = 0; j < i; j++) {
            unsigned int temp =
              cbImplements(cb)[j] = get2bytesI16(context); /* XXX: We should use get2bytesU16 and "implements" should be unsigned int */
            /* printf(" %s has interface %s\n", cbName(cb), getAscizFromClass(context, temp)); */
            ValidateClassName(context, getAscizFromClass(context, temp));
	}
    }

    fields_count = cbFieldsCount(cb) = get2bytesU16(context);
    if (fields_count > 0) { 
 	uint32_t sizeOfAllocation = cbFieldsCount(cb) * sizeof(struct fieldblock);
 	
 	cbFields(cb) = CICalloc(context, sizeOfAllocation);
	if (cbFields(cb) == NULL) {
	    SignalError(EE(), JAVAPKG "OutOfMemoryError", NULL);
  	    return FALSE;
	}
	else {
	    memset(cbFields(cb), 0, sizeOfAllocation);
	}
	
    }
    for (i = fields_count, fb = cbFields(cb); --i >= 0; fb++) {
	fieldclass(fb) = cb;
	fb->access = get2bytesU16(context) & ACC_WRITTEN_FLAGS;
	fb->name = getAsciz(context);
	fb->signature = getAsciz(context);
	attribute_count = get2bytesU16(context);
	for (j = 0; j < (int32_t) attribute_count; j++) {
	    char *name = getAsciz(context);
	    int32_t length = get4bytesI32(context);
	    if ((fb->access & ACC_STATIC) 
		 && strcmp(name, "ConstantValue") == 0) {
		if (length != 2) 
		    ERROR("Wrong size for VALUE attribute");
		fb->access |= ACC_VALKNOWN;
		/* we'll change this below */
		fb->u.offset = get2bytesU16(context); 
	    } else {
		getNbytes(context, length, NULL);
	    }
	}
	if (fb->access & ACC_STATIC) {
	    if (!InitializeStaticVar(fb, context)) {
		SignalError(EE(), JAVAPKG "OutOfMemoryError", NULL);
  	 	return FALSE;
	    }
	}
    }

    if ((cbMethodsCount(cb) = get2bytesU16(context)) > 0) {
 	uint32_t sizeOfAllocation = cbMethodsCount(cb) * sizeof(struct methodblock);
 	
 	cbMethods(cb) = CICalloc(context, sizeOfAllocation);
	if (cbMethods(cb) == NULL) {
	    SignalError(EE(), JAVAPKG "OutOfMemoryError", NULL);
 	    return FALSE;
 	}
	else {
	    memset(cbMethods(cb), 0, sizeOfAllocation);
	}

    }
    for (i = cbMethodsCount(cb), mb = cbMethods(cb); --i >= 0; mb++) {
	fieldclass(&mb->fb) = cb;
	mb->fb.access = get2bytesU16(context) & ACC_WRITTEN_FLAGS;
	mb->fb.name = getAsciz(context);
	mb->fb.signature = getAsciz(context);
	context->in_clinit = !strcmp(mb->fb.name, "<clinit>") &&
            !strcmp(mb->fb.signature, "()V");
	mb->args_size = Signature2ArgsSize(mb->fb.signature) 
	                + ((mb->fb.access & ACC_STATIC) ? 0 : 1);
	if (mb->args_size > 255) ERROR("Too many arguments");
	attribute_count = get2bytesU16(context);
	for (j = 0; j < (int32_t) attribute_count; j++) {
	    char *name = getAsciz(context);
	    if (CompiledCodeAttribute && !strcmp(name, CompiledCodeAttribute) &&
                cbLoader(cb) == NULL) {
                int32_t length = get4bytes(context);
                ReadInCompiledCode(context, mb, length, get1byte, get2bytes,
                                   get4bytes, getNbytes);
	    } else if ((strcmp(name, "Code") == 0) 
		&& ((mb->fb.access & (ACC_NATIVE | ACC_ABSTRACT))==0)) {
			if (!ReadInCode(context, mb)) {
	  		     SignalError(EE(), JAVAPKG "OutOfMemoryError", NULL);
			     return FALSE;
			}
	    } else if (strcmp(name, "Exceptions") == 0) {
                int32_t length = get4bytes(context);
                unsigned int k, index;
                mb->nexceptions = get2bytesU16(context);
                if (length != 2 + 2 * mb->nexceptions)
                    ERROR("Wrong size for Exceptions attribute");
                if (mb->nexceptions) {
                    mb->exceptions = CICalloc(context, mb->nexceptions * sizeof(unsigned short));
                    if (mb->exceptions == NULL) {
                        SignalError(EE(), JAVAPKG "OutOfMemoryError", NULL);
                        return FALSE;
                    }
                }
                for (k = 0; k < mb->nexceptions; k++) {
                    index = get2bytesU16(context);
                    if (index == 0 || index >= cbConstantPoolCount(cb) ||
                        (CONSTANT_POOL_TYPE_TABLE_GET_TYPE(
                            cbConstantPool(cb)[0].p, index)) != CONSTANT_Class)
                        ERROR("Bad Exceptions constant pool index");
                    mb->exceptions[k] = (unsigned short)index;
                }
	    } else {
		int32_t length = get4bytes(context);
		getNbytes(context, length, NULL);
	    }
	}
    }

    context->in_clinit = 0;

    /* See if there are class attributes */
    attribute_count = get2bytesU16(context); 
    for (j = 0; j < (int32_t) attribute_count; j++) {
	char *name = getAsciz(context);
	int32_t length = get4bytes(context);
	if (strcmp(name, "SourceFile") == 0) {
	    if (length != 2) {
		ERROR("Wrong size for VALUE attribute");
	    }
	    cbSourceName(cb) = getAsciz(context);
#ifdef JCOV
        } else if (!strcmp(name, "AbsoluteSourcePath") && length == 2) {
            cbAbsoluteSourceName(cb) = getAsciz(context);
        } else if (!strcmp(name, "TimeStamp") && length == 8) {
            unsigned long high = get4bytes(context);
            unsigned long low = get4bytes(context);
            cbTimestamp(cb) = ll_add(ll_shl(uint2ll(high), 32), uint2ll(low));
#endif
	} else {
	    getNbytes(context, length, NULL);
	}
    }
    return TRUE;
}

static bool_t InitializeStaticVar(struct fieldblock *fb, CICcontext *context) {
#if defined(HAVE_ALIGNED_DOUBLES)
    Java8 t1, t2;
#endif
    char isig = fb->signature[0];
    if (fb->access & ACC_VALKNOWN) { 
	ClassClass *cb = context->cb;
	unsigned int index = fb->u.offset; /* set by "VALUE" attribute */
	union cp_item_type *constant_pool = cbConstantPool(cb);
	unsigned char *type_table = 
	      constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p;

	if (index <= 0 || index >= cbConstantPoolCount(cb)) 
	    ERROR("Bad initial value");
	switch (isig) { 
	    case SIGNATURE_DOUBLE: 
		if (type_table[index] != 
		    (CONSTANT_Double | CONSTANT_POOL_ENTRY_RESOLVED))
		    ERROR("Bad index into constant pool");
		twoword_static_address(fb) = CICalloc(context, (size_t)(2 * sizeof(int32_t)));
		if (twoword_static_address(fb) == NULL)
			return FALSE;
		SET_DOUBLE(t1, twoword_static_address(fb),
			   GET_DOUBLE(t2, &constant_pool[index]));
	    break;

	    case SIGNATURE_LONG: 
		if (type_table[index] != 
		    (CONSTANT_Long | CONSTANT_POOL_ENTRY_RESOLVED))
		    ERROR("Bad index into constant pool");
		twoword_static_address(fb) = CICalloc(context, (size_t)(2 * sizeof(int32_t)));
		if (twoword_static_address(fb) == NULL)
			return FALSE;
		SET_INT64(t1, twoword_static_address(fb),
			  GET_INT64(t2, &constant_pool[index]));
		break;

	    case SIGNATURE_CLASS:
		if (strcmp(fb->signature, "L" JAVAPKG "String;") != 0) {
		    ERROR("Cannot set initial value for object");
		} else if (!ResolveClassStringConstant(cb, index, 0)) { 
		    ERROR("Unable to resolve string");
		} else {
		    *(void **)normal_static_address(fb) = 
			   constant_pool[index].p;
		}
		break;

	    case SIGNATURE_BYTE: 
		if (type_table[index] != 
		    (CONSTANT_Integer | CONSTANT_POOL_ENTRY_RESOLVED))
		    ERROR("Bad index into constant pool");
		*(int *)normal_static_address(fb) = 
		        (signed char)(constant_pool[index].i);
		break;

	    case SIGNATURE_CHAR:
		if (type_table[index] != 
		    (CONSTANT_Integer | CONSTANT_POOL_ENTRY_RESOLVED))
		    ERROR("Bad index into constant pool");
		*(int *)normal_static_address(fb) = 
		        (unsigned short)(constant_pool[index].i);
		break;

	    case SIGNATURE_SHORT:
		if (type_table[index] != 
		    (CONSTANT_Integer | CONSTANT_POOL_ENTRY_RESOLVED))
		    ERROR("Bad index into constant pool");
		*(int *)normal_static_address(fb) = 
		        (signed short)(constant_pool[index].i);
		break;
	    case SIGNATURE_BOOLEAN:
		if (type_table[index] != 
		    (CONSTANT_Integer | CONSTANT_POOL_ENTRY_RESOLVED))
		    ERROR("Bad index into constant pool");
		*(int *)normal_static_address(fb) = 
		       (constant_pool[index].i != 0);
		break;

	    case SIGNATURE_INT:  
		if (type_table[index] != 
		    (CONSTANT_Integer | CONSTANT_POOL_ENTRY_RESOLVED))
		    ERROR("Bad index into constant pool");
		*(int32_t *)normal_static_address(fb) = constant_pool[index].i;
		break;
		
	    case SIGNATURE_FLOAT:
		if (type_table[index] != 
		        (CONSTANT_Float | CONSTANT_POOL_ENTRY_RESOLVED))
		    ERROR("Bad index into constant pool");
		*(float *)normal_static_address(fb) = constant_pool[index].f;
		break;
			
	    default: 
		ERROR("Unable to set initial value");
	}
    } else { 
	switch (isig) { 
	    default:
		*(int *)normal_static_address(fb) = 0;
		break;

	    case SIGNATURE_FLOAT:
		*(float *)normal_static_address(fb) = (float)0.0;
		break;

	    case SIGNATURE_LONG: 
		twoword_static_address(fb) = CICalloc(context, (size_t)(2 * sizeof(int32_t)));
		if (twoword_static_address(fb) == NULL)
			return FALSE;
		SET_INT64(t1, twoword_static_address(fb), ll_zero_const);
		break;

	    case SIGNATURE_DOUBLE: 
		twoword_static_address(fb) = CICalloc(context, (size_t)(2 * sizeof(int32_t)));
		if (twoword_static_address(fb) == NULL)
			return FALSE;
		SET_DOUBLE(t1, twoword_static_address(fb), 0.0);
		break;

	    case SIGNATURE_CLASS:
		*normal_static_address(fb) = 0;
		break;
	}
    }
    return TRUE;
}




static bool_t LoadConstantPool(CICcontext *context) 
{
    ClassClass *cb = context->cb;
    unsigned short nconstants = get2bytesU16(context);
    cp_item_type *constant_pool;
    unsigned char *type_table;
    uint32_t sizeOfAllocation;
    unsigned short i;
#if defined(HAVE_ALIGNED_DOUBLES)
    Java8 t1;
#endif
    
    if (nconstants < CONSTANT_POOL_UNUSED_INDEX) {
	ERROR("Illegal constant pool size");
    }
    
    sizeOfAllocation = nconstants * sizeof(cp_item_type);
    constant_pool = CICalloc(context, sizeOfAllocation);
    if (constant_pool == NULL)
    	return FALSE;
    else
        memset(constant_pool, 0, sizeOfAllocation);
    	
    sizeOfAllocation = nconstants * sizeof(char);
    type_table = CICalloc(context, sizeOfAllocation);
    if (type_table == NULL)
    	return FALSE;
     else
        memset(type_table, 0, sizeOfAllocation);
   	
    cbConstantPool(cb) = constant_pool;
    cbConstantPoolCount(cb) = nconstants;

    constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p = type_table;
    for (i = CONSTANT_POOL_UNUSED_INDEX; i < nconstants; i++) {
	int type = get1byteI8(context);
	CONSTANT_POOL_TYPE_TABLE_PUT(type_table, i, type);
	switch (type) {
	  case CONSTANT_Utf8: {
	      unsigned short length = get2bytesU16(context);
	      char *result = CICalloc(context, (size_t)length + 1);
	      getNbytes(context, length, result);
	      result[length] = '\0';
	      constant_pool[i].p = result;
	      CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED(type_table, i);
	      break;
	  }
	
	  case CONSTANT_Class:
	  case CONSTANT_String:
	    constant_pool[i].i = get2bytes(context);
	    break;

	  case CONSTANT_Fieldref:
	  case CONSTANT_Methodref:
	  case CONSTANT_InterfaceMethodref:
	  case CONSTANT_NameAndType:
	    constant_pool[i].i = get4bytes(context);
	    break;
	    
	  case CONSTANT_Integer:
	  case CONSTANT_Float:
	    constant_pool[i].i = get4bytes(context);
	    CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED(type_table, i);
	    break;

	  case CONSTANT_Long:
	  case CONSTANT_Double: {
	      uint32_t high, low;
	      int64_t value;
	      if (i + 1 >= nconstants) ERROR("Illegal constant pool index");
	      high = get4bytes(context);
	      low = get4bytes(context);
	      value = ll_add(ll_shl(uint2ll(high), 32), uint2ll(low));
	      SET_INT64(t1, &constant_pool[i], value);
	      CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED(type_table, i);
	      i++;		/* increment i for loop, too */
	      /* Indicate that the next object in the constant pool cannot
	       * be accessed independently.    */
	      CONSTANT_POOL_TYPE_TABLE_PUT(type_table, i, 0);
	      CONSTANT_POOL_TYPE_TABLE_SET_RESOLVED(type_table, i);
	      break;
	  }

	  default:
	      ERROR("Illegal constant pool type");
	}
    }
   	return TRUE;

}



static bool_t ReadInCode(CICcontext *context, struct methodblock *mb)
{
    uint32_t attribute_length = get4bytes(context);
    unsigned char *end_ptr;
    unsigned short attribute_count;
    uint32_t code_length;
    uint32_t i;

    if (attribute_length > (unsigned long)(context->end_ptr - context->ptr))
        ERROR("Truncated class file");
    end_ptr = context->ptr + attribute_length;
    if (cbMinorVersion(context->cb) <= 2) { 
	mb->maxstack = (unsigned short) get1byte(context);
	mb->nlocals = (unsigned short) get1byte(context);
	code_length = mb->code_length = get2bytes(context);
    } else { 
	mb->maxstack = get2bytesU16(context);
	mb->nlocals = get2bytesU16(context);
	code_length = mb->code_length = get4bytes(context);
    }

    mb->code = CICalloc(context, code_length);
    if (mb->code == NULL)
        return FALSE;

    getNbytes(context, code_length, (char *)mb->code);
    if ((mb->exception_table_length = get2bytes(context)) > 0) {
	uint32_t exception_table_size = mb->exception_table_length 
	                                  * sizeof(struct CatchFrame);

	mb->exception_table = CICalloc(context, (size_t)exception_table_size);
	if (mb->exception_table == NULL)
		return FALSE;

	for (i = 0; i < mb->exception_table_length; i++) {
	    mb->exception_table[i].start_pc = get2bytes(context);
	    mb->exception_table[i].end_pc = get2bytes(context);		
	    mb->exception_table[i].handler_pc = get2bytes(context); 
	    mb->exception_table[i].catchType = get2bytesI16(context);
	}
    }
    attribute_count = get2bytesU16(context);
    for (i = 0; i < attribute_count; i++) {
	char *name = getAsciz(context);
	if (strcmp(name, "LineNumberTable") == 0) {
	    if (!ReadLineTable(context, mb))
	    	return FALSE;
#ifdef JCOV
        } else if (!strcmp(name, "CoverageTable") && cov_file) {
            ReadCoverageTable(context, mb);
#endif
	} else if (strcmp(name, "LocalVariableTable") == 0) {
	    if (!ReadLocalVars(context, mb))
	    	return FALSE;
	} else {
	    int32_t length = get4bytes(context);
	    getNbytes(context, length, NULL);
	}
    }
    if (context->ptr != end_ptr) 
	ERROR("Code segment was wrong length");
	return TRUE;
}


#ifdef JCOV
/* Primary 1000F9C9: counters/index survive reclamation of clinit bytecode. */
static void ReadCoverageTable(CICcontext *context, struct methodblock *mb)
{
    unsigned long length = get4bytes(context), i;
    unsigned char *end;
    unsigned long *index;
    int was_clinit = context->in_clinit;
    if (length > (unsigned long)(context->end_ptr - context->ptr))
        ERROR("Truncated class file");
    end = context->ptr + length;
    context->in_clinit = 0;
    mb->coverage_table_length = get2bytes(context);
    if (mb->coverage_table_length) {
        mb->coverage_table = CICalloc(context,
            mb->coverage_table_length * sizeof(struct covtable) +
            mb->code_length * sizeof(unsigned long));
        index = (unsigned long *)(mb->coverage_table + mb->coverage_table_length);
        for (i = 0; i < mb->coverage_table_length; ++i) {
            struct covtable *entry = mb->coverage_table + i;
            entry->pc = get2bytes(context);
            /* deviation: reject malformed PCs before writing outside the index. */
            if (entry->pc >= mb->code_length) ERROR("Bad coverage PC");
            index[entry->pc] = i + 1;
            entry->type = get2bytes(context);
            entry->where = get4bytes(context);
            entry->count = 0;
        }
    }
    if (context->ptr != end) mb->coverage_table = NULL;
    context->in_clinit = was_clinit;
}
#endif

static bool_t ReadLineTable(CICcontext *context, struct methodblock *mb)
{
    int32_t attribute_length = get4bytes(context);
    unsigned char *end_ptr;
    int32_t i;
    if (attribute_length > (unsigned long)(context->end_ptr - context->ptr))
        ERROR("Truncated class file");
    end_ptr = context->ptr + attribute_length;
    if ((mb->line_number_table_length = get2bytes(context)) > 0) {
	struct lineno *ln = NULL;
	ln = CICalloc(context, (size_t)(mb->line_number_table_length * sizeof(struct lineno)));
	if (ln == NULL)
		return FALSE;
	mb->line_number_table = ln;
	for (i = (int32_t) mb->line_number_table_length; --i >= 0; ln++) {
	    ln->pc = get2bytes(context);
	    ln->line_number = get2bytes(context);
	}
    }
    if (context->ptr != end_ptr)
	ERROR("Line number table was wrong length?");
	return TRUE;
}	

static bool_t ReadLocalVars(CICcontext *context, struct methodblock *mb)
{
    int32_t attribute_length = get4bytes(context);
    unsigned char *end_ptr;
    int32_t i;
    if (attribute_length > (unsigned long)(context->end_ptr - context->ptr))
        ERROR("Truncated class file");
    end_ptr = context->ptr + attribute_length;
    if ((mb->localvar_table_length = get2bytes(context)) > 0) {
	struct localvar *lv = NULL;
	lv = CICalloc(context, (size_t)(mb->localvar_table_length * sizeof(struct localvar)));
	if (lv == NULL)
		return FALSE;
	mb->localvar_table = lv;
	for (i = (int32_t) mb->localvar_table_length; --i >= 0; lv++) {
	    lv->pc0 = get2bytes(context);
	    lv->length = get2bytes(context);
	    lv->nameoff = get2bytesI16(context);
	    lv->sigoff = get2bytesI16(context);
	    lv->slot = get2bytes(context);
	}
    }
    if (context->ptr != end_ptr)
	ERROR("Local variables table was wrong length?");
	return TRUE;
}	

unsigned Signature2ArgsSize(char *method_signature)
{
    char *p;
    int args_size = 0;
    for (p = method_signature; *p != SIGNATURE_ENDFUNC; p++) {
	switch (*p) {
	  case SIGNATURE_BOOLEAN:
	  case SIGNATURE_BYTE:
	  case SIGNATURE_CHAR:
	  case SIGNATURE_SHORT:
	  case SIGNATURE_INT:
	  case SIGNATURE_FLOAT:
	    args_size += 1;
	    break;
	  case SIGNATURE_CLASS:
	    args_size += 1;
	    while (*p != SIGNATURE_ENDCLASS) p++;
	    break;
	  case SIGNATURE_ARRAY:
	    args_size += 1;
	    while ((*p == SIGNATURE_ARRAY)) p++;
	    /* If an array of classes, skip over class name, too. */
	    if (*p == SIGNATURE_CLASS) { 
		while (*p != SIGNATURE_ENDCLASS) 
		  p++;
	    } 
	    break;
	  case SIGNATURE_DOUBLE:
	  case SIGNATURE_LONG:
	    args_size += 2;
	    break;
	  case SIGNATURE_FUNC:	/* ignore initial (, if given */
	    break;
	  default:
	    /* Indicate an error. */
	    return 0;
	}
    }
    return args_size;
}


/* Primary 1000EBE7: four CP words followed by four type bytes. */
ClassClass *createFakeArrayClass(char *name, int base_type, int depth,
                               ClassClass *inner_cb,
                               struct Hjava_lang_ClassLoader *loader)
{
    ClassClass *cb = allocClassClass();
    cp_item_type *pool;
    unsigned char *types;
    char *copy;
    if (!cb) return NULL;
    pool = sysCalloc(CONSTANT_POOL_ARRAY_LENGTH, sizeof(*pool) + 1);
    if (!pool) return NULL;
    copy = sysMalloc(strlen(name) + 1);
    if (!copy) { sysFree(pool); return NULL; }
    strcpy(copy, name);
    sysAssert(name[0] == SIGNATURE_ARRAY);
    cbMajorVersion(cb) = JAVA_VERSION;
    cbMinorVersion(cb) = JAVA_MINOR_VERSION;
    cbName(cb) = copy;
    cbSuperName(cb) = JAVAPKG "Object";
    cbConstantPool(cb) = pool;
    cbConstantPoolCount(cb) = CONSTANT_POOL_ARRAY_LENGTH;
    cbLoader(cb) = loader;
    types = (unsigned char *)(pool + CONSTANT_POOL_ARRAY_LENGTH);
    pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p = types;
    pool[CONSTANT_POOL_ARRAY_DEPTH_INDEX].i = depth;
    pool[CONSTANT_POOL_ARRAY_TYPE_INDEX].i = base_type;
    types[CONSTANT_POOL_ARRAY_DEPTH_INDEX] =
        types[CONSTANT_POOL_ARRAY_TYPE_INDEX] = CONSTANT_Integer | CONSTANT_POOL_ENTRY_RESOLVED;
    if (base_type == T_CLASS) {
        pool[CONSTANT_POOL_ARRAY_CLASS_INDEX].p = inner_cb;
        types[CONSTANT_POOL_ARRAY_CLASS_INDEX] = CONSTANT_Class | CONSTANT_POOL_ENTRY_RESOLVED;
        cbAccess(cb) = (cbAccess(inner_cb) & ACC_PUBLIC) | ACC_FINAL | ACC_ABSTRACT;
    } else {
        types[CONSTANT_POOL_ARRAY_CLASS_INDEX] = CONSTANT_Integer | CONSTANT_POOL_ENTRY_RESOLVED;
        cbAccess(cb) = ACC_PUBLIC | ACC_FINAL | ACC_ABSTRACT;
    }
    AddBinClass(cb);
    return cb;
}

/* Primary 1000FE54: Code and Exceptions may appear in either order. */
void freeClassInitializer(struct methodblock *mb)
{
    unsigned char *base = mb->code;
    if (mb->exceptions && (unsigned char *)mb->exceptions <= base)
        base = (unsigned char *)mb->exceptions;
    sysFree(base);
    mb->code = NULL;
    mb->exceptions = NULL;
}

/* Free only native metadata. The collector owns the class body and handle. */
void FreeClass(ClassClass *cb)
{
    struct methodblock *mb;
    unsigned int i;
    CompilerFreeClass(cb);
    for (i = 0, mb = cbMethods(cb); i < cbMethodsCount(cb); ++i, ++mb) {
        if (!strcmp(mb->fb.name, "<clinit>") &&
            !strcmp(mb->fb.signature, "()V") && mb->code_length) {
            freeClassInitializer(mb);
        }
    }
    sysFree(cbConstantPool(cb));
    sysFree(cbMethodTableMem(cb));
    sysFree(cbSlotTable(cb));
    if (cbImplementsCount(cb) || cbIsInterface(cb))
        sysFree(cbIntfMethodTable(cb));
    /* deviation: legacy source-retry callers can leave the managed body
     * reachable after freeing metadata; keep subsequent GC scans safe. */
    cbConstantPool(cb) = NULL;
    cbFields(cb) = NULL;
    cbMethods(cb) = NULL;
    cbImplements(cb) = NULL;
    cbName(cb) = cbSuperName(cb) = cbSourceName(cb) = NULL;
    cbConstantPoolCount(cb) = cbFieldsCount(cb) = cbMethodsCount(cb) = 0;
    cbImplementsCount(cb) = 0;
    cbMethodTableMem(cb) = NULL;
    cbSlotTable(cb) = NULL;
    cbIntfMethodTable(cb) = NULL;
}

static unsigned long get1byte(CICcontext *context)
{
    unsigned char *ptr = context->ptr;
    if (context->end_ptr - ptr < 1) {
	ERROR("Truncated class file");
    } else {
	unsigned char *ptr = context->ptr;
	unsigned long value = ptr[0];
	(context->ptr) += 1;
	return value;
    }
}

#define U32(x)\
	((uint32_t)x)

static unsigned long get2bytes(CICcontext *context)
{
    unsigned char *ptr = context->ptr;
    if (context->end_ptr - ptr < 2) {
	ERROR("Truncated class file");
    } else {
	unsigned long value = (U32(ptr[0]) << 8) + ptr[1];
	(context->ptr) += 2;
	return value;
    }
}


static unsigned long get4bytes(CICcontext *context)
{
    unsigned char *ptr = context->ptr;
    if (context->end_ptr - ptr < 4) {
	ERROR("Truncated class file");
    } else {
	unsigned long value = 
	    ( U32(ptr[0]) << 24)
	  + ( U32(ptr[1]) << 16)
	  + ( U32(ptr[2]) << 8) 
	  + ptr[3];
#ifdef OSF1
	value &= 0xffffffff;
#endif
	(context->ptr) += 4;
	return value;
    }
}

static void getNbytes(CICcontext *context, int32_t count, char *buffer) 
{
    unsigned char *ptr = context->ptr;
    if (count < 0 || context->end_ptr - ptr < count)
	ERROR("Truncated class file");
    if (buffer != NULL) 
	memcpy(buffer, ptr, (size_t) count);
    (context->ptr) += count;
}


static char *getAsciz(CICcontext *context) 
{
    ClassClass *cb = context->cb;
    union cp_item_type *constant_pool = cbConstantPool(cb);
    unsigned int nconstants = cbConstantPoolCount(cb);
    unsigned char *type_table = constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p;

    unsigned int value = get2bytesU16(context);
    if ((value == 0) || (value >= nconstants) || 
	   type_table[value] != (CONSTANT_Utf8 | CONSTANT_POOL_ENTRY_RESOLVED))
	ERROR("Illegal constant pool index");
    return constant_pool[value].cp;
}

static char *getAscizFromClass(CICcontext *context, unsigned int value) 
{
    ClassClass *cb = context->cb;
    union cp_item_type *constant_pool = cbConstantPool(cb);
    unsigned int nconstants = cbConstantPoolCount(cb);
    unsigned char *type_table = constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p;
    if ((value <= 0) || (value >= nconstants) 
	|| (type_table[value] != CONSTANT_Class))
	ERROR("Illegal constant pool index");
    value = constant_pool[value].i;
    if ((value <= 0) || (value >= nconstants) ||
	 (type_table[value] != (CONSTANT_Utf8 | CONSTANT_POOL_ENTRY_RESOLVED)))
	ERROR("Illegal constant pool index");
    return constant_pool[value].cp;
}

static void ValidateClassName(CICcontext *context, char *name) 
{
    if (!name || !name[0] || '[' == name[0]) { 
        /* printf("Validation error on %s\n", name ? name:"No Name"); */
        ERROR("Invalid class name in byte code"); 
    }
}


/* In certain cases, we only need a class's name.  We don't want to resolve
 * the class reference if it isn't, already. 
 */

char *GetClassConstantClassName(cp_item_type *constant_pool, int index)
{
    unsigned char *type_table = constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p;
    switch(type_table[index]) {
        case CONSTANT_Class | CONSTANT_POOL_ENTRY_RESOLVED: {
	    ClassClass *cb = constant_pool[index].p;
	    return cbName(cb);
	}

        case CONSTANT_Class: {
	    unsigned int name_index = constant_pool[index].i;
	    return constant_pool[name_index].cp;
	}
	    
	default:
	    return (char *)0;
    }
}

