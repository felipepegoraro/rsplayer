#include "../headers/playlist.h"

Playlist playlist_init(const char *name, int is_q, RsMusic *music){
    Playlist p;
    p.queue = malloc(sizeof(Queue));
    *p.queue = q_create(music);

    p.current_index = 0;
    p.is_real_q = is_q; 
    strcpy(p.name, name);

    return p;
}

RsMusic *playlist_next(Playlist *p){
    if (!p || !p->queue || p->current_index >= p->queue->len) {
        return NULL;
    }

    Node *current_song = p->queue->root;

    // atual
    size_t i = 0;
    while (i < p->current_index && current_song) {
        current_song = current_song->next;
        i++;
    }

    // proixmo
    if (current_song && current_song->next) {
        Node *next_song = current_song->next;

        if (p->is_real_q == 1) { // fila, remove a atual
            q_dequeue(p->queue);
            p->current_index = (p->current_index > 0) ? p->current_index - 1 : 0;
        } else {
            p->current_index++;
        }

        return next_song->music;
    }

    return NULL;
}

RsMusic *playlist_prev(Playlist *p) {
    if (!p || !p->queue || p->current_index == 0) {
        return NULL;
    }

    Node *current_song = p->queue->root;
    
    size_t i = 0;
    while (i < p->current_index && current_song) {
        current_song = current_song->next;
        i++;
    }

    if (current_song && current_song->prev) {
        Node *prev_song = current_song->prev;

        if (p->is_real_q == 1) {
            q_dequeue(p->queue);
            p->current_index--;
        }

        p->current_index--;
        return prev_song->music;
    }

    return NULL;
}

void playlist_add(Playlist *p, RsMusic *new_song){
    if (!p || !new_song) return;

    printf("q_enqueue...\n");
    q_enqueue(p->queue, new_song);
}

void playlist_clean(Playlist *p) {
    if (!p || !p->queue) return;

    while (p->queue->len > 0)
        q_dequeue(p->queue);

    p->current_index = 0;
}


void playlist_free(Playlist *p) {
    if (!p) return;

    if (p->queue) {
        q_free(p->queue);
        free(p->queue);
        p->queue = NULL;
    }
}

