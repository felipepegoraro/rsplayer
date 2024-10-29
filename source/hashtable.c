#include "../headers/hashtable.h"
#include <string.h>
#include <openssl/sha.h> // libssl-dev => -lssl -lcrypto
#include "../headers/id3.h"

unsigned long key_sha256_hash(const char *key) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)key, strlen(key), hash);

    unsigned long result = 0;
    for (int i = 0; i < 4; i++)
        result = (result << 8) | hash[i];
    return result;
}

HashTable hs_create(void){
    HashTable hs;

    hs.size = INITIAL_BUCKET_SIZE;
    hs.length = 0;
    hs.arena = arena_create(ARENA_BLOCK_SIZE);

    if (!hs.arena) {
        printf("nao foi possivel alocar memoria para arena da tabela hash\n");
        hs.buckets = NULL;
        return hs;
    }

    hs.buckets = (Entry **)arena_alloc(hs.arena, hs.size * sizeof(Entry *));

    if (!hs.buckets){
        arena_free(hs.arena);
        hs.arena = NULL;
        return hs;
    }

    for (size_t i=0; i< hs.size; i++)
        hs.buckets[i] = NULL;

    return hs;
}

void hs_free(HashTable *hs){
    if (!hs) return;

    arena_free(hs->arena);

    hs = NULL;
}

size_t hs_insert(HashTable *hs, Music *value, const char *songName) {
    if (!hs || !value) return 0;


    size_t title_size = strlen(value->tags.title);
    if (title_size == 0 && (!songName || strlen(songName) == 0)) {
        printf("erro ao gerar a chave: valores inválidos\n");
        return 0;
    }

    char key_input[T_ID3V2_MAX*3+6] = {};

    if (title_size > 0){
        snprintf(key_input, sizeof(key_input), "%s-%s-%s-%s", 
             value->tags.title,
             value->tags.artist, 
             value->tags.album, 
             value->tags.year);
    } else {
        strcpy(key_input, songName);
    }

    printf("key_input: %s\n", key_input);

    unsigned long key = key_sha256_hash(key_input);
    printf("Chave gerada: %lu\n", key);

    size_t index = key % hs->size;

    Entry *new_entry = (Entry *)arena_alloc(hs->arena, sizeof(Entry));
    if (!new_entry) {
        printf("erro ao alocar nova entrada na arena\n");
        return 0;
    }

    new_entry->key = key;
    new_entry->music = value;
    new_entry->next = NULL;

    if (hs->buckets[index] == NULL) {
        hs->buckets[index] = new_entry;
    } else {
        new_entry->next = hs->buckets[index];
        hs->buckets[index] = new_entry;
    }

    hs->length++;
    return index;
}

Entry *hs_search(const HashTable *hs, unsigned long key){
    if (!hs || !key) return NULL;

    size_t index = key % hs->size;
    Entry *current = hs->buckets[index];
    if (current && current->next) {
        printf("Colisão detectada no índice %zu para a chave %lu\n", index, key);
    } else if (current) {
        printf("Sem colisão no índice %zu para a chave %lu\n", index, key);
    }
    
    while (current) {
        if (current->key == key) {
            printf("Chave %lu encontrada no índice %zu\n", key, index);
            return current;
        }
        current = current->next;
    }

    return current;
}

int hs_delete(HashTable*, unsigned long key);
