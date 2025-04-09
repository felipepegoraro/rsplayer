#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <math.h>
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

typedef struct PlayerState {
    int indexSong;
    int isPaused;
    int isMuted;
    float volume;
} PlayerState;

typedef struct AppStatus {
    PlayerState player;
    Tooltip tooltip;
    RsMusic music; // atual
} AppStatus;

typedef struct AppContext {
    AppStatus *status;
    arena_t arenaSongs;
    // HashTable *songs
} AppContext;

typedef enum {
    KEY_TOGGLE_PLAY_PAUSE = KEY_P,
    KEY_SEEK_FORWARD      = KEY_F,
    KEY_SEEK_BACKWARD     = KEY_B,
    KEY_PLAY_NEXT         = KEY_PERIOD, // '.'
    KEY_PLAY_PREVIOUS     = KEY_COMMA,  // ','
    KEY_MUTE_VOLUME       = KEY_M
} KeyCommand;

typedef struct Commands {
    KeyCommand key;
    void (*fn) (void*); // (AppContext*)
} Commands;

void m_readFileToArena(arena_t *arena, int max_songs)
{
    DIR *d;
    struct dirent *dir;
    d = opendir(DEFAULT_DIR);
    if (!d) return;

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

void m_cmdPlayPause(void *context){
    AppContext *ctx = (AppContext*)context;
    if (!ctx) return;

    int *ip = &ctx->status->player.isPaused;
    if (*ip){ResumeMusicStream(*ctx->status->music.currentSong); *ip=0;}
    else {PauseMusicStream(*ctx->status->music.currentSong); *ip=1;}
}

void m_cmdSeekForward(void *context) {
    AppContext *ctx = (AppContext*)context;

    if (ctx && ctx->status && ctx->status->music.currentSong) {
        Music *song = ctx->status->music.currentSong;

        float current = GetMusicTimePlayed(*song);
        float total = GetMusicTimeLength(*song);

        if (!((!isnan(current) && current >= 0.0f) || 
              (!isnan(total) && total >= 0.0f))
        ){
            ctx->status->tooltip.message = "Erro ao carregar.";
            ctx->status->tooltip.visible = true;
            ctx->status->tooltip.color = ERROR_COLOR;
            return;
        }

        float next = current + 10.0f;
        if (next > total) next = total;
        SeekMusicStream(*song, next);

        ctx->status->tooltip.message = "+10s";
        ctx->status->tooltip.visible = true;
        ctx->status->tooltip.color = TIP_COLOR;
    }
}


void m_cmdSeekBackward(void *context) {
    AppContext *ctx = (AppContext*)context;

    if (ctx && ctx->status && ctx->status->music.currentSong) {
        Music *song = ctx->status->music.currentSong;

        float next = GetMusicTimePlayed(*song) - 10.0f;
        if (!(!isnan(next) && next >= 0.0f)){
            ctx->status->tooltip.message = "Erro ao carregar.";
            ctx->status->tooltip.visible = true;
            ctx->status->tooltip.color = ERROR_COLOR;
            return;
        }

        if (next < 0.0f) next = 0.0f;
        SeekMusicStream(*song, next);

        ctx->status->tooltip.message = "-10s";
        ctx->status->tooltip.visible = true;
        ctx->status->tooltip.color = TIP_COLOR;
    }
}

void m_playSong(AppContext *ctx, const char *song_path) {
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

    SetMusicVolume(*musicPtr, ctx->status->player.isMuted 
        ? 0.0f 
        : ctx->status->player.volume
    );

    ID3V2_Tags tg = id3_get_song_tags(f);
    fclose(f);

    ctx->status->music.currentSong = musicPtr;
    ctx->status->music.tags = tg;
    ctx->status->player.isPaused = 0;

    if (strlen(tg.title) > 0 && strlen(tg.artist) > 0 && strlen(tg.year) > 0){
        printf("Tocando: %s - %s (%s)\n", tg.title, tg.artist, tg.year);
    } else {
        printf("Tocando: %s\n", song_path);
    }
}


// TODO: playlist (prev/next deve verificar contexto)

void m_cmdPlayNextSong(void *context) {
    AppContext *ctx = (AppContext*)context;
    if (!ctx) return;

    PlayerState *p = &ctx->status->player;
    p->indexSong = (p->indexSong + 1) % ctx->arenaSongs.total;

    const char *next = arena_get_by_index(&ctx->arenaSongs, p->indexSong);
    if (next) m_playSong(ctx, next);
}

void m_cmdPlayPrevSong(void *context) {
    AppContext *ctx = (AppContext*)context;
    if (!ctx) return;

    PlayerState *p = &ctx->status->player;
    p->indexSong = (p->indexSong - 1 + ctx->arenaSongs.total) % ctx->arenaSongs.total;

    const char *prev = arena_get_by_index(&ctx->arenaSongs, p->indexSong);
    if (prev) m_playSong(ctx, prev);
    p->indexSong--;
}

void m_cmdMuteVolume(void *context){
    AppContext *ctx = (AppContext*)context;
    if (!ctx) return;

    Music *m = ctx->status->music.currentSong;
    Tooltip *t = &ctx->status->tooltip;
    PlayerState *p = &ctx->status->player;

    while (!IsMusicReady(*m));

    if (p->isMuted){
        SetMusicVolume(*m, p->volume);
        p->isMuted = 0;
    } else {
        SetMusicVolume(*m, 0.0f);
        p->isMuted = 1;
    }

    t->message = p->isMuted ? "Volume mutado." : "Volume desmutado.";
    t->color   = TIP_COLOR;
    t->visible = true;
}

// void cmd_search_song(void *ctx){
    // usar trie para autocomplete das musicas na busca
    // ao pegar uma musica (dps do autocomplete e apertar enter)
    // tocar a musica.
// }

int m_RaylibInit(void){
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        printf("Erro ao inicializar o sistema de áudio da Raylib!\n");
        return EXIT_FAILURE;
    }

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, PLAYER_NAME);
    return EXIT_SUCCESS;
}

void m_rayClose(AppContext *ctx) {
    if (ctx && ctx->status && ctx->status->music.currentSong) {
        UnloadMusicStream(*ctx->status->music.currentSong);
        ctx->status->music.currentSong = NULL;
    }

    CloseAudioDevice();
    CloseWindow();
}


void *m_musicThreadFn(void *context) {
    AppContext *ctx = (AppContext*) context;

    while (!WindowShouldClose()) {
        if (ctx->status->music.currentSong && !ctx->status->player.isPaused) {
            if (IsMusicReady(*ctx->status->music.currentSong)){
                UpdateMusicStream(*ctx->status->music.currentSong);
            }

            float played = GetMusicTimePlayed(*ctx->status->music.currentSong);
            float total = GetMusicTimeLength(*ctx->status->music.currentSong);

            if (!isnan(played) && !isnan(total) && isfinite(played) && isfinite(total)) {
                if (played > 0.1f && (total - played) < 0.1f) {
                    m_cmdPlayNextSong(ctx);
                    printf("NEXT FUCKING SONG!\n");
                } else {
                    printf("thread played: %.0f\n", played);
                }

            }         
        }

        usleep(10000);
    }

    printf("close!\n");
    return NULL;
}

void m_handleUserInput(AppContext *ctx, Commands *cmd, int cmd_count) {
    for (int i = 0; i < cmd_count; i++) {
        if (IsKeyPressed(cmd[i].key)) {
            cmd[i].fn(ctx);
            break;
        }
    }
}

Button buttons[] = {
    { .bounds = (Rectangle){160, 50, 100, 40}, .color = RED, .label = "Pause", .onClick = m_cmdPlayPause },
    { .bounds = (Rectangle){270, 50, 100, 40}, .color = DARKBLUE, .label = "Next", .onClick = m_cmdPlayNextSong },
    { .bounds = (Rectangle){50, 50, 100, 40}, .color = DARKBLUE, .label = "Prev", .onClick = m_cmdPlayPrevSong }
};

int main(void)
{
    if (m_RaylibInit()!=0) return EXIT_FAILURE;

    Arena *a = arena_create(ARENA_BLOCK_SIZE);
    arena_t arena = {0, a};

    m_readFileToArena(&arena, MAX_SONGS);

    Commands cmd[] = {
        {KEY_TOGGLE_PLAY_PAUSE, m_cmdPlayPause},
        {KEY_SEEK_FORWARD,      m_cmdSeekForward},
        {KEY_SEEK_BACKWARD,     m_cmdSeekBackward},
        {KEY_PLAY_NEXT,         m_cmdPlayNextSong},
        {KEY_PLAY_PREVIOUS,     m_cmdPlayPrevSong},
        {KEY_MUTE_VOLUME,       m_cmdMuteVolume}
    };


    AppStatus status = {
        .player = {
            .indexSong = 0,
            .isPaused = 0,
            .isMuted = 0,
            .volume = 1.0f,
        },
        .tooltip = { 
            .visible = false,
            .timeShown = 0.0f
        },
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
    m_playSong(&ctx, song);

    pthread_t music_thread;
    pthread_create(&music_thread, NULL, m_musicThreadFn, &ctx);

    Music *currentMusic = ctx.status->music.currentSong;
    Timer timer = {0};
    
    while (!WindowShouldClose()){
        m_handleUserInput(&ctx, cmd, sizeof(cmd) / sizeof(cmd[0]));

        updateTimer(
            &timer,
            GetMusicTimePlayed(*currentMusic),
            GetMusicTimeLength(*currentMusic)
        );

        BeginDrawing();
            ClearBackground(BLACK);

            DrawText(
                strlen(ctx.status->music.tags.title) > 0 
                ? ctx.status->music.tags.title
                : arena_get_by_index(&ctx.arenaSongs, ctx.status->player.indexSong),
                10, 10, 20, RAYWHITE
            );

            drawTimer(
                &timer,
                currentMusic,
                (Rectangle){20, 230, 400, 20}
            );

            for (int i=0; i<3; i++){
                m_DrawButton(buttons[i]);
                if (m_CallbackButtonClicked(buttons[i])) buttons[i].onClick(&ctx);
            }

            if (ctx.status->tooltip.visible) m_DrawToolTip(&ctx.status->tooltip, GetFrameTime());

        EndDrawing();

        buttons[0].label = ctx.status->player.isPaused ? "Play" : "Pause";
    }

    m_rayClose(&ctx);
    pthread_join(music_thread, NULL);
    arena_free(ctx.arenaSongs.arena);

    return EXIT_SUCCESS;
}
