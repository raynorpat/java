/*
 * Native method linking on Unix: libraries loaded with System.load or
 * System.loadLibrary, plus the program and libjavai (which holds the
 * built-in native stubs), are searched in load order for each stub symbol.
 */
#include <dlfcn.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "sys_api.h"
#include "path_md.h"

#define MAX_LIBS 256

#ifdef __APPLE__
#define LIB_SUFFIX ".dylib"
#else
#define LIB_SUFFIX ".so"
#endif

static pthread_mutex_t libs_lock = PTHREAD_MUTEX_INITIALIZER;
static void *libs[MAX_LIBS];
static int nlibs;

static void add_lib(void *h)
{
    int i;

    pthread_mutex_lock(&libs_lock);
    for (i = 0; i < nlibs && libs[i] != h; i++)
	;
    if (i == nlibs && nlibs < MAX_LIBS)
	libs[nlibs++] = h;
    pthread_mutex_unlock(&libs_lock);
}

/*
 * Save the ld path
 */
void sysSaveLDPath(char *ldp)
{
    putenv(ldp);
}

/*
 * <path>/lib<name>.so
 */
void sysBuildLibName(char *holder, int holderlen, char *pname, char *fname)
{
    if (holderlen > 0) {
	if (strlen(pname) + strlen(fname) + sizeof("/lib" LIB_SUFFIX) > (size_t) holderlen) {
	    holder[0] = 0;
	    return;
	}
	sprintf(holder, "%s/lib%s" LIB_SUFFIX, pname, fname);
    }
}

/*
 * Initialize the linker. Returns the library search path for
 * System.loadLibrary: <java home>/lib, then LD_LIBRARY_PATH.
 */
char *sysInitializeLinker()
{
    void *self;
    const char *path = getenv("LD_LIBRARY_PATH");
    const char *home = sysJavaHome();
    char *buf;

    /* The program and the libraries it was linked with, libjavai included. */
    if ((self = dlopen(NULL, RTLD_LAZY)) != NULL)
	add_lib(self);

    if (path == NULL)
	path = "";
    if ((buf = malloc(strlen(home) + strlen(path) + 6)) != NULL)
	sprintf(buf, "%s/lib:%s", home, path);
    return buf;
}

/*
 * Add a shared library to the list
 */
int sysAddDLSegment(char *fn)
{
    void *h = dlopen(fn, RTLD_LAZY | RTLD_GLOBAL);

    if (h == NULL)
	return 0;
    add_lib(h);
    return 1;
}

long sysDynamicLink(char *sym_p)
{
    void *f = NULL;
    int i;

    if (nlibs == 0)
	free(sysInitializeLinker());	/* registers the program */

    pthread_mutex_lock(&libs_lock);
    for (i = 0; i < nlibs && f == NULL; i++) {
        f = dlsym(libs[i], sym_p);
    }
    pthread_mutex_unlock(&libs_lock);
    return (long) f;
}
