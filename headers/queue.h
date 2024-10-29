#ifndef QUEUE_H
#define QUEUE_H

#include "../headers/arena.h"
#include "id3.h"
#include <stdio.h>

typedef struct node {
    struct node *prev;
    struct node *next;
    Music *music;
} Node;

typedef struct {
    Node *root;
    Node *tail;
    Arena *arena;
    size_t len;
} Queue;

Node *q_node_new(Arena *arena);
Queue q_create(Music *firstSong);
void q_enqueue(Queue *queue, Music *song);
Music *q_dequeue(Queue *queue);
Music *q_peek(Queue *queue);
void q_free(Queue *queue);

#endif
