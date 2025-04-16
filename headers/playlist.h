#ifndef PLAYLIST_H
#define PLAYLIST_H

#include "./queue.h"
#include <string.h>

typedef struct playlist {
    Queue *queue;
    size_t current_index;
    char name[44];
    int is_real_q; // 1: fila | 0: playlist
} Playlist;

Playlist playlist_init(const char *name, int is_q, RsMusic *music);
RsMusic *playlist_next(Playlist *p);
RsMusic *playlist_prev(Playlist *p);
void playlist_add(Playlist *p, RsMusic *new_song);
void playlist_clean(Playlist *p);
void playlist_free(Playlist *p);

#endif
