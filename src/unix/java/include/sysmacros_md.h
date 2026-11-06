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


#define sysOpen(name,mode,mask)         open(name,mode,mask)
#define sysRead(fd,buf,n)               read(fd,buf,n)
#define sysWrite(fd,buf,n)              write(fd,buf,n)
#define sysSeek(fd,off,whence)          lseek(fd,off,whence)
#define sysClose(fd)                    close(fd)
#define sysCloseSocket(fd)              close(fd)
#define sysIsAbsolute(path)             (*(path) == '/')
#define sysUnlink(path)                 unlink(path)
#define sysStat(path,buf)               stat(path,buf)
#define sysFStat(fd,buf)                fstat(fd,buf)
#define sysRename(src,dst)              rename(src,dst)
#define sysRmdir(path)                  rmdir(path)
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

/* Retained asynchronous collector polling; not part of Sun monitor.h. */
#define INTERRUPTS_PENDING() sysInterruptsPending()
#define sysCurrentCacheKey(tid) ((tid)->cacheKey)
#define sysLocalMonCache(tid, hash) ((tid)->monitorCache[hash])

/* POSIX adaptation of the original recursive monitor-cache mutex. */
#include <pthread.h>
#define SYS_TLS_MONCACHE 8
typedef struct {
    pthread_mutex_t mutex;
    long entry_count;
    pthread_t owner;
} cache_lock_t;
extern cache_lock_t _moncache_lock;
void sysCacheLockInit(void);
void sysCacheLock(void);
void sysCacheUnlock(void);
int sysCacheLocked(void);

#endif /*SYSMACROS_MD_H_*/
