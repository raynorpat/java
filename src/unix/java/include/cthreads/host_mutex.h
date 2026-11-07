/* Small native locks for services which can run outside a Java thread. */
#ifndef MACH115_HOST_MUTEX_H
#define MACH115_HOST_MUTEX_H
#include <mach/cthreads.h>
typedef struct mutex host_mutex_t;
#define HOST_MUTEX_INITIALIZER {0}
#define host_mutex_init(m) memset((m), 0, sizeof(*(m)))
#define host_mutex_destroy(m) ((void)0)
#define host_mutex_lock(m) do { if (!mutex_try_lock(m)) mutex_wait_lock(m); } while (0)
#define host_mutex_unlock(m) ((m)->lock = 0)
#endif
