/*
 * @(#)sysmacros_md.h	1.1 95/10/06  
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

#ifndef SYSMACROS_MD_H_
#define SYSMACROS_MD_H_

/* JDK 1.1.5 per-thread monitor cache size. */
#define SYS_TLS_MONCACHE 8

/* Win32: the HPI file calls map onto the C runtime. */
#include <io.h>
#include <direct.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>

#define sysMalloc(size) (malloc(size))
#define sysFree(ptr)	(free(ptr))
#define sysCalloc(num, size) (calloc(num, size))
#define sysRealloc(ptr, size) (realloc(ptr, size))

#ifdef DEBUG
#define sysAssert(ex) assert(ex)
#else
#define sysAssert(ex) ((void)0)
#endif

/************************************************************************/

#define MAXPATHLEN                      _MAX_PATH


/* Directory listing and OS identification (dir_md.c) */
extern void sysOSInfo(char *name, int namelen, char *version, int versionlen);
/* deviation: compatibility names for existing File.list/native tests. */
#include "io_md.h"
typedef DIR sys_dir_t;
typedef struct dirent sys_dirent_t;

/* deviation: retained collector/cleanup adapters, not Sun public HPI. */
struct sys_thread;
struct sys_mon;
void **sysThreadRegisters(struct sys_thread *, int *);
void sysThreadSetBackPtr(struct sys_thread *, void *);
int sysMonitorCleanup(struct sys_mon *, struct sys_thread *);
#define SYS_DESTROY -7
#define SYS_INVALID -2
#define SYS_NOTHREAD -3
#define SYS_MINSTACK -4


#define sysOpen(name,mode,mask)         _open(name,(mode)|O_BINARY,mask)
#define sysRead(fd,buf,n)               _read(fd,buf,n)
#define sysWrite(fd,buf,n)              _write(fd,buf,n)
#define sysSeek(fd,off,whence)          _lseek(fd,off,whence)
#define sysClose(fd)                    _close(fd)
#define sysCloseSocket(fd)              closesocket(fd)
#define sysIsAbsolute(path)             (*(path) == '\\')
#define sysUnlink(path)                 _unlink(path)
#define sysStat(path,buf)               stat(path,buf)
#define sysFStat(fd,buf)                fstat(fd,buf)
#define sysRename(src,dst)              rename(src,dst)
#define sysRmdir(path)                  _rmdir(path)
#define sysMkdir(path,mode)             _mkdir(path)

/* Symbolic constants for the access() function */
#define R_OK    4                       /*  Test for read permission    */
#define W_OK    2                       /*  Test for write permission   */
#define F_OK    0                       /*  Test for existence of file  */

#define sysAccess(path,mode)            _access(path,mode)

/************************************************************************/

/*
 * Check whether an exception occurred.  This also gives us the oppor-
 * tunity to use the already-required exception check as a trap for other
 * system-specific conditions.
 */
#define sysCheckException(ee)		\
	if (!exceptionOccurred(ee)) {	\
	   continue;			\
	}

#endif /*SYSMACROS_MD_H_*/
