#ifndef sun_java_str2id_h___
#define sun_java_str2id_h___

#define STR2ID_HTSIZE 2003		/* must be prime */
#define kHashTableArenaIncrementSize	4 * 1024

#include "arena.h"

typedef struct StrIDhash {
    struct StrIDhash *next;
    short used, baseid;
    char *hash[STR2ID_HTSIZE];
    unsigned is_malloced[(STR2ID_HTSIZE + 31) >> 5];
    ArenaPool	stringArena;
    void **param;
} StrIDhash;

#endif /* sun_java_str2id_h___ */
