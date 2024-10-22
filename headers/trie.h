#ifndef PREFIX_TRIE_H
#define PREFIX_TRIE_H

#include "./arena.h"

#define ALPHABET_SIZE 40 // 26letras+10digitos+1ponto+2traços+1espaco
#define INITIAL_TRIE_ARENA_SIZE 1024
#define MAX_WORD_SIZE 256

typedef struct TrieNode {
    char value;
    int is_terminal;
    struct TrieNode *children[ALPHABET_SIZE];
} TrieNode;

typedef struct {
    TrieNode *root;
    Arena *arena;
} PrefixTrie;

TrieNode *trie_node_new(Arena *arena);
PrefixTrie ptrie_new(void);

void ptrie_free(PrefixTrie *trie);

void ptrie_insert(PrefixTrie *trie, const char *word);
int ptrie_delete(PrefixTrie *trie, const char *word);

int ptrie_search(PrefixTrie *trie, const char *word);
int ptrie_starts_with(PrefixTrie *trie, const char *prefix);

void ptrie_print(PrefixTrie *trie); // ordem lexicográfica

#endif
