#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../headers/id3.h"

FILE *id3_read_song_file(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        printf("nao foi possivel abrir o arquivo.\n");
        return NULL;
    }

    return fp;
}

int id3_file(FILE *fp, ID3V2_Tags *tags) {
    if (!fp) return 1; //erro

    char buffer[10];
    size_t sread = fread(buffer, sizeof(char), 10, fp);
    if (sread < 10)
        if (feof(fp) || ferror(fp)) {
            printf("erro ao ler arquivo\n");
            return 1;
        };

    if (buffer[0] != 'I' || buffer[1] != 'D' || buffer[2] != '3') {
        printf("nao é arquivo id3v2\n");
        return 1;
    }

    tags->version = (buffer[3] << 8) | buffer[4];
    tags->size =   ((uint8_t) (buffer[6] & 0x7f) << 21) | //0b01111111
                   ((uint8_t) (buffer[7] & 0x7f) << 14) |
                   ((uint8_t) (buffer[8] & 0x7f) <<  7) |
                   ((uint8_t) (buffer[9] & 0x7f));

    return 0;
}

ID3FrameCollection *id3_read_id3v2_frames(FILE *fp, uint32_t tag_size) {
    if (!fp) return NULL;

    ID3FrameCollection *collection = malloc(sizeof(ID3FrameCollection));
    if (!collection) return NULL;

    collection->frames = malloc(sizeof(ID3Frame) * 256);
    if (!collection->frames) {
        free(collection);
        return NULL;
    }

    collection->count = 0;

    size_t bytes_read = 0;

    while (bytes_read < tag_size) {
        char frame_header[10];
        size_t sread = fread(frame_header, sizeof(char), 10, fp);
        bytes_read += sread;

        if (sread < 10) break;

        ID3Frame *current_frame = &collection->frames[collection->count];

        strncpy(current_frame->frame_id, frame_header, 4);
        current_frame->frame_id[4] = '\0';

        current_frame->frame_size = ((uint32_t)(frame_header[4]) << 24) |
                                    ((uint32_t)(frame_header[5]) << 16) |
                                    ((uint32_t)(frame_header[6]) << 8)  |
                                     (uint32_t)(frame_header[7]);

        current_frame->frame_data = malloc(current_frame->frame_size + 1);
        if (!current_frame->frame_data) {
            for (size_t i = 0; i < collection->count; i++)
                free(collection->frames[i].frame_data);
            free(collection->frames);
            free(collection);
            return NULL;
        }

        fread(current_frame->frame_data, sizeof(char), current_frame->frame_size, fp);
        collection->count++;
    }

    return collection;
}

void id3_free_id3_frame_collection(ID3FrameCollection *collection) {
    if (collection) {
        for (size_t i = 0; i < collection->count; i++)
            free(collection->frames[i].frame_data);
        free(collection->frames);
        free(collection);
    }
}

void id3_populate_tags_from_frames(ID3FrameCollection *collection, ID3V2_Tags *tags) {
    if (!collection || !tags) return;

    for (size_t i = 0; i < collection->count; i++) {
        ID3Frame *frame = &collection->frames[i];
        if (strcmp(frame->frame_id, "TIT2") == 0) {
            strcpy(tags->tags.title, frame->frame_data+1);
            tags->tags.title[frame->frame_size-1] = '\0';
        }
        else if (strcmp(frame->frame_id, "TPE1") == 0) {
            strcpy(tags->tags.artist, frame->frame_data + 1);
            tags->tags.artist[frame->frame_size-1] = '\0';
        }
        else if (strcmp(frame->frame_id, "TALB") == 0) {
            strcpy(tags->tags.album, frame->frame_data + 1);
            tags->tags.album[frame->frame_size-1] = '\0';
        }
        else if (strcmp(frame->frame_id, "TYER") == 0) {
            strncpy(tags->tags.year, frame->frame_data + 1, 4);
            tags->tags.year[4] = '\0';
        }
    }
}

ID3V2_Tags id3_get_song_tags(FILE *fp) {
    if (!fp) return (ID3V2_Tags){};

    ID3V2_Tags tg = {};

    if (id3_file(fp, &tg) == 0) {
        // uint8_t major = (tg.version >> 8) & 0xff;
        // uint8_t minor = tg.version & 0xff;

        // printf("version: %u.%u\n", major, minor);
        // printf("size...: %u\n", tg.size);

        ID3FrameCollection *collection = id3_read_id3v2_frames(fp, tg.size);
        if (collection) {
            id3_populate_tags_from_frames(collection, &tg);
            id3_free_id3_frame_collection(collection);
        }
    } else {
        printf("nao é id3\n");
    }

    return tg;
}

/*
    // exemplo de uso:
    
    int main(int argc, char **argv) {
        FILE *fp = id3_read_song_file(argv[1]);
        ID3V2_Tags tg;

        if (fp) {
            tg = id3_get_song_tags(fp);
            fclose(fp);
            
            printf("Título: %s\n", tg.tags.title);
            printf("Artista: %s\n", tg.tags.artist);
            printf("Álbum: %s\n", tg.tags.album);
            printf("Ano: %s\n", tg.tags.year);
        }

        return 0;
    }
*/
