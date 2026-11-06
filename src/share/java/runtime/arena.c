#include <stdlib.h>
#include "arena.h"

#define ARENA_ALIGN 8

struct ArenaChunk {
    ArenaChunk *next;
    char *avail, *limit;
    double data[1];		/* aligns the first allocation */
};

void ArenaInit(ArenaPool *pool, size_t chunksize)
{
    pool->head = NULL;
    pool->chunksize = chunksize;
}

void *ArenaAlloc(ArenaPool *pool, size_t size)
{
    ArenaChunk *c = pool->head;
    char *p;

    size = (size + ARENA_ALIGN - 1) & ~(size_t)(ARENA_ALIGN - 1);
    if (c == NULL || (size_t)(c->limit - c->avail) < size) {
	/* Oversized requests get a chunk of their own. */
	size_t n = size > pool->chunksize ? size : pool->chunksize;
	if ((c = (ArenaChunk *) malloc(offsetof(ArenaChunk, data) + n)) == NULL)
	    return NULL;
	c->avail = (char *) c->data;
	c->limit = c->avail + n;
	c->next = pool->head;
	pool->head = c;
    }
    p = c->avail;
    c->avail += size;
    return p;
}

void ArenaFreeAll(ArenaPool *pool)
{
    ArenaChunk *c = pool->head, *next;

    for (; c != NULL; c = next) {
	next = c->next;
	free(c);
    }
    pool->head = NULL;
}
