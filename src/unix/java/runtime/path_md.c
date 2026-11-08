/*
 * @(#)path_md.c	1.18 95/10/18
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

/*
 * Machine dependent path name and file name manipulation code
 */

#if defined(__APPLE__) && !defined(JDK115_CTHREADS)
#include <mach-o/dyld.h>	/* _NSGetExecutablePath */
#elif defined(__APPLE__) && defined(JDK115_CTHREADS)
#include <crt_externs.h>
#endif
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "sys_api.h"
#include "io_md.h"
#include "path.h"	/* for cpe_t */
#include "zip.h"	/* for zip_t */

/*
 * The JDK directory: the running program lives in <java home>/bin.
 * Where the program's path can't be found, JAVA_HOME names it.
 */
const char *
sysJavaHome(void)
{
    static char home[MAXPATHLEN];
    char *p;

    if (home[0] == 0) {
#if defined(__APPLE__) && !defined(JDK115_CTHREADS)
	uint32_t size = sizeof home;
	if (_NSGetExecutablePath(home, &size) != 0)
	    home[0] = 0;
#elif defined(__linux__) && !defined(JDK115_CTHREADS)
	ssize_t n = readlink("/proc/self/exe", home, sizeof home - 1);
	home[n > 0 ? n : 0] = 0;
#elif defined(__APPLE__) && defined(JDK115_CTHREADS)
	if (!realpath((*_NSGetArgv())[0], home)) home[0] = 0;
#endif
	if (home[0] == 0) {
	    p = getenv("JAVA_HOME");
	    strncpy(home, p ? p : ".", sizeof home - 1);
	    return home;
	}
	if ((p = strrchr(home, '/')) != NULL) *p = 0;	/* program */
	if ((p = strrchr(home, '/')) != NULL) *p = 0;	/* bin */
    }
    return home;
}

char *
sysNativePath(char *path)
{
    return path;
}

static const char* clpath = NULL;

JAVA_PUBLIC_API(const char*)
CLASSPATH(void)
{
	return clpath;
}

JAVA_PUBLIC_API(void)
setCLASSPATH(const char* cp)
{
	if (clpath) free((char*)clpath);
	clpath = cp;
}

cpe_t **
sysGetClassPath(void)
{
	static cpe_t **classpath;
	
	if (classpath == 0) {			/* once and only once */
		char *cps, *strPtr;
		int numClassPathEntries = 1;
		cpe_t **cpp;
		
		/* 	Grab the environment variable containting the list of class paths
			and make a copy of it for local munging. */
		if ((cps = (char*)CLASSPATH()) == 0 && (cps = getenv("CLASSPATH")) == 0) {
			/* The optional Linux peer archive supplements the baseline classes. */
			static char def[2 * MAXPATHLEN + 64];
			char peers[MAXPATHLEN + 32];
			struct stat info;
			sprintf(def, ".:%s/lib/classes.zip", sysJavaHome());
#ifdef JDK115_CTHREADS
			sprintf(peers, "%s/lib/rhapsody-peers.zip", sysJavaHome());
#else
			sprintf(peers, "%s/lib/motif-peers.zip", sysJavaHome());
#endif
			if (stat(peers, &info) == 0 && S_ISREG(info.st_mode)) {
				strcat(def, ":");
				strcat(def, peers);
			}
			cps = def;
		}
		if (CLASSPATH() == 0) {
			setCLASSPATH(strdup(cps));	/* reported as java.class.path */
		}
		if ((cps = strdup(cps)) == 0) {
			return 0;				/* Out of memeory */
		}
		
		/* Count the number of paths in the class path list */
		for (strPtr = cps; *strPtr != '\0'; strPtr++) {
			if (*strPtr == PATH_SEPARATOR) {
				numClassPathEntries++;
			}
		}
		
		/* Allocate a class path element structure for each path in the list */
		cpp = classpath = (cpe_t **) sysMalloc((numClassPathEntries + 1) * sizeof(cpe_t *));
		if (cpp == 0) {
			return 0;				/* Out of memeory */
		}
		
		while (cps && *cps) {		/* For each path in the list */
			struct stat statBuf;
			char *path = cps;
			if ((cps = strchr(cps, PATH_SEPARATOR)) != 0) {
				*cps++ = '\0';
			}
			if (*path == '\0') {
				path = ".";
			}
			sysNativePath(path);
			if (sysStat(path, &statBuf) == 0 && S_ISREG(statBuf.st_mode)) {		/* it's a file */
				int len = strlen(path);
				if ((len > 4) && (strcasecmp(path+len-4, ".zip") == 0)) { /* Is it a zip file? */
					zip_t *zip = zip_open(path);
					if (zip != 0) {												/* successfully opened it */
						cpe_t *cpe = sysMalloc(sizeof(cpe_t));
						if (cpe == 0) {
							return 0;				/* Out of memeory */
						}
						cpe->type = CPE_ZIP;
						cpe->u.zip = zip;
						*cpp++ = cpe;
					}
				}
			} else {										/* not a file, assume directory */
				cpe_t *cpe = sysMalloc(sizeof(cpe_t));
				if (cpe == 0) {
					return 0;				/* Out of memeory */
				}
				cpe->type = CPE_DIR;
				cpe->u.dir = path;
				*cpp++ = cpe;
			}
		}
		*cpp = 0;
	}
	return classpath;
}

