#include "../headers/trie.h"
#include <string.h>
#include <stdio.h>

//primeira vez impleemntando uma prefix trie 
//talvez seja usada na busca por musicas ou nao,
//pelo menos ta pronto
//corrigir vazamento

TrieNode *trie_node_new(Arena *arena){
    TrieNode *node = (TrieNode*) arena_alloc(arena, sizeof(TrieNode));

    if (node){
        memset(node->children, 0, sizeof(node->children));
        node->is_terminal = 0;
        node->value = '\0';
    }

    return node;
}

PrefixTrie ptrie_new(void){
    PrefixTrie trie;
    trie.arena = arena_create(INITIAL_TRIE_ARENA_SIZE);

    if (!trie.arena){
        trie.root = NULL;
        return trie;
    }
    
    trie.root = trie_node_new(trie.arena);
    if (!trie.root){
        arena_free(trie.arena);
        trie.arena = NULL;
    }

    return trie;
}

int get_index(char ch) {
    if (ch >= 'a' && ch <= 'z') return ch - 'a';
    else if (ch >= '0' && ch <= '9') return ch - '0' + 26;
    else if (ch == '.') return 36;
    else if (ch == '-') return 37;
    else if (ch == '_') return 0x5f;
    return -1; //inválido
}


void ptrie_insert(PrefixTrie *trie, const char *word){
    TrieNode *current = trie->root;

    for (const char *ch = word; *ch; ch++){
        int index = get_index(*ch);

        if (index < 0 || index >= ALPHABET_SIZE) {
            continue;
        }

        if (!current->children[index])
            current->children[index] = trie_node_new(trie->arena);

        current = current->children[index];
        current->value = *ch;
    }

    current->is_terminal = 1;
}

void ptrie_free(PrefixTrie *trie) {
    if (trie) arena_free(trie->arena);
}

void helper_ptrie_collect();
char **ptrie_search(PrefixTrie *trie, const char *prefix);

int ptrie_starts_with(PrefixTrie *trie, const char *prefix){
    if (!trie || !trie->root) return 1;

    TrieNode *current = trie->root;
    for (const char *ch = prefix; *ch; ch++){
        int index = get_index(*ch);
        if (index < 0 || index >= ALPHABET_SIZE) return 1;
        if (!current->children[index]) return 1;
        current = current->children[index];
    }

    return 0;
}

void helper_ptrie_print(TrieNode*node, char *prefix, int depth){
    if (node->is_terminal){
        prefix[depth] = '\0';
        printf("%s\n", prefix);
    }

    for (int i=0;i<ALPHABET_SIZE; i++){
        if (node->children[i]){
            prefix[depth] = node->children[i]->value;
            helper_ptrie_print(node->children[i], prefix, depth+1);
        }
    }
}

void ptrie_print(PrefixTrie *trie){
    char prefix[MAX_WORD_SIZE];
    helper_ptrie_print(trie->root, prefix, 0);
}

int helper_no_children(TrieNode *node) {
    for (int i = 0; i < 26; i++) {
        if (node->children[i] != NULL) 
            return 0;
    }
    return 1;
}

int helper_ptrie_delete(TrieNode *current, const char *word, size_t depth){
    //1 é erro nesse caso
    if (!current) return 1;
    if (depth == strlen(word)){
        if (current->is_terminal){
            current->is_terminal = 0;
            return helper_no_children(current) ? 0 : 1;
        }
        return 1;
    }

    int index = get_index(word[depth]);

    if (helper_ptrie_delete(current->children[index], word, depth+1) == 0){
        current->children[index] = NULL;
        return (!current->is_terminal && helper_no_children(current)) ? 0 : 1;
    }

    return 1;
}

int ptrie_delete(PrefixTrie *trie, const char *word){
    if (!trie || !trie->root) return 1;
    return helper_ptrie_delete(trie->root, word, 0) ? 0 : 1;
}
