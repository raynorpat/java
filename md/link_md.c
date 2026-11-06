/*
 * Native method linking on Win32: libraries loaded with System.load or
 * System.loadLibrary, plus javai.dll itself (which holds the built-in
 * native stubs), are searched in load order for each stub symbol.
 */
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "sys_api.h"
#include "path_md.h"

#define MAX_LIBS 256

static SRWLOCK libs_lock = SRWLOCK_INIT;
static HMODULE libs[MAX_LIBS];
static int nlibs;

static void add_lib(HMODULE h)
{
    int i;

    AcquireSRWLockExclusive(&libs_lock);
    for (i = 0; i < nlibs && libs[i] != h; i++)
	;
    if (i == nlibs && nlibs < MAX_LIBS)
	libs[nlibs++] = h;
    ReleaseSRWLockExclusive(&libs_lock);
}

/*
 * Save the ld path
 */
void sysSaveLDPath(char *ldp)
{
    putenv(ldp);
}

/*
 * <path>\<name>.dll
 */
void sysBuildLibName(char *holder, int holderlen, char *pname, char *fname)
{
    if (holderlen > 0) {
	_snprintf(holder, holderlen, "%s\\%s.dll", pname, fname);
	holder[holderlen - 1] = 0;
    }
}

/*
 * Initialize the linker. Returns the library search path for
 * System.loadLibrary: <java home>\bin, then PATH.
 */
char *sysInitializeLinker()
{
    HMODULE self;
    const char *path = getenv("PATH");
    const char *home = sysJavaHome();
    char *buf;

    /* The VM's own stubs live in this DLL. */
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
			   GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			   (LPCSTR) sysInitializeLinker, &self))
	add_lib(self);

    if (path == NULL)
	path = "";
    if ((buf = malloc(strlen(home) + strlen(path) + 6)) != NULL)
	sprintf(buf, "%s\\bin;%s", home, path);
    return buf;
}

/*
 * Add a shared library to the list
 */
int sysAddDLSegment(char *fn)
{
    HMODULE h = LoadLibraryA(fn);

    if (h == NULL)
	return 0;
    add_lib(h);
    return 1;
}

long sysDynamicLink(char *sym_p, void* *lib)
{
    FARPROC f = NULL;
    int i;

    if (lib != NULL)
	*lib = NULL;
    if (nlibs == 0)
	free(sysInitializeLinker());	/* registers javai.dll */

    AcquireSRWLockShared(&libs_lock);
    for (i = 0; i < nlibs && f == NULL; i++) {
	if ((f = GetProcAddress(libs[i], sym_p)) != NULL && lib != NULL)
	    *lib = libs[i];
    }
    ReleaseSRWLockShared(&libs_lock);
    return (long) f;
}
