#ifndef ID3_TAGS_H
#define ID3_TAGS_H

// para usar as tags ID3:
//    `id3v2 --artist "artist-name" filename`
//    `id3v2 --album "album-name" filename`
// etc. (veja todos em `id3v2 --help`)

#define T_ID3V2_MAX 128

#include <stdint.h>
#include <stdio.h>

// typedef struct Tags {
// } Tags;

typedef struct ID3V2_Tags {
    uint8_t size;
    uint16_t version;

    char title[T_ID3V2_MAX];    // TIT2 (Título)
    char artist[T_ID3V2_MAX];   // TPE1 (Artista)
    char album[T_ID3V2_MAX];    // TALB (Álbum)
    char year[5];               // TYER (Ano)
    char genre[T_ID3V2_MAX];    // TCON (Gênero)
} ID3V2_Tags;

#include "raylib.h"
typedef struct {
    ID3V2_Tags tags;
    Music *currentSong;
} RsMusic;

// 10 bytes + data
typedef struct {
    // 4 pro id + 1 '\0' (frame header TIT2, TPE1, etc)
    char frame_id[5];
    uint32_t frame_size;
    char *frame_data;
} ID3Frame;

typedef struct {
    ID3Frame *frames;
    size_t count; 
} ID3FrameCollection;

FILE *id3_read_song_file(const char *filename);
int id3_file(FILE *fp, ID3V2_Tags *tags);
ID3FrameCollection *id3_read_id3v2_frames(FILE *fp, uint32_t tag_size);
void id3_free_id3_frame_collection(ID3FrameCollection *collection);
void id3_populate_tags_from_frames(ID3FrameCollection *collection, ID3V2_Tags *tags);
ID3V2_Tags id3_get_song_tags(FILE *fp);

#endif // ID3_TAGS_H
