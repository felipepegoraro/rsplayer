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

#define DEFAULT_DIR "/home/felipe/Músicas/"
#define MAX_SONG 100

typedef struct AppStatus {
    int indexSong;
    int isPaused;
    Mix_Music *currentSong;
} AppStatus;

typedef struct AppContext {
    AppStatus *status;
    arena_t arenaSongs;
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
    int *ip = &ctx->status->isPaused;
    if (*ip){Mix_ResumeMusic(); *ip=0;}
    else {Mix_PauseMusic(); *ip=1;}
}

void cmd_seek_forward(AppContext *ctx){
    double currentPos = Mix_GetMusicPosition(ctx->status->currentSong);
    Mix_SetMusicPosition(currentPos + 10);
    printf("+10s\n");
}

void cmd_seek_backward(AppContext *ctx){
    double currentPos = Mix_GetMusicPosition(ctx->status->currentSong);
    Mix_SetMusicPosition(currentPos - 10);
    printf("-10s\n");
}

void play_song(AppContext *ctx, const char *song_path) {
    if (ctx->status->currentSong) {
        Mix_FreeMusic(ctx->status->currentSong);
    }
    
    ctx->status->currentSong = Mix_LoadMUS(song_path);
    if (!ctx->status->currentSong) {
        printf("Falha ao carregar a música! Erro SDL_mixer: %s\n", Mix_GetError());
        return;
    }

    ctx->status->isPaused = 0;
    printf("tocando: %s\n", song_path);
    Mix_PlayMusic(ctx->status->currentSong, 1);
}


void cmd_play_next_song(AppContext *ctx) {
    ctx->status->indexSong = (ctx->status->indexSong + 1) % ctx->arenaSongs.total;
    const char *next = arena_get_by_index(&ctx->arenaSongs, ctx->status->indexSong);
    if (next) play_song(ctx, next);
}

void cmd_play_prev_song(AppContext *ctx) {
    ctx->status->indexSong = (ctx->status->indexSong - 1 + ctx->arenaSongs.total) % ctx->arenaSongs.total;
    const char *prev = arena_get_by_index(&ctx->arenaSongs, ctx->status->indexSong);
    if (prev) play_song(ctx, prev);
}

int sdl_init(void){
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
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
    Mix_FreeMusic(ctx->status->currentSong);
    Mix_CloseAudio();
    SDL_Quit();
    arena_free(ctx->arenaSongs.arena);
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
        .currentSong = NULL
    };

    AppContext ctx = {
        .arenaSongs = arena,
        .status = &status
    };

    // TODO: fuzzy finder para selecionar mppusica (Levenshtein Distance ou Trie)
    const char *song = arena_get_by_index(&ctx.arenaSongs, 0);
    play_song(&ctx, song);
    FILE *f = id3_read_song_file(song);
    ID3V2_Tags tg;
    if (f) {
        tg = id3_get_song_tags(f);
        fclose(f);
        
        printf("Título: %s\n", tg.tags.title);
        printf("Artista: %s\n", tg.tags.artist);
        printf("Álbum: %s\n", tg.tags.album);
        printf("Ano: %s\n", tg.tags.year);
    }

    handle_user_input(&ctx, cmd, sizeof(cmd) / sizeof(cmd[0]));

    sdl_close(&ctx);
    return 0;
}
