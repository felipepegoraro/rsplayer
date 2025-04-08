#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>
#include "./headers/arena.h"
#include "./headers/id3.h"
#include "./headers/ui.h"
#include "./headers/timer.h"

// #include "./headers/levenshtein.h"
// #include "./headers/trie.h"
// #include "./headers/queue.h"
// #include "./headers/hashtable.h"
// #include "./headers/playlist.h"

#define DEFAULT_DIR "/home/felipe/Músicas/"
#define MAX_SONGS 100 

typedef struct AppStatus {
    int indexSong;
    int isPaused;
    RsMusic music; // atual
} AppStatus;

int agora = 0;

typedef struct AppContext {
    AppStatus *status;
    arena_t arenaSongs;
    // HashTable *songs
} AppContext;

typedef struct Commands {
    char s;
    void (*fn) (void*); // (AppContext*)
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

void cmd_play_pause(void *context){
    AppContext *ctx = (AppContext*)context;
    if (ctx){
        int *ip = &ctx->status->isPaused;
        if (*ip){ResumeMusicStream(*ctx->status->music.currentSong); *ip=0;}
        else {PauseMusicStream(*ctx->status->music.currentSong); *ip=1;}
    }
}

void cmd_seek_forward(void *context) {
    AppContext *ctx = (AppContext*)context;

    if (ctx && ctx->status && ctx->status->music.currentSong) {
        Music *song = ctx->status->music.currentSong;
        float current = GetMusicTimePlayed(*song);
        float total = GetMusicTimeLength(*song);
        float next = current + 10.0f;
        if (next > total) next = total;
        SeekMusicStream(*song, next);
        printf("+10s (%.2fs / %.2fs)\n", next, total);
    }
}


void cmd_seek_backward(void *context) {
    AppContext *ctx = (AppContext*)context;

    if (ctx && ctx->status && ctx->status->music.currentSong) {
        Music *song = ctx->status->music.currentSong;
        float current = GetMusicTimePlayed(*song);
        float next = current - 10.0f;
        if (next < 0.0f) next = 0.0f;
        SeekMusicStream(*song, next);
        printf("-10s (%.2fs)\n", next);
    }
}

void play_song(AppContext *ctx, const char *song_path) {
    if (!ctx || !song_path) return;

    FILE *f = id3_read_song_file(song_path);
    if (!f) {
        printf("erro inesperado\n");
        return;
    }

    // vazamento de memória resolvido
    // carrega música atual
    Music tempMusic = LoadMusicStream(song_path);
    if (!tempMusic.ctxData) {
        printf("Erro ao carregar música: %s\n", song_path);
        fclose(f);
        return;
    }

    // descarrega música anterior
    if (ctx->status->music.currentSong) {
        StopMusicStream(*ctx->status->music.currentSong);
        UnloadMusicStream(*ctx->status->music.currentSong);
    }

    // aloca na arena apenas o ponteiro para Music
    Music *musicPtr = arena_alloc(ctx->arenaSongs.arena, sizeof(Music));
    if (!musicPtr) {
        printf("Erro de alocação\n");
        UnloadMusicStream(tempMusic);
        fclose(f);
        return;
    }

    // move o Music real para a arena (sem sobrescrever depois)
    memcpy(musicPtr, &tempMusic, sizeof(Music));
    ctx->status->music.currentSong = musicPtr;

    while (!IsMusicReady(*musicPtr));
    PlayMusicStream(*musicPtr);

    ID3V2_Tags tg = id3_get_song_tags(f);
    fclose(f);

    ctx->status->music.currentSong = musicPtr;
    ctx->status->music.tags = tg;
    ctx->status->isPaused = 0;

    if (strlen(tg.title) > 0 && strlen(tg.artist) > 0 && strlen(tg.year) > 0){
        printf("Tocando: %s - %s (%s)\n", tg.title, tg.artist, tg.year);
    } else {
        printf("Tocando: %s\n", song_path);
    }
}


// TODO: playlist (prev/next deve verificar contexto)

void cmd_play_next_song(void *context) {
    AppContext *ctx = (AppContext*)context;

    if (ctx){
        ctx->status->indexSong = (ctx->status->indexSong + 1) % ctx->arenaSongs.total;
        const char *next = arena_get_by_index(&ctx->arenaSongs, ctx->status->indexSong);
        printf("next: %s\n", next);
        if (next) play_song(ctx, next);
        agora++;
    }
}

void cmd_play_prev_song(void *context) {
    AppContext *ctx = (AppContext*)context;

    if (ctx){
        ctx->status->indexSong = (ctx->status->indexSong - 1 + ctx->arenaSongs.total) % ctx->arenaSongs.total;
        const char *prev = arena_get_by_index(&ctx->arenaSongs, ctx->status->indexSong);
        if (prev) play_song(ctx, prev);
        agora--;
    }
}

// void cmd_search_song(void *ctx){
    // usar trie para autocomplete das musicas na busca
    // ao pegar uma musica (dps do autocomplete e apertar enter)
    // tocar a musica.
// }

int raylib_init(void){
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        printf("Erro ao inicializar o sistema de áudio da Raylib!\n");
        return EXIT_FAILURE;
    }

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, PLAYER_NAME);
    return EXIT_SUCCESS;
}

void ray_close(AppContext *ctx) {
    if (ctx && ctx->status && ctx->status->music.currentSong) {
        UnloadMusicStream(*ctx->status->music.currentSong);
        ctx->status->music.currentSong = NULL;
    }

    CloseAudioDevice();
    CloseWindow();
}


void *music_thread_fn(void *context) {
    AppContext *ctx = (AppContext*) context;

    while (!WindowShouldClose()) {
        if ((ctx->status->music.currentSong && IsMusicStreamPlaying(*ctx->status->music.currentSong)) || !ctx->status->isPaused) {
            if (IsMusicReady(*ctx->status->music.currentSong)){
                UpdateMusicStream(*ctx->status->music.currentSong);
            }

            if (!ctx->status->isPaused && ctx->status->music.currentSong) {
                float played = GetMusicTimePlayed(*ctx->status->music.currentSong);
                float total = GetMusicTimeLength(*ctx->status->music.currentSong);

                if ((total - played) < 0.1f) {
                    cmd_play_next_song(ctx);
                    printf("NEXT FUCKING SONG!\n");
                }
            }
        }

        usleep(10000);
    }

    printf("close!\n");
    return NULL;
}

void handle_user_input(AppContext *ctx, Commands *cmd, int cmd_count) {
    for (int i = 0; i < cmd_count; i++) {
        if (IsKeyPressed(cmd[i].s)) {
            cmd[i].fn(ctx);
            break;
        }
    }
}

Button buttons[] = {
    { .bounds = (Rectangle){160, 50, 100, 40}, .color = RED, .label = "Pause", .onClick = cmd_play_pause },
    { .bounds = (Rectangle){270, 50, 100, 40}, .color = DARKBLUE, .label = "Next", .onClick = cmd_play_next_song },
    { .bounds = (Rectangle){50, 50, 100, 40}, .color = DARKBLUE, .label = "Prev", .onClick = cmd_play_prev_song }
};

int main(void)
{
    if (raylib_init()!=0) return 1;

    Arena *a = arena_create(ARENA_BLOCK_SIZE);
    arena_t arena = {0, a};

    read_dir_files(&arena, MAX_SONGS);

    Commands cmd[] = {
        {KEY_P, cmd_play_pause},
        {KEY_F, cmd_seek_forward},
        {KEY_R, cmd_seek_backward},
        {KEY_N, cmd_play_next_song},
        {KEY_B, cmd_play_prev_song}
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

    pthread_t music_thread;
    pthread_create(&music_thread, NULL, music_thread_fn, &ctx);

    Timer timer = {0};

    while (!WindowShouldClose()){
        handle_user_input(&ctx, cmd, sizeof(cmd) / sizeof(cmd[0]));

        updateTimer(
            &timer,
            GetMusicTimePlayed(*ctx.status->music.currentSong),
            GetMusicTimeLength(*ctx.status->music.currentSong)
        );

        BeginDrawing();
            ClearBackground(BLACK);

            DrawText(
                strlen(ctx.status->music.tags.title) > 0 
                ? ctx.status->music.tags.title
                : arena_get_by_index(&ctx.arenaSongs, agora),
                10, 10, 20, RAYWHITE
            );

            drawTimer(
                &timer,
                ctx.status->music.currentSong,
                (Rectangle){20, 230, 400, 20}
            );

            for (int i=0; i<3; i++){
                m_DrawButton(buttons[i]);
                if (m_CallbackButtonClicked(buttons[i])) buttons[i].onClick(&ctx);
            } 

        EndDrawing();

        buttons[0].label = ctx.status->isPaused ? "Play" : "Pause";
    }

    ray_close(&ctx);
    pthread_join(music_thread, NULL);
    arena_free(ctx.arenaSongs.arena);

    return EXIT_SUCCESS;
}
