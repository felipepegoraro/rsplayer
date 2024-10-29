#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <limits.h>
#include "./headers/arena.h"
#include "./headers/id3.h"
// #include "./headers/levenshtein.h"
// #include "./headers/trie.h"
// #include "./headers/queue.h"
#include "./headers/hashtable.h"

#define DEFAULT_DIR "/home/felipe/Músicas/"
#define MAX_SONG 100

typedef struct AppStatus {
    int indexSong;
    int isPaused;
    Music music; // atual
} AppStatus;

typedef struct AppContext {
    AppStatus *status;
    arena_t arenaSongs;
    // HashTable *songs
} AppContext;

typedef struct Commands {
    char s;
    void (*fn) (AppContext*);
} Commands;

void read_dir_files(arena_t *arena, int max_songs)
{
    DIR *d;
    struct dirent *dir;
    d = opendir(DEFAULT_DIR);
    if (d){
        int i=0;
        while ((dir = readdir(d)) != NULL && i < max_songs) {
            if (strstr(dir->d_name, ".mp3") != NULL){
                size_t path = strlen(DEFAULT_DIR) + strlen(dir->d_name) + 1;
                char *song_path = arena_alloc(arena->arena, path);
                if (song_path){
                    strcpy(song_path, DEFAULT_DIR);
                    strcat(song_path, dir->d_name);
                    i++;
                    arena->total++;
                }
            }
        }
        closedir(d);
    }
}

void cmd_play_pause(AppContext *ctx){
    if (ctx){
        int *ip = &ctx->status->isPaused;
        if (*ip){Mix_ResumeMusic(); *ip=0;}
        else {Mix_PauseMusic(); *ip=1;}
    }
}

void cmd_seek_forward(AppContext *ctx){
    if (ctx){
        double currentPos = Mix_GetMusicPosition(ctx->status->music.currentSong);
        Mix_SetMusicPosition(currentPos + 10);
        printf("+10s\n");
    }
}

void cmd_seek_backward(AppContext *ctx){
    if (ctx){
        double currentPos = Mix_GetMusicPosition(ctx->status->music.currentSong);
        Mix_SetMusicPosition(currentPos - 10);
        printf("-10s\n");
    }
}

void play_song(AppContext *ctx, const char *song_path) {
    if (!ctx || !song_path) return;

    FILE *f = id3_read_song_file(song_path);
    if (!f) {
        printf("erro inesperado\n");
        return;
    }

    if (ctx->status->music.currentSong) {
        Mix_FreeMusic(ctx->status->music.currentSong);
    }
    
    ctx->status->music.currentSong = Mix_LoadMUS(song_path);
    if (!ctx->status->music.currentSong) {
        printf("Falha ao carregar a música! Erro SDL_mixer: %s\n", Mix_GetError());
        return;
    }

    ctx->status->isPaused = 0;

    Mix_PlayMusic(ctx->status->music.currentSong, 1);

    ID3V2_Tags tg =  id3_get_song_tags(f);

    if (strlen(tg.title)>0 && strlen(tg.artist)>0 && strlen(tg.year)>0){
        ctx->status->music.tags = tg;
        printf("tocando: %s - %s (%s)\n", tg.title, tg.artist, tg.year);
    } else {
        printf("tocando: %s\n", song_path);
    }
}


// TODO: playlist (prev/next deve verificar contexto)

void cmd_play_next_song(AppContext *ctx) {
    if (ctx){
        ctx->status->indexSong = (ctx->status->indexSong + 1) % ctx->arenaSongs.total;
        const char *next = arena_get_by_index(&ctx->arenaSongs, ctx->status->indexSong);
        if (next) play_song(ctx, next);
    }
}

void cmd_play_prev_song(AppContext *ctx) {
    if (ctx){
        ctx->status->indexSong = (ctx->status->indexSong - 1 + ctx->arenaSongs.total) % ctx->arenaSongs.total;
        const char *prev = arena_get_by_index(&ctx->arenaSongs, ctx->status->indexSong);
        if (prev) play_song(ctx, prev);
    }
}

// void cmd_search_song(AppContext *ctx){
    // usar trie para autocomplete das musicas na busca
    // ao pegar uma musica (dps do autocomplete e apertar enter)
    // tocar a musica.
// }

int sdl_init(void){
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0) {
        printf("SDL não pôde ser inicializado! Erro SDL: %s\n", SDL_GetError());
        return 1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer não pôde ser inicializado! Erro SDL_mixer: %s\n", Mix_GetError());
        return 1;
    }

    return 0;
}

void sdl_close(AppContext *ctx){
    Mix_FreeMusic(ctx->status->music.currentSong);
    Mix_CloseAudio();
    SDL_Quit();
}

void handle_user_input(AppContext *ctx, Commands *cmd, int cmd_count) {
    char s;

    while (Mix_PlayingMusic() || !ctx->status->isPaused){
        SDL_Delay(100);

        if (!Mix_PlayingMusic()) cmd_play_next_song(ctx);
        if (scanf(" %c", &s) != 1) continue;

        for (int i = 0; i < cmd_count; i++) {
            if (s == cmd[i].s) {
                cmd[i].fn(ctx);
                break;
            }
        }

        if (s == 'q') break;
    }
}

int main(void)
{
    //
    // Music ms = {
    //     .tags = {
    //         .version = 0,
    //         .size = 0,
    //         .year = "2020",
    //         .album = "album",
    //         .genre = "genre",
    //         .title = "title",
    //         .artist = "artist"
    //     },
    //     .currentSong = NULL
    // };
    //
    // Music ms1 = {
    //     .tags = {
    //         .version = 0,
    //         .size = 0,
    //     },
    //     .currentSong = NULL
    // };
    //
    // Music mss = {
    //     .tags = {
    //         .version = 0,
    //         .size = 0,
    //         .year = "2020",
    //         .album = "xxxxx",
    //         .genre = "xxxx",
    //         .title = "xxxx",
    //         .artist = "xxxxx"
    //     },
    //     .currentSong = NULL
    // };
    //
    // HashTable hs = hs_create();
    // hs_insert(&hs, &ms, "ok");
    // hs_insert(&hs, &ms1, "musica-null");
    // hs_insert(&hs, &mss, "ok");
    // (void)hs_search(&hs, 2867999238);
    // (void)hs_search(&hs, 612476476);
    // hs_free(&hs);
    // return 0 ;

    if (sdl_init()!=0) return 1;

    Arena *a = arena_create(ARENA_BLOCK_SIZE);
    arena_t arena = {0, a};

    read_dir_files(&arena, MAX_SONG);

    Commands cmd[] = {
        {'p', cmd_play_pause},
        {'f', cmd_seek_forward},
        {'r', cmd_seek_backward},
        {'n', cmd_play_next_song},
        {'b', cmd_play_prev_song}
        // {'s', cmd_search_song}
    };

    AppStatus status = {
        .indexSong = 0,
        .isPaused = 0,
        .music = {
            .tags = {},
            .currentSong = NULL
        }
    };

    AppContext ctx = {
        .arenaSongs = arena,
        .status = &status
    };

    // TODO: fuzzy finder para selecionar mppusica (Levenshtein Distance ou Trie)
    const char *song = arena_get_by_index(&ctx.arenaSongs, 0);
    play_song(&ctx, song);

    handle_user_input(&ctx, cmd, sizeof(cmd) / sizeof(cmd[0]));

    sdl_close(&ctx);
    arena_free(ctx.arenaSongs.arena);
    return 0;
}
