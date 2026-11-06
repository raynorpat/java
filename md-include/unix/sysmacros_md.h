#ifndef SYSMACROS_MD_H_
#define SYSMACROS_MD_H_

/* Unix: the HPI file calls are the POSIX ones. */
#include <unistd.h>
#include <fcntl.h>
#include <sys/param.h>
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

#ifndef MAXPATHLEN
#define MAXPATHLEN                      1024
#endif

/* Directory listing and OS identification (dir_md.c) */
extern void sysOSInfo(char *name, int namelen, char *version, int versionlen);
typedef struct sys_dir sys_dir_t;
typedef struct { char *d_name; } sys_dirent_t;
extern sys_dir_t *sysOpenDir(const char *path);
extern sys_dirent_t *sysReadDir(sys_dir_t *dir);
extern void sysCloseDir(sys_dir_t *dir);

#define sysOpen(name,mode,mask)         open(name,mode,mask)
#define sysRead(fd,buf,n)               read(fd,buf,n)
#define sysWrite(fd,buf,n)              write(fd,buf,n)
#define sysSeek(fd,off,whence)          lseek(fd,off,whence)
#define sysClose(fd)                    close(fd)
#define sysCloseSocket(fd)              close(fd)
#define sysIsAbsolute(path)             (*(path) == '/')
#define sysUnlink(path)                 unlink(path)
#define sysStat(path,buf)               stat(path,buf)
#define sysMkdir(path,mode)             mkdir(path,mode)
#define sysAccess(path,mode)            access(path,mode)

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
