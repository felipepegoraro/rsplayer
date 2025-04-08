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

void *arena_alloc(Arena *a, size_t size) {
    Arena *current = a;

    while (current) {
        if (current->used + size <= current->total_size) {
            void *ptr = current->memory + current->used;
            current->used += size;
            return ptr;
        }
        if (!current->next) break;
        current = current->next;
    }

    size_t new_block_size = (size > ARENA_BLOCK_SIZE) ? size : ARENA_BLOCK_SIZE;
    Arena *new_arena = arena_create(new_block_size);
    if (!new_arena) return NULL;

    current->next = new_arena;
    void *ptr = new_arena->memory;
    new_arena->used = size;
    return ptr;
}


// void *arena_alloc(Arena *a, size_t size){
//     if (a->used + size > a->total_size) {
//         // só cria um novo bloco se não houver espaço suficiente
//         size_t new_block_size = (size > ARENA_BLOCK_SIZE) ? size : ARENA_BLOCK_SIZE;
//         Arena *new_arena = arena_create(new_block_size);
//         if (!new_arena) return NULL;
//
//         new_arena->next = a;
//         a = new_arena;
//     }
//
//     void *ptr = a->memory + a->used;
//     a->used += size;
//
//     return ptr;
// }

void arena_free(Arena *a) {
    while (a) {
        Arena *next = a->next;
        free(a->memory);
        a->memory = NULL;
        free(a);
        a = next;
    }
    if (!a){
        printf("OK sem free\n");
    } else {
        printf("OK free\n");
        free(a);
    }
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
