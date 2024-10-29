#ifndef QUEUE_H
#define QUEUE_H

#include "../headers/arena.h"
#include <stdio.h>

typedef struct node {
    struct node *prev;
    struct node *next;
    char songName[256];
} Node;

typedef struct {
    Node *root;
    Node *tail;
    Arena *arena;
    size_t len;
} Queue;

Node *q_node_new(Arena *arena);
Queue q_create(char *firstSong);
void q_enqueue(Queue *queue, const char *songName);
const char *q_dequeue(Queue *queue);
const char *q_peek(Queue *queue);
void q_free(Queue *queue);

#endif
