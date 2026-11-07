#ifndef UNIX_HOST_MUTEX_H
#define UNIX_HOST_MUTEX_H
#ifdef JDK115_CTHREADS
#include "cthreads/host_mutex.h"
#else
#include <pthread.h>
typedef pthread_mutex_t host_mutex_t;
#define HOST_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define host_mutex_init(m) pthread_mutex_init((m), NULL)
#define host_mutex_destroy(m) pthread_mutex_destroy(m)
#define host_mutex_lock(m) pthread_mutex_lock(m)
#define host_mutex_unlock(m) pthread_mutex_unlock(m)
#endif
#endif
