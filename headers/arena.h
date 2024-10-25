#ifndef ARENA_H
#define ARENA_H

#include <stdlib.h>

#define ARENA_BLOCK_SIZE 4096

typedef struct Arena {
    char *memory;
    size_t used;
    size_t total_size;
    struct Arena *next;
} Arena;

typedef struct arena_t {
    size_t total;
    Arena *arena;
} arena_t;

Arena *arena_create(size_t block_size);
void *arena_alloc(Arena *a, size_t size);
void arena_free(Arena *a);
const char *arena_get_by_index(arena_t *arena, size_t index);

#endif
