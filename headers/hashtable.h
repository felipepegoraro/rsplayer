#ifndef HASHTABLE_H
#define HASHTABLE_H

#define MAX_HS_SIZE 1024
#define MAX_KEY_SIZE 25
#define INITIAL_BUCKET_SIZE 128

#include "../headers/id3.h"
#include "../headers/arena.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct entry {
    unsigned long key;
    Music *music;
    struct entry *next;
} Entry;

typedef struct hashtable {
    size_t size;
    size_t length;
    Arena *arena;
    Entry **buckets;
} HashTable;

HashTable hs_create(void);
void hs_free(HashTable*);

void hs_insert(HashTable *hs, Music *value, const char *songName);
Entry *hs_search(const HashTable *hs, const char *key);
int hs_delete(HashTable*, const char *key);

// void hs_map(
//   HashTable*,
//   void (*callback)()
// );

#endif // !HASHTABLE_H
