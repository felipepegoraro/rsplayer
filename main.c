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
#define MAX_START_ATTEMPTS 20
#define MUSIC_READY_SLEEP_TIME 100000   // 100ms esperar musica ficar pronta
#define MUSIC_THREAD_SLEEP_TIME 10000   // 10ms loop do thread de musica

typedef struct PlayerState {
    int indexSong;
    int isPaused;
    int isMuted;
    float volume;
    Timer timer;
} PlayerState;

typedef struct AppStatus {
    PlayerState player;
    Tooltip tooltip;
    RsMusic music; // atual
} AppStatus;

typedef struct AppContext {
    AppStatus *status;
    arena_t arenaSongs;
    pthread_mutex_t timer_mutex;
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

void m_cmdPlayNextSong(void *context);

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
    if (!ctx || !ctx->status) return;

    Music *music =  ctx->status->music.currentSong;
    Timer *timer = &ctx->status->player.timer;

    pthread_mutex_lock(&ctx->timer_mutex);

    if (timer->isPaused){
        ResumeMusicStream(*music);
        resumeTimer(timer);
        timer->isPaused=0;
    } else {
        PauseMusicStream(*music);
        pauseTimer(timer);
        timer->isPaused=1;
    }
    pthread_mutex_unlock(&ctx->timer_mutex);
}

void m_cmdSeekForward(void *context) {
    AppContext *ctx = (AppContext*)context;

    if (!ctx || !ctx->status) return;

    Tooltip *tooltip = &ctx->status->tooltip;
    Timer *timer = &ctx->status->player.timer;
    Music *music = ctx->status->music.currentSong;

    if (music) {
        float current = getTimePlayed(*timer);
        float total = getTimeLength(*music);

        if (!((!isnan(current) && current >= 0.0f) || 
              (!isnan(total) && total >= 0.0f))
        ){
            tooltip->message = "Erro ao carregar.";
            tooltip->visible = true;
            tooltip->color = ERROR_COLOR;
            return;
        }

        float next = current + 10.0f;
        if (next > total) next = total;
        SeekMusicStream(*music, next);

        tooltip->message = "+10s";
        tooltip->visible = true;
        tooltip->color = TIP_COLOR;

        pthread_mutex_lock(&ctx->timer_mutex);
        timer->seekOffset += 10;
        pthread_mutex_unlock(&ctx->timer_mutex);
    }
}


void m_cmdSeekBackward(void *context) {
    AppContext *ctx = (AppContext*)context;
    
    if (!ctx || !ctx->status) return;

    Tooltip *tooltip = &ctx->status->tooltip;
    Timer *timer = &ctx->status->player.timer;
    Music *music = ctx->status->music.currentSong;

    if (music) {
        float back = getTimePlayed(*timer) - 10.0f;

        if (isnan(back)){
            tooltip->message = "Erro ao carregar.";
            tooltip->visible = true;
            tooltip->color = ERROR_COLOR;
            return;
        }

        pthread_mutex_lock(&ctx->timer_mutex);
        timer->seekOffset -= 10;
        pthread_mutex_unlock(&ctx->timer_mutex);

        if (back <= 0.0f) {
            back = 0.0f;
            resetTimer(timer);
        }

        SeekMusicStream(*music, back);

        tooltip->message = "-10s";
        tooltip->visible = true;
        tooltip->color = TIP_COLOR;
    }
}

static FILE *m_openSongFile(const char *songPath) {
    FILE *f = id3_read_song_file(songPath);
    if (!f) printf("erro inesperado\n");
    return f;
}

static void m_stopAndUnloadPreviousSong(AppContext *ctx) {
    if (!ctx || !ctx->status) return;
    pthread_mutex_lock(&ctx->timer_mutex);
    Music *m = ctx->status->music.currentSong;

    if (m){
        StopMusicStream(*m);
        while (!IsMusicReady(*m));
        UnloadMusicStream(*m);
    }
    pthread_mutex_unlock(&ctx->timer_mutex);
}

static Music *m_loadNewSong(AppContext *ctx, const char *songPath, FILE *f) {
    Music tempMusic = LoadMusicStream(songPath);
    if (!tempMusic.ctxData) {
        printf("Erro ao carregar musica: %s\n", songPath);
        fclose(f);
        return NULL;
    }

    Music *musicPtr = arena_alloc(ctx->arenaSongs.arena, sizeof(Music));
    if (!musicPtr) {
        printf("Erro de alocação\n");
        UnloadMusicStream(tempMusic);
        fclose(f);
        return NULL;
    }

    memcpy(musicPtr, &tempMusic, sizeof(Music));
    return musicPtr;
}

static bool m_waitUntilMusicReady(Music *musicPtr) {
    int attempts = 0;
    while (!IsMusicReady(*musicPtr) && attempts < MAX_START_ATTEMPTS) {
        usleep(MUSIC_READY_SLEEP_TIME);
        attempts++;
    }

    if (!IsMusicReady(*musicPtr)) {
        printf("Falha ao preparar música\n");
        UnloadMusicStream(*musicPtr);
        return false;
    }
    return true;
}

static void m_prepareAndPlayMusic(AppContext *ctx, Music *musicPtr) {
    resetTimer(&ctx->status->player.timer);
    PlayMusicStream(*musicPtr);
    SetMusicVolume(*musicPtr, ctx->status->player.isMuted 
        ? 0.0f 
        : ctx->status->player.volume
    );
}


static void m_loadAndPrintTags(AppContext *ctx, FILE *f, const char *songPath) {
    if (!ctx || !f || !songPath) return;

    ID3V2_Tags tags = id3_get_song_tags(f);
    fclose(f);

    ctx->status->music.tags = tags;

    bool hasTitle   = strlen(tags.title)  > 0;
    bool hasArtist  = strlen(tags.artist) > 0;
    bool hasYear    = strlen(tags.year)   > 0;

    if (hasTitle && hasArtist && hasYear) {
        printf("Tocando: %s - %s (%s)\n", tags.title, tags.artist, tags.year);
    } else {
        printf("Tocando: %s\n", songPath);
    }
}


void m_playSong(AppContext *ctx, const char *songPath) {
    if (!ctx || !songPath) return;

    FILE *f = m_openSongFile(songPath);
    if (!f) return;

    m_stopAndUnloadPreviousSong(ctx);

    Music *musicPtr = m_loadNewSong(ctx, songPath, f);
    if (!musicPtr) return;

    if (!m_waitUntilMusicReady(musicPtr)) {
        fclose(f);
        return;
    }

    ctx->status->music.currentSong = musicPtr;

    m_prepareAndPlayMusic(ctx, musicPtr);
    m_loadAndPrintTags(ctx, f, songPath);
}

// TODO: playlist (prev/next deve verificar contexto)

void m_cmdPlayNextSong(void *context) {
    AppContext *ctx = (AppContext*)context;
    if (!ctx || !ctx->status) return;

    PlayerState *p = &ctx->status->player;
    p->indexSong = (p->indexSong + 1) % ctx->arenaSongs.total;

    const char *next = arena_get_by_index(&ctx->arenaSongs, p->indexSong);
    if (next) m_playSong(ctx, next);
}

void m_cmdPlayPrevSong(void *context) {
    AppContext *ctx = (AppContext*)context;
    if (!ctx || !ctx->status) return;

    PlayerState *p = &ctx->status->player;
    p->indexSong = (p->indexSong - 1 + ctx->arenaSongs.total) % ctx->arenaSongs.total;

    const char *prev = arena_get_by_index(&ctx->arenaSongs, p->indexSong);
    if (prev) m_playSong(ctx, prev);
}

void m_cmdMuteVolume(void *context){
    AppContext *ctx = (AppContext*)context;
    if (!ctx || !ctx->status) return;

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
    if (!ctx) return NULL;

    Music *music = NULL;
    bool isPlaying = false;

    while (!WindowShouldClose()) {
        pthread_mutex_lock(&ctx->timer_mutex);

        // acho melhor do que ficar criando uma variável em todo loop
        music     = ctx->status->music.currentSong;
        isPlaying = music && !ctx->status->player.timer.isPaused;

        if (isPlaying) {
            if (IsMusicReady(*music)) UpdateMusicStream(*music);

            float played = getTimePlayed(ctx->status->player.timer);
            float total = getTimeLength(*music);

            // mais legível...
            bool playedValid = isnan(played) == 0 && isfinite(played);
            bool totalValid  = isnan(total) == 0 && isfinite(total);

            if (playedValid && totalValid) {
                if (played > 0.1f && (total - played) < 0.1f) {
                    m_cmdPlayNextSong(ctx);
                    printf("NEXT FUCKING SONG!\n");
                }
            }         
        }

        pthread_mutex_unlock(&ctx->timer_mutex);
        usleep(MUSIC_THREAD_SLEEP_TIME);
    }

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
            .isMuted = 0,
            .volume = 1.0f,
            .timer = {0}
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
        .status = &status,
        .timer_mutex = PTHREAD_MUTEX_INITIALIZER
    };

    // TODO: fuzzy finder para selecionar mppusica (Levenshtein Distance ou Trie)
    const char *song = arena_get_by_index(&ctx.arenaSongs, 0);
    m_playSong(&ctx, song);

    pthread_t music_thread;
    pthread_create(&music_thread, NULL, m_musicThreadFn, &ctx);

    while (!WindowShouldClose()){
        m_handleUserInput(&ctx, cmd, sizeof(cmd) / sizeof(cmd[0]));

        BeginDrawing();
            ClearBackground(BLACK);

            DrawText(
                strlen(ctx.status->music.tags.title) > 0 
                ? ctx.status->music.tags.title
                : arena_get_by_index(&ctx.arenaSongs, ctx.status->player.indexSong),
                10, 10, FONT_SIZE, RAYWHITE
            );

            DrawRectangle(
                SCREEN_WIDTH-GAP*2,
                SCREEN_HEIGHT-GAP*2,
                GAP, GAP,
                IsMusicReady(*ctx.status->music.currentSong) ? GREEN : RED
            );

            m_drawTrackCounter(
                ctx.status->player.indexSong,
                ctx.arenaSongs.total,
                (Vector2){
                    .x = SCREEN_WIDTH - GAP * 3,
                    .y = SCREEN_HEIGHT - FONT_SIZE/1.5f - GAP
                }
            );

            drawTimer(
                &ctx.status->player.timer,
                ctx.status->music.currentSong,
                (Rectangle){20, SCREEN_HEIGHT-4*GAP, 400, 20}
            );

            for (int i=0; i<3; i++){
                m_DrawButton(buttons[i]);
                if (m_CallbackButtonClicked(buttons[i])) buttons[i].onClick(&ctx);
            }

            if (ctx.status->tooltip.visible) m_DrawToolTip(&ctx.status->tooltip, GetFrameTime());

        EndDrawing();

        buttons[0].label = ctx.status->player.timer.isPaused ? "Play" : "Pause";
    }

    m_rayClose(&ctx);
    pthread_join(music_thread, NULL);
    arena_free(ctx.arenaSongs.arena);

    return EXIT_SUCCESS;
}
