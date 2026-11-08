/* Reconstructed from JDK 1.1.5 javai.dll (Win32) and Rhapsody DR2 libjava.A.dylib. */
/* Monitor cache reconstructed from Sun javai_g.dll 10037070..10038199.
 * See tests/native/monitor-cache115-evidence.md for the binary contract. */
#include "threads.h"
#include "threads_md.h"
#include "monitor_md.h"
#include "monitor.h"
#include "monitor_cache.h"
#include "log.h"
#include "sys_api.h"

#define INIT_HASHTABLE_BUCKETS 128
#define INIT_MONITOR_COUNT 128
#define EXPANSION_QUANTUM 16
#define HASHTABLE_MULTIPLE 2
#define FREE_MONITOR_THRESHOLD 5
#define HASH(k) ((((k) >> 2) ^ ((k) >> 10)) & (monHashTableBuckets - 1))
#define UNTAG(p) ((monitor_t *)((unsigned long)(p) & ~1UL))

static monitor_t *initMonHashTable[INIT_HASHTABLE_BUCKETS];
monitor_t **monHashTable = initMonHashTable;
int monHashTableBuckets = INIT_HASHTABLE_BUCKETS;
int monCount;
static monitor_t *monFreeList;
static int monFreeCount;
static sys_thread_t *expandingThread;
bool_t monitorsInitialized;
int systemIsMP;
cache_lock_t _moncache_lock;

#ifndef XP_PC
#ifdef JDK115_CTHREADS
void sysCacheLockInit(void)
{
    memset(&_moncache_lock, 0, sizeof(_moncache_lock));
}
int sysCacheLocked(void)
{
    return _moncache_lock.entry_count > 0 && _moncache_lock.owner == cthread_self();
}
void sysCacheLock(void)
{
    if (!sysCacheLocked()) {
        if (!mutex_try_lock(&_moncache_lock.mutex)) mutex_wait_lock(&_moncache_lock.mutex);
        _moncache_lock.owner = cthread_self();
    }
    ++_moncache_lock.entry_count;
}
void sysCacheUnlock(void)
{
    sysAssert(sysCacheLocked());
    if (--_moncache_lock.entry_count == 0) {
        _moncache_lock.owner = NULL;
        _moncache_lock.mutex.lock = 0;
    }
}
#else
void sysCacheLockInit(void)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&_moncache_lock.mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    _moncache_lock.entry_count = 0;
}
void sysCacheLock(void)
{
    pthread_mutex_lock(&_moncache_lock.mutex);
    if (_moncache_lock.entry_count++ == 0)
        _moncache_lock.owner = pthread_self();
}
int sysCacheLocked(void)
{
    return _moncache_lock.entry_count > 0 &&
        pthread_equal(_moncache_lock.owner, pthread_self());
}
void sysCacheUnlock(void)
{
    sysAssert(sysCacheLocked());
    --_moncache_lock.entry_count;
    pthread_mutex_unlock(&_moncache_lock.mutex);
}
#endif
#endif

/* Primary 100370ED. Caller must not hold the cache lock. */
void expandMonCache(int count)
{
    size_t stride = sizeof(monitor_t) + sysMonitorSizeof();
    monitor_t *start, *mon;
    monitor_t **oldtable, **newtable, **slot;
    int i, oldbuckets, buckets;
    if (count <= 0) return;
    start = (monitor_t *)sysCalloc(count, stride);
    if (!start) out_of_memory();
    Log2(2, "Expanding monitor cache by %d monitors to %d\n", count, monCount + count);
    CACHE_LOCK();
    monCount += count;
    mon = start;
    for (i = 1; i < count; ++i) {
        monitor_t *next = (monitor_t *)((char *)mon + stride);
        monitorInit(mon);
        mon->next = next;
        mon = next;
    }
    monitorInit(mon);
    mon->next = monFreeList;
    monFreeList = start;
    if (HASHTABLE_MULTIPLE * monHashTableBuckets > monCount) {
        monFreeCount += count;
        CACHE_UNLOCK();
        return;
    }
    oldbuckets = buckets = monHashTableBuckets;
    oldtable = monHashTable;
    do { buckets *= 2; } while (HASHTABLE_MULTIPLE * buckets <= monCount);
    Log2(2, "Expanding monitor cache hash table from %d to %d buckets\n",
         oldbuckets, buckets);
    CACHE_UNLOCK();
    newtable = (monitor_t **)sysCalloc(buckets, sizeof(monitor_t *));
    CACHE_LOCK();
    if (!newtable) {
        monFreeCount += count;
        CACHE_UNLOCK();
        return;
    }
    monHashTable = newtable;
    monHashTableBuckets = buckets;
    for (i = 0; i < oldbuckets; ++i) {
        mon = oldtable[i];
        while (mon) {
            monitor_t *next = mon->next;
            slot = &monHashTable[HASH(mon->key)];
            mon->next = *slot;
            *slot = mon;
            mon = next;
        }
    }
    monFreeCount += count;
    CACHE_UNLOCK();
    if (oldtable != initMonHashTable) sysFree(oldtable);
}

/* Primary 10037AFC: invalidate local caches before sampling current keys. */
static int markThreadCache(sys_thread_t *thread, void *unused)
{
    unsigned int key;
    monitor_t *mon;
    int i;
    for (i = 0; i < SYS_TLS_MONCACHE; ++i) sysLocalMonCache(thread, i) = NULL;
    if (systemIsMP) sysMemoryFlush();
    key = sysCurrentCacheKey(thread);
    if (key) {
        for (mon = monHashTable[HASH(key)]; mon; mon = UNTAG(mon->next)) {
            if (mon->key == key) {
                mon->next = (monitor_t *)((unsigned long)mon->next | MON_LOCAL_CACHE_REF);
                break;
            }
        }
    }
    return 0;
}

/* Primary 1003797E, with both thread queue and cache locked. */
static int reclaimMonitors(void)
{
    monitor_t **slot, *mon;
    int i, reclaimed = 0;
    sysAssert(QUEUE_LOCKED() && CACHE_LOCKED());
    sysThreadEnumerateOver(markThreadCache, NULL);
    for (i = monHashTableBuckets; --i >= 0;) {
        slot = &monHashTable[i];
        while ((mon = *slot) != NULL) {
            if (((unsigned long)mon->next & MON_LOCAL_CACHE_REF) ||
                sysMonitorInUse(&sysmon(mon))) {
                mon->next = UNTAG(mon->next);
                slot = &mon->next;
            } else {
                *slot = mon->next;
                mon->key = 0;
                mon->next = monFreeList;
                monFreeList = mon;
                ++monFreeCount;
                ++reclaimed;
            }
        }
    }
    return reclaimed;
}

/* Primary 10037588. Returns with the caller's cache lock reacquired. */
static int replenishMonitors(void)
{
    int retry = 0;
    sysAssert(CACHE_LOCKED());
    CACHE_UNLOCK();
    if (!_queue_lock) {
        expandMonCache(EXPANSION_QUANTUM);
        CACHE_LOCK();
        return 1;
    }
    QUEUE_LOCK();
    CACHE_LOCK();
    if (!expandingThread && monFreeCount < FREE_MONITOR_THRESHOLD) {
        reclaimMonitors();
        if (monFreeCount < FREE_MONITOR_THRESHOLD) {
            CACHE_UNLOCK();
            expandingThread = sysThreadSelf();
            QUEUE_UNLOCK();
            expandMonCache(EXPANSION_QUANTUM);
            QUEUE_LOCK();
            expandingThread = NULL;
            sysMonitorNotifyAll(_queue_lock);
            QUEUE_UNLOCK();
        } else {
            CACHE_UNLOCK();
            QUEUE_UNLOCK();
        }
        retry = 1;
    } else if (expandingThread == sysThreadSelf()) {
        CACHE_UNLOCK();
        QUEUE_UNLOCK();
    } else {
        CACHE_UNLOCK();
        while (expandingThread) QUEUE_WAIT();
        QUEUE_UNLOCK();
        retry = 1;
    }
    CACHE_LOCK();
    return retry;
}

void monitorCacheInit(void)
{
    CACHE_LOCK_INIT();
    expandMonCache(INIT_MONITOR_COUNT);
    monitorsInitialized = TRUE;
    systemIsMP = sysIsMP();
}

monitor_t *findMonitor(sys_thread_t *thread, unsigned int key, bool_t create)
{
    monitor_t *mon;
    int hash;
    CACHE_LOCK();
    for (;;) {
        hash = HASH(key);
        for (mon = monHashTable[hash]; mon; mon = mon->next) {
            if (mon->key == key) goto found;
        }
        if (!create) { CACHE_UNLOCK(); return NULL; }
        if (monFreeCount < FREE_MONITOR_THRESHOLD && replenishMonitors()) continue;
        sysAssert(monFreeList != NULL);
        mon = monFreeList;
        monFreeList = mon->next;
        --monFreeCount;
        sysAssert(!sysMonitorInUse(&sysmon(mon)));
        mon->key = key;
        mon->next = monHashTable[hash];
        monHashTable[hash] = mon;
found:
        if (thread) sysLocalMonCache(thread, (key >> 3) & (SYS_TLS_MONCACHE - 1)) = mon;
        CACHE_UNLOCK();
        return mon;
    }
}

monitor_t *lookupMonitor(unsigned int key)
{
    sys_thread_t *thread = sysThreadSelf();
    monitor_t *mon;
    checkCache(mon, thread, key);
    return mon ? mon : findMonitor(thread, key, FALSE);
}

monitor_t *createMonitor(unsigned int key)
{
    sys_thread_t *thread = sysThreadSelf();
    monitor_t *mon;
    checkCache(mon, thread, key);
    return mon ? mon : findMonitor(thread, key, TRUE);
}

void monitorEnumerate_unlocked(void (*fcn)(monitor_t *, void *), void *cookie)
{
    monitor_t *mon, *next;
    int i;
    sysAssert(CACHE_LOCKED());
    for (i = monHashTableBuckets; --i >= 0;) {
        mon = UNTAG(monHashTable[i]);
        while (mon) {
            next = UNTAG(mon->next);
            fcn(mon, cookie);
            mon = next;
        }
    }
}

void monitorEnumerate(void (*fcn)(monitor_t *, void *), void *cookie)
{
    CACHE_LOCK();
    monitorEnumerate_unlocked(fcn, cookie);
    CACHE_UNLOCK();
}

#ifdef DEBUG

void DumpMonCacheInfo()
{
    int i;
    monitor_t *mon;

    fprintf(stderr, "Monitor cache info:\n");
    fprintf(stderr, "  Initial monitor count: %d\n", INIT_MONITOR_COUNT);    
    fprintf(stderr, "  Initial hash table buckets: %d\n",
	    INIT_HASHTABLE_BUCKETS);  
    fprintf(stderr, "  Hash table expansion multiple: %d\n", HASHTABLE_MULTIPLE);
    fprintf(stderr, "  Minimum number of free monitors before expansion: %d\n",
	    FREE_MONITOR_THRESHOLD);    
    fprintf(stderr, "  Number of new monitors added per expansion: %d\n",
	    EXPANSION_QUANTUM);    
    fprintf(stderr, "  Current total number of monitors: %d\n", monCount);
    fprintf(stderr, "  Current number of free monitors: %d\n", monFreeCount);
    fprintf(stderr, "  Current number of hash table buckets (power of 2!): %d\n",
	    monHashTableBuckets);

    fprintf(stderr, "  Current hash table bucket dump:\n");
    for (i = 0; i < monHashTableBuckets; i++) {
	mon = monHashTable[i];
	if (mon != 0) {
	    fprintf(stderr, "    Bucket %d: ", i);
	    while (mon != 0) {
	        fprintf(stderr, "0x%lx ", mon);
	        mon = UNTAG(mon->next);
	    }
	    fprintf(stderr, "\n");
	}
    }
}

#endif
