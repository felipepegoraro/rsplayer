#include <stdio.h>
#include <string.h>
#include "../headers/queue.h"

Node *q_node_new(Arena *arena){
    Node *n = (Node*) arena_alloc(arena, sizeof(Node));
    if (n){
        n->next = NULL;
        n->prev = NULL;
    }
    
    return n;
}

Queue q_create(Music *firstSong){
    Queue q;
    q.arena = arena_create(ARENA_BLOCK_SIZE);
    if (!q.arena) {
        q.root = NULL;
        q.tail = NULL;
        return q;
    };
    
    q.root = q_node_new(q.arena);
    if (!q.root){
        arena_free(q.arena);
        q.arena = NULL;
        return q;
    }

    q.root->music = firstSong;
    q.tail = q.root;
    q.len = 1;

    return q;
}

void q_enqueue(Queue *queue, Music *song){
    if (!queue || !queue->arena) return;
     
    Node *newNode = q_node_new(queue->arena);
    if (!newNode) return;

    newNode->music = song;

    if (queue->len == 0){
        queue->root = newNode;
        queue->tail = newNode;
    } else {
        newNode->prev = queue->tail;
        queue->tail->next = newNode;
        queue->tail = newNode;
    }

    queue->len++;
}

Music *q_dequeue(Queue *queue){
    if (!queue || !queue->arena || queue->len == 0) 
        return NULL;

    Music *song = queue->root->music;
    printf("removendo musica: %s\n", song->tags.title);

    if (queue->len == 1){
        free(queue->tail);
        free(queue->root);
        queue->tail = NULL;
        queue->root = NULL;
    } else {
        queue->root = queue->root->next;
        if (queue->root) queue->root->prev = NULL;
    }

    queue->len--;
    return song;
}

Music *q_peek(Queue *queue) {
    return (queue && queue->root)
        ? queue->root->music
        : NULL;
}


void q_free(Queue *queue){
    if (queue)
        arena_free(queue->arena);

    queue = NULL;
}
