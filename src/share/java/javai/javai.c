/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/*
 * @(#)javai.c	1.75 95/08/11
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
 *      interpret an java object file
 */


#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include "threads.h"
#include "errno.h"
#include "tree.h"
#include "oobj.h"
#include "interpreter.h"
#include "exceptions.h"
#include "java_lang_Throwable.h"
#include "javaString.h"
#include "log.h"
#include "installpath.h"
#include "bool.h"
#include "jio.h"
#include "jni.h"
#include "classhash115.h"
#ifdef JCOV
#include "jcov.h"
#endif

#ifndef NATIVE
#include "iomgr.h"
#include <fcntl.h>
#ifndef XP_PC
#include <sys/wait.h>
#include <unistd.h>
#include <sys/signal.h>
#endif /* ! XP_PC */
#include <signal.h>
#else /* NATIVE  */
#include "monitor.h"
#endif /* NATIVE */

/* Platform (md) start-up hooks */
extern void intrInit(void);
extern void InitializeMem(void);
extern void InitializeAsyncIO(void);

#ifdef MONITOR
#include <mon.h>
#endif

#ifndef NATIVE
#include <java_lang_Thread.h>
#endif /* NATIVE */


extern int verbose_class_loading;

/* Force load in the language level natives */
extern void _java_lang_init(void);

extern Hjava_lang_Thread *InitializeClassThread(ExecEnv *ee, char **);

#ifndef NATIVE
extern void InitializeSbrk(void);
#endif /* NATIVE */

extern void WaitToDie(void);

static void usage(void);

#ifdef MONITOR
static void MonitorTeardown(void) {
    monitor( 0, 0, 0, 0, 0 );
}

static void MonitorSetup(void) {
    extern int etext(void);
    extern int _start(void);
    int (* low)() = &_start;
    int (* high)() = & etext;
    WORD * stuff;
    int textwords = ((int)high - (int)low) / 4;
    int nbytes = sizeof(struct hdr) + 10*sizeof( struct cnt )
				    + textwords*sizeof(WORD);
    stuff = (WORD*)malloc( nbytes );
    monitor( low, high, stuff, nbytes/sizeof(WORD), 10 );
    sysAtexit( MonitorTeardown );
}
#endif

/* from properties_md.c */
extern char **user_props;
static int nprops = 0;
static int max_props = 0;

/* add a user property */
void add_user_prop(char *def) 
{
    char *p, *str;

    /* scan to the '=' */
    for (p = def ; *p && *p != '=' ; p++);

    /* grow the array (if needed) */
    if (nprops + 3 > max_props) {
	if (user_props == 0) {
	    user_props = (char **)malloc(16 * sizeof(char *));
	    max_props = 16;
	} else {
	    char **new_props = (char **)malloc(2 * max_props * sizeof(char *));
	    memcpy(new_props, user_props, nprops * sizeof(char *));
	    free(user_props);
	    user_props = new_props;
	    max_props *= 2;
	}
    }

    /* malloc the key */
    str = (char *)malloc((p - def) + 1);
    strncpy(str, def, p - def);
    str[p - def] = '\0';
    user_props[nprops++] = str;

    /* malloc the value */
    if (*p == '=') {
	p++;
    }
    str = (char *)malloc(strlen(p) + 1);
    strcpy(str, p);
    user_props[nprops++] = str;
    user_props[nprops] = NULL;
}

/*
 * have to put the hidden copy of the command line argument list where GC
 * will find it
 */

HArrayOfString *
commandLineArguments(int argc, char **argv, struct execenv *ee)
{
    HArrayOfString *ret = (HArrayOfString *) ArrayAlloc(T_CLASS, argc);
    if (ret == 0) {
OutOfMemory:
	SignalError(ee, JAVAPKG "OutOfMemoryError", 0);
	return 0;
    }
    /* Give the array a "type" */
    unhand((HArrayOfObject *)ret)->body[argc] = 
                        (JHandle *)FindClass(ee, JAVAPKG "String", TRUE);
    while (--argc >= 0) {
	char *s = argv[argc];
	if ((unhand(ret)->body[argc] = makeJavaString(s, strlen(s))) == 0)
	    goto OutOfMemory;
    }
    return (HArrayOfString *) ret;
}

long
atomi(register char *p)
{
    long v = 0;
    long mult = 1;
    long c;
    while ((c = *p++))
	switch (c) {
	case 'M':
	case 'm':
	    mult = 1024 * 1024;
	    break;
	case 'K':
	case 'k':
	    mult = 1024;
	    break;
	default:
	    if (c < '0' || c > '9')
		return -1;
	    v = v * 10 + c - '0';
	    break;
	}
    return v * mult;
}

extern void InitializeAsyncMonitors(void);

#include <path_md.h>

/*
 * Default values for minimum and maximum amounts of memory to devote
 * to the Java heap.  The current abstraction, which may not hold up
 * forever, is that the initial size of the heap is the minimum heap.
 *
 * The maximum memory to map: As we're mmapping the memory, it looks
 * like we should ask swapctl(2) how much swap we have, and thus what
 * the maximum we've got is.
 *
 * These values can be overridden on the java command line.
 */
#ifdef USE_MALLOC
#define MINHEAPSIZE ((int) ( 3 * (1024*1024)))
#define MAXHEAPSIZE MINHEAPSIZE
#else
#define MINHEAPSIZE ((int) ( 1 * (1024*1024)))
#define MAXHEAPSIZE ((int) (16 * (1024*1024)))
#endif /* USE_MALLOC */

/*
 * The default thread stack size.  This is machine-dependent, because it
 * is based on a platform's C stack usage.
 */
#define PROCSTACKSIZE (128 * 1024)     /* Default size of a thread C stack */

int awt_init_xt = 1;

#if defined(STATIC_JAVA)
extern void _java_awt_init(void);
extern void _java_mmedia_init(void);
#endif /* STATIC_JAVA */

#if defined(XP_PC) && !defined(_WIN32)
/*
** On 16-bit windows, the Floating Point subsystem (WINEM87.DLL) 
** must be explicitly initialized... 
** Unfortunately, the necessary functions do not appear in any
** header files...
*/
void _acrtused2() {};
extern LPVOID   pascal  _FPInit();
extern void     pascal  _FPTerm(LPVOID);
#endif

#define INITBUFSIZE 1024
#define INITNARGS 32

/* Shared launcher/invocation startup; Java main execution remains separate. */
int java_initialize_runtime(ExecEnv *ee, int MinHeapSize, int MaxHeapSize)
{
    Hjava_lang_Thread *self;
    char *errmsg;
    InitializeExecEnv(ee, 0);  /* Call before InitializeMem() */
    InitializeMem();		/* Could throw exception on open() */
#ifdef USE_MALLOC
    MinHeapSize = MaxHeapSize;
#endif /* USE_MALLOC */
    if (!InitializeAlloc(MaxHeapSize, MinHeapSize)) return 1;
    if (!initializeClassHash115()) return 1;
    InitializeInterpreter();	/* Call before InitializeClassThread() */

    self = InitializeClassThread(ee, &errmsg);
    if (self == 0) {
	fprintf(stderr, "Unable to initialize threads: %s\n", errmsg);
	sysExit(1);
    }
    setThreadName(self,MakeString("main",4));

    /* Reconstructed 1.1.5 boot step (javai_g.dll 0x10002841).
     * deviation: keep the old class-library baseline runnable during migration;
     * its System class has no initializeSystemClass method. */
    {
	ClassClass *system = FindClass(ee, JAVAPKG "System", TRUE);
	int index;
	if (system == NULL)
	    return 1;
	for (index = 0; index < cbMethodsCount(system); index++) {
	    struct methodblock *method = &cbMethods(system)[index];
	    if (strcmp(fieldname(&method->fb), "initializeSystemClass") == 0 &&
		strcmp(fieldsig(&method->fb), "()V") == 0) {
		/* The old name-based helper enforces Java access checks; the VM
		 * invokes this private boot method directly, as for <clinit>. */
		do_execute_java_method(ee, system, 0, 0, method, TRUE);
		if (exceptionOccurred(ee)) {
		    /* deviation: report boot failure even before System.err exists. */
		    if (ee->exception.exc != NULL) {
			char detail[256];
			Hjava_lang_Throwable *error = (Hjava_lang_Throwable *)ee->exception.exc;
			detail[0] = 0;
			if (unhand(error)->detailMessage != NULL)
			    javaString2CString(unhand(error)->detailMessage, detail, sizeof(detail));
			fprintf(stderr, "System initialization failed: %s\n",
				cbName(obj_classblock(ee->exception.exc)));
			fprintf(stderr, "%s\n", detail);
		    }
		    exceptionDescribe(ee);
		    return 1;
		}
		break;
	    }
	}
    }

    return 0;
}

int java_initialize_platform(void)
{
#if defined(XP_PC) /* jag, hack hack :) */
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(1,1);

    if( WSAStartup( wVersionRequested, &wsaData ) ) {
        fprintf(stderr, "winsock failed to initialize\n");
        return 1;
    }
#endif

    set_java_io_mode(JIO_UNRESTRICTED);
    set_java_fs_mode(JFS_UNRESTRICTED);
    set_java_rt_mode(JRT_UNRESTRICTED);
    intrInit();

    /* Initialize the monitor registry */
    monitorRegistryInit();
    /* Create the monitor cache */
    monitorCacheInit();

#ifndef NATIVE
    /* Initialize the special case for sbrk on Solaris (see synch.c) */
    InitializeSbrk();
    /* Initialize the async io */
    InitializeAsyncIO();
#endif 

    return 0;
}

/* javai_g.dll 100012d5: the public launcher uses the invocation API. */
JAVA_PUBLIC_API(int)
java_main(int argc, char **argv)
{
    JDK1_1InitArgs init;
    JavaVM *vm;
    JNIEnv *env;
    ExecEnv *ee;
    ClassClass *cb, *owner;
    struct methodblock *main_method = NULL;
    HArrayOfString *arguments;
    const char *main_error = "void main(String argv[]) is not defined";
    char *class_name, *p, *default_path;
    const char *user_path = getenv("CLASSPATH");
    int i, result = 0, profiling = 0, extra_monitors = 0;
    extern int monCount;
    extern void expandMonCache(int);
    long size;
    extern int tracegc;
    extern char *prof_file;
    extern long barriersize_red, barriersize_yellow;

    JNI_GetDefaultJavaVMInitArgs(&init);
    if (!user_path) user_path = ".";
    default_path = malloc(strlen(user_path) + strlen(init.classpath) + 2);
    if (!default_path) return 1;
    sprintf(default_path, "%s%c%s", user_path, PATH_SEPARATOR, init.classpath);
    init.classpath = default_path;
    progname = strrchr(argv[0], LOCAL_DIR_SEPARATOR);
    progname = progname ? progname + 1 : argv[0];
    while (--argc > 0 && (*++argv)[0] == '-') {
        p = *argv;
#ifdef TRACING
        if (!strcmp(p, "-t")) trace++;
        else if (!strcmp(p, "-tm")) tracem++;
        else
#endif
        if (!strcmp(p, "-v") || !strcmp(p, "-verbose")) init.verbose = 1;
        else if (!strcmp(p, "-debug")) init.debugging = 1;
        else if (!strcmp(p, "-noagent")) init.debugPort = -1;
        else if (!strcmp(p, "-debugport") || !strcmp(p, "-classpath")) {
            if (argc <= 1) {
                fprintf(stderr, "%s\n", !strcmp(p, "-classpath") ?
                        "-classpath requires class path specification" :
                        "-debugport requires port number");
                usage(); return 1;
            }
            if (!strcmp(p, "-classpath")) init.classpath = argv[1];
            else init.debugPort = atol(argv[1]);
            argc--; argv++;
        } else if (!strcmp(p, "-noasyncgc")) init.disableAsyncGC = 1;
        else if (!strcmp(p, "-verbosegc")) init.enableVerboseGC = 1;
        else if (!strcmp(p, "-noclassgc")) init.enableClassGC = 0;
        else if (!strcmp(p, "-verify")) init.verifyMode = VERIFY_ALL;
        else if (!strcmp(p, "-verifyremote")) init.verifyMode = VERIFY_REMOTE;
        else if (!strcmp(p, "-noverify")) init.verifyMode = VERIFY_NONE;
        else if (!strcmp(p, "-tracegc")) tracegc++;
        else if (!strcmp(p, "-prof")) { profiling = 1; init.enableClassGC = 0; }
        else if (!strncmp(p, "-prof:", 6) && p[6]) {
            prof_file = p + 6; profiling = 1; init.enableClassGC = 0;
        }
        else if (!strcmp(p, "-prof=jcov") ||
                 (!strncmp(p, "-prof=jcov:", 11) && p[11])) {
#ifdef JCOV
            cov_file = p[10] == ':' ? p + 11 : "java.jcov";
            init.enableClassGC = 0;
            sysAtexit(java_cov_dump);
#endif
        }
        else if (!strncmp(p, "-ss", 3)) {
            if ((size = atomi(p + 3)) >= 1000) init.nativeStackSize = size;
        } else if (!strncmp(p, "-oss", 4)) {
            if ((size = atomi(p + 4)) >= 1000) init.javaStackSize = size;
        } else if (!strncmp(p, "-ms", 3)) {
            if ((size = atomi(p + 3)) >= 1000) init.minHeapSize = size;
        } else if (!strncmp(p, "-mx", 3)) {
            if ((size = atomi(p + 3)) >= 1000) init.maxHeapSize = size;
        } else if (!strncmp(p, "-nm", 3)) {
            if ((size = atomi(p + 3)) >= 0)
                extra_monitors = size > monCount ? size - monCount : 0;
        } else if (!strncmp(p, "-mr", 3)) {
            if ((size = atomi(p + 3)) > 0) barriersize_red = size;
        } else if (!strncmp(p, "-my", 3)) {
            if ((size = atomi(p + 3)) > 0) barriersize_yellow = size;
        } else if (!strncmp(p, "-D", 2)) add_user_prop(p + 2);
#ifdef LOGGING
        else if (!strncmp(p, "-l", 2)) logging_level = atomi(p + 2);
#endif
        else if (!strcmp(p, "-help") || !strcmp(p, "-h") || !strcmp(p, "-?")) {
            usage(); return 0;
        } else if (!strcmp(p, "-version") || !strcmp(p, "-fullversion")) {
            fprintf(stderr, "%s %sversion \"%s\"\n", progname,
                    !strcmp(p, "-version") ? "" : "full ",
                    !strcmp(p, "-version") ? "1.1.5" : "JDK1.1.5K");
            return 0;
        } else {
            fprintf(stderr, "%s: illegal argument\n", p);
            usage(); return 1;
        }
    }
    if (argc <= 0) { usage(); return 1; }
    class_name = *argv++;
    argc--;
    if (strchr(class_name, '\\')) {
        fprintf(stderr, "Invalid class name '%s'\n", class_name);
        usage(); return 1;
    }
    for (p = class_name; *p; p++) if (*p == '.') *p = '/';
    if (JNI_CreateJavaVM(&vm, &env, &init)) { sysExit(1); return 1; }
    ee = EE();
    /* deviation: keep the primordial root bound in this live launcher frame. */
    mainstktop = (stackp_t)&argc;
    if (profiling) {
        extern void javamon(int);
        sysAtexit(java_mon_dump);
        javamon(1);
    }
    cb = FindClass(ee, class_name, TRUE);
    if (!cb) {
        fprintf(stderr, "Can't find class %s\n", class_name);
        sysExit(1); return 1;
    }
    cbFlags(cb) |= CCF_IsSticky;
    if (extra_monitors > 0) expandMonCache(extra_monitors);
    FindClass(ee, JAVAPKG "Compiler", TRUE);
    for (owner = cb; owner && !main_method;
         owner = cbSuperclass(owner) ? cbSuperclass(owner) : NULL) {
        for (i = 0; i < cbMethodsCount(owner); i++) {
            struct methodblock *method = &cbMethods(owner)[i];
            if (strcmp(fieldname(&method->fb), "main") ||
                strncmp(fieldsig(&method->fb), "([Ljava/lang/String;)", 21)) continue;
            if (fieldsig(&method->fb)[21] != SIGNATURE_VOID)
                main_error = "main must return void";
            else if ((method->fb.access & (ACC_STATIC | ACC_PUBLIC)) != (ACC_STATIC | ACC_PUBLIC))
                main_error = "main must be public and static";
            else { main_method = method; main_error = NULL; }
            goto found_main;
        }
    }
found_main:
    if (main_error) {
        fprintf(stderr, "In class %s: %s\n", class_name, main_error);
        sysExit(1); return 1;
    }
    arguments = commandLineArguments(argc, argv, ee);
    if (!arguments) { sysExit(1); return 1; }
    do_execute_java_method(ee, cb, NULL, NULL, main_method, TRUE, (JHandle *)arguments);
    if (exceptionOccurred(ee) && ee->thread && THREAD(ee->thread)->group) {
        JHandle *error = ee->exception.exc;
        exceptionClear(ee);
        execute_java_dynamic_method(ee, (HObject *)THREAD(ee->thread)->group,
            "uncaughtException", "(Ljava/lang/Thread;Ljava/lang/Throwable;)V", ee->thread, error);
        result = 1;
    }
    WaitToDie();
    sysExit(result);
    return result;
}

void
Execute(char **command, char *alternate)
{
    long pid, wpid;
    long lapcnt;
    int status = -1;
    if (verbose) {
	printf("[Executing");
	for (lapcnt = 0; command[lapcnt]; lapcnt++)
	    printf(" %s", command[lapcnt]);
	printf("]\n");
    }
    lapcnt = 0;
#ifdef XP_PC	/* rjp */
    sysAssert(0);
#else
    while ((pid = fork()) < 0) {
	if (lapcnt == 0)
	    write(2, "[ Running out of system memory, waiting...", 42);
	lapcnt++;
	sleep(5);
    }
    if (pid) {
	if (lapcnt)
	    write(2, " got it ]\n", 10);
	while ((wpid = wait(&status)) == -1 || wpid != pid);
	if (status != 0) {
	    fprintf(stderr, "%s: failed (%X)\n", command[0], status);
	    sysExit(1);
	}
    } else {
	lapcnt = 0;
	while (1) {
	    execvp(command[0], command);
	    if (alternate)
		execvp(alternate, command);
	    if (errno != ENOMEM) {
		perror(command[0]);
		sysExit(1);
	    }
	    if (lapcnt == 0)
		write(2, "Waiting for system memory...\n", 29);
	    sleep(20);
	}
    }
#endif
    if (verbose)
	printf("[Finished %s]\n", command[0]);
}

static void usage()
{
    fprintf(stderr, "Usage: %s [-options] class\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "where options include:\n");
    fprintf(stderr, "    -help             print out this message\n");
    fprintf(stderr, "    -version          print out the build version\n");
#ifndef trace
    fprintf(stderr, "    -t                turn on instruction tracing\n");
    fprintf(stderr, "    -tm               turn on method tracing\n");
#endif
#ifdef MONITOR
    fprintf(stderr, "    -m                turn on monitoring\n");
#endif
    fprintf(stderr, "    -v -verbose       turn on verbose mode\n");
    fprintf(stderr, "    -debug            enable JAVA debugging\n");
    fprintf(stderr, "    -noasyncgc        don't allow asynchronous gc's\n");
    fprintf(stderr, "    -verbosegc        print a message when GCs occur\n");
    fprintf(stderr, "    -cs -checksource  check if source is newer when loading classes\n");
    fprintf(stderr, "    -ss<number>       set the C stack size of a process\n");
    fprintf(stderr, "    -oss<number>      set the JAVA stack size of a process\n");
    fprintf(stderr, "    -ms<number>       set the initial Java heap size\n");
    fprintf(stderr, "    -mx<number>       set the maximum Java heap size\n");
    fprintf(stderr, "    -D<name>=<value>  set a system property\n");
#ifdef LOGGING
    fprintf(stderr, "    -l<number>        set the logging level\n");
#endif
    fprintf(stderr, "    -classpath <directories separated by colons>\n");
    fprintf(stderr, "                      list directories in which to look for classes\n");
    fprintf(stderr, "    -prof             output profiling data to ./java.prof\n");
    fprintf(stderr, "    -verify           verify all classes when read in\n");
    fprintf(stderr, "    -verifyremote     verify classes read in over "
                                              "the network [default]\n");
    fprintf(stderr, "    -noverify         do not verify any class\n");
}

void fakeref(void)
{
    _java_lang_init();
#if defined(STATIC_JAVA)
    _java_awt_init();
    _java_mmedia_init();
#endif
}
