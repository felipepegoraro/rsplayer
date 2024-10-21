#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include "./headers/arena.h"

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
                    printf("total: %zu\n", arena->total);
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

    printf("música %s\n", *ip ? "pausada" : "tocando");
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

void cmd_play_next_song(AppContext *ctx){
    Mix_FreeMusic(ctx->status->currentSong);

    if ((ctx->status->indexSong + 1) >= ctx->arenaSongs.total) {
        printf("nao há mais músicas, reinicia playlist\n");
        ctx->status->indexSong = 0;
    } else ctx->status->indexSong++;

    const char *nt = arena_get_by_index(&ctx->arenaSongs, ctx->status->indexSong);

    if (nt){
        ctx->status->currentSong = Mix_LoadMUS(nt);
        if (!ctx->status->currentSong)
            printf("Falha ao carregar a música! Erro SDL_mixer: %s\n", Mix_GetError());

        Mix_PlayMusic(ctx->status->currentSong, 1);
        printf("tocando nova musica: %s\n", nt);
    }
}

int main(int argc, char* argv[])
{
    Arena *a = arena_create(ARENA_BLOCK_SIZE);
    arena_t arena = {0, a};


    printf(" ===== LENDO ARQUIVOS ===== \n");
    read_dir_files(&arena, MAX_SONG);

    Commands cmd[] = {
        {'p', cmd_play_pause},
        {'f', cmd_seek_forward},
        {'r', cmd_seek_backward},
        {'n', cmd_play_next_song}
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

    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf("SDL não pôde ser inicializado! Erro SDL: %s\n", SDL_GetError());
        return 1;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer não pôde ser inicializado! Erro SDL_mixer: %s\n", Mix_GetError());
        return 1;
    }

    const char *song = arena_get_by_index(&ctx.arenaSongs, 0);
    status.currentSong = Mix_LoadMUS(song);
    if (!status.currentSong) {
        printf("Failed to load music! SDL_mixer Error: %s\n", Mix_GetError());
        return 1;
    }

    printf("\n ===== TOCANDO ===== \n\t(%s)\n", song);
    Mix_PlayMusic(status.currentSong, 1);

    char s;

    while (Mix_PlayingMusic() && s != 'q') {
        SDL_Delay(100);
        s = getchar();

        for (int i = 0; i < sizeof(cmd) / sizeof(cmd[0]); i++) {
            if (s == cmd[i].s) {
                cmd[i].fn(&ctx);
                break;
            }
        }
    }

    printf("música finalizada.\n");

    Mix_FreeMusic(status.currentSong);
    Mix_CloseAudio();
    SDL_Quit();
    arena_free(arena.arena);
    return 0;
}

