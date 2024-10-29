#include "../headers/arena.h"
#include <string.h>
#include <stdio.h>

Arena *arena_create(size_t block_size){
    Arena *a = malloc(sizeof(Arena));
    if (!a) return NULL;

    a->memory = malloc(block_size);
    if (!a->memory) {
        free(a);
        return NULL;
    }

    a->used = 0;
    a->total_size = block_size;
    a->next = NULL;

    return a;
}

void *arena_alloc(Arena *a, size_t size){
    if (a->used + size > a->total_size) {
        // só cria um novo bloco se não houver espaço suficiente
        size_t new_block_size = (size > ARENA_BLOCK_SIZE) ? size : ARENA_BLOCK_SIZE;
        Arena *new_arena = arena_create(new_block_size);
        if (!new_arena) return NULL;

        new_arena->next = a;
        a = new_arena;
    }

    void *ptr = a->memory + a->used;
    a->used += size;

    return ptr;
}

void arena_free(Arena *a) {
    while (a) {
        Arena *next = a->next;
        free(a->memory);
        a->memory = NULL;
        free(a);
        a = next;
    }
    free(a);
    a = NULL;
}

const char *arena_get_by_index(arena_t *arena, size_t index) {
    if (index >= arena->total) return NULL;

    size_t offset = 0;
    for (size_t i = 0; i < index; i++) {
        size_t len = strlen(arena->arena->memory + offset);
        if (offset + len >= arena->arena->total_size) {
            printf("Erro: acesso fora dos limites\n");
            return NULL;
        }
        offset += len + 1;
    }
    return arena->arena->memory + offset;
}
