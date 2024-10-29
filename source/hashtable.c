#include "../headers/hashtable.h"
#include <string.h>
#include <openssl/sha.h> // libssl-dev => -lssl -lcrypto

unsigned long key_sha256_hash(const char *key) {
    unsigned char hash[SHA_DIGEST_LENGTH];
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

void hs_insert(HashTable *hs, Music *value, const char *songName) {
    if (!hs || !value) return;

    const char *key_input = NULL;

    if (strlen(value->tags.tags.title) > 0) key_input = value->tags.tags.title;
    else if (songName && strlen(songName) > 0) key_input = songName;

    if (!key_input) {
        printf("erro ao gerar a chave: valores invalidos\n");
        return;
    }

    unsigned long key = key_sha256_hash(key_input);
    printf("Chave gerada: %lu\n", key);
}

Entry *hs_search(const HashTable *hs, const char *key);
int hs_delete(HashTable*, const char *key);
