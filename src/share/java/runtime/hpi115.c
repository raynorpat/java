/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/* JDK 1.1 host API additions.  See tests/native/hpi115-evidence.md. */
#include "sys_api.h"
#include <sys/stat.h>
#ifdef XP_PC
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#endif

int sysAvailable(int fd, long *bytes)
{
    struct stat st;
    long current, end;
    int status;
#ifdef XP_PC
    HANDLE handle;
    DWORD ready, available;
#else
    int available;
#endif
    status = fstat(fd, &st);
    if (status < 0) return status;
#ifdef XP_PC
    if ((st.st_mode & _S_IFCHR) || (st.st_mode & _S_IFIFO)) {
        if (fd == 0) {
            handle = GetStdHandle(STD_INPUT_HANDLE);
            if (handle == INVALID_HANDLE_VALUE) return 0;
            ready = WaitForSingleObject(handle, 0);
            if (ready == WAIT_OBJECT_0) { *bytes = 1; return 1; }
            if (ready == WAIT_ABANDONED || ready == WAIT_TIMEOUT) {
                *bytes = 0; return 1;
            }
            return 0;
        }
        handle = (HANDLE)_get_osfhandle(fd);
        if (handle == INVALID_HANDLE_VALUE) return 0;
        if (!PeekNamedPipe(handle, NULL, 0, NULL, &available, NULL)) {
            if (GetLastError() != ERROR_BROKEN_PIPE) return 0;
            available = 0;
        }
        *bytes = (long)available;
        return 1;
    }
#else
    /* POSIX mapping of pipe/device readiness; ioctl writes an int. */
    if (S_ISCHR(st.st_mode) || S_ISFIFO(st.st_mode) || S_ISSOCK(st.st_mode)) {
        if (ioctl(fd, FIONREAD, &available) >= 0) {
            *bytes = available;
            return 1;
        }
    }
#endif
    current = sysSeek(fd, 0, SEEK_CUR);
    if (current == -1) return 0;
    end = sysSeek(fd, 0, SEEK_END);
    if (end == -1 || sysSeek(fd, current, SEEK_SET) == -1) return 0;
    *bytes = end - current;
    return 1;
}

int sysIsMP(void)
{
#ifdef XP_PC
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwNumberOfProcessors > 1;
#else
    return sysconf(_SC_NPROCESSORS_ONLN) > 1;
#endif
}

#ifndef sysMemoryFlush
void sysMemoryFlush(void)
{
#ifdef XP_PC
    /* Sun Win32 sysmacros_md.h uses this full locked instruction. */
    __asm { lock xor DWORD PTR [esp], 0 }
#else
    __sync_synchronize();
#endif
}

#endif
#ifndef sysStoreBarrier
void sysStoreBarrier(void)
{
#ifndef XP_PC
    __atomic_thread_fence(__ATOMIC_RELEASE);
#endif
}

#endif
