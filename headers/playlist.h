#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "./queue.h"
#include <string.h>

typedef struct playlist {
    Queue *queue;
    char name[64];
    size_t current_index;
    int is_real_q; // 1: fila | 0: playlist
} Playlist;

Playlist playlist_init(const char *name, int is_q, Music *music);
Music *playlist_next(Playlist *p);
Music *playlist_prev(Playlist *p);
void playlist_add(Playlist *p, Music *new_song);
void playlist_clean(Playlist *p);
void playlist_free(Playlist *p);

#endif
