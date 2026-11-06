#ifndef _ARENA_H_
#define _ARENA_H_

#include <stddef.h>

/*
 * Bump allocator for memory that is freed all at once (per-class storage,
 * interned strings). Allocations are 8-byte aligned and not zeroed.
 */
typedef struct ArenaChunk ArenaChunk;

typedef struct ArenaPool {
    ArenaChunk *head;		/* most recent chunk */
    size_t chunksize;		/* usable bytes per normal chunk */
} ArenaPool;

void  ArenaInit(ArenaPool *pool, size_t chunksize);
void *ArenaAlloc(ArenaPool *pool, size_t size);	/* NULL when out of memory */
void  ArenaFreeAll(ArenaPool *pool);		/* pool stays usable */

#endif /* !_ARENA_H_ */
