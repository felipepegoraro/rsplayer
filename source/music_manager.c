#include "../headers/music_manager.h"
#include <math.h>
#include <unistd.h>
#include <dirent.h>


// toca próxima música
void m_cmdPlayNextSong(void *context) {
    AppContext *ctx = (AppContext*)context;
    if (!ctx || !ctx->status) return;

    PlayerState *p = &ctx->status->player;
    p->indexSong = (p->indexSong + 1) % ctx->arenaSongs.total;

    const char *next = arena_get_by_index(&ctx->arenaSongs, p->indexSong);
    if (next) m_playSong(ctx, next);
}


// toca música anterior
void m_cmdPlayPrevSong(void *context) {
    AppContext *ctx = (AppContext*)context;
    if (!ctx || !ctx->status) return;

    PlayerState *p = &ctx->status->player;
    p->indexSong = (p->indexSong - 1 + ctx->arenaSongs.total) % ctx->arenaSongs.total;

    const char *prev = arena_get_by_index(&ctx->arenaSongs, p->indexSong);
    if (prev) m_playSong(ctx, prev);
}


// controla o pause
void m_cmdPlayPause(void *context){
    AppContext *ctx = (AppContext*)context;
    if (!ctx || !ctx->status) return;

    Music *music =  ctx->status->music.currentSong;

    if (ctx->status->player.timer.isPaused){
        ResumeMusicStream(*music);
        resumeTimer(&ctx->status->player.timer);
    } else {
        PauseMusicStream(*music);
        pauseTimer(&ctx->status->player.timer);
    }
}


// adianta a música em 10s
void m_cmdSeekForward(void *context) {
    AppContext *ctx = (AppContext*)context;

    if (!ctx || !ctx->status) return;

    Tooltip *tooltip = &ctx->status->tooltip;
    Timer *timer = &ctx->status->player.timer;
    Music *music = ctx->status->music.currentSong;

    if (!music) return;
    if (IsMusicStreamPlaying(*music)) PauseMusicStream(*music);

    float current = getTimePlayed(*timer);
    float total = getTimeLength(*music);

    if (!((!isnan(current) && current >= 0.0f) || 
          (!isnan(total) && total >= 0.0f))
    ){
        snprintf(tooltip->message, sizeof(tooltip->message), "Erro ao carregar.");
        tooltip->visible = true;
        tooltip->color = ERROR_COLOR;
        return;
    }

    float next = current + 10.0f;
    if (next > total) next = total;
    SeekMusicStream(*music, next);

    int nt = ctx->keyLog.localRepeater;

    if (nt == 0) snprintf(tooltip->message, sizeof(tooltip->message), "+10s");
    else snprintf(tooltip->message, sizeof(tooltip->message), "%ds", nt * 10);

    tooltip->visible = true;
    tooltip->color = TIP_COLOR;

    timer->seekOffset += 10;

    ResumeMusicStream(*music);
}


// retrocede a música em 10s
void m_cmdSeekBackward(void *context) {
    AppContext *ctx = (AppContext*)context;
    
    if (!ctx || !ctx->status) return;

    Tooltip *tooltip = &ctx->status->tooltip;
    Timer *timer = &ctx->status->player.timer;
    Music *music = ctx->status->music.currentSong;


    if (!music) return;
    if (IsMusicStreamPlaying(*music)) PauseMusicStream(*music);

    float back = getTimePlayed(*timer) - 10.0f;

    if (isnan(back)){
        snprintf(tooltip->message, sizeof(tooltip->message), "Erro ao carregar.");
        tooltip->visible = true;
        tooltip->color = ERROR_COLOR;
        return;
    }

    timer->seekOffset -= 10;
    if (back <= 0.0f) {
        back = 0.0f;
        resetTimer(timer);
    }

    SeekMusicStream(*music, back);

    int nt = ctx->keyLog.localRepeater;

    if (nt == 0) snprintf(tooltip->message, sizeof(tooltip->message), "-10s");
    else snprintf(tooltip->message, sizeof(tooltip->message), "%ds", nt * -10);

    tooltip->visible = true;
    tooltip->color = TIP_COLOR;

    ResumeMusicStream(*music);
}


// muta e desmuta
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

    snprintf(t->message, sizeof(t->message), "%s", p->isMuted ? "Volume mutado." : "Volume desmutado.");
    t->color   = TIP_COLOR;
    t->visible = true;
}


// le (no máximo MAX_SONGS) arquivos para a arena de músicas
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


// PLAYSONG HELPER: abre o arquivo via path
static FILE *m_openSongFile(const char *songPath) {
    FILE *f = id3_read_song_file(songPath);
    if (!f) printf("erro inesperado\n");
    return f;
}


// PLAYSONG HELPER: descarrega/libera música anterior
static void m_stopAndUnloadPreviousSong(AppContext *ctx) {
    if (!ctx || !ctx->status) return;

    Music *m = ctx->status->music.currentSong;
    if (!m || !m->ctxData) return;

    StopMusicStream(*m);
    UnloadMusicStream(*m);

    memset(m, 0, sizeof(Music));
}


// PLAYSONG HELPER: carrega nova música
static Music *m_loadNewSong(AppContext *ctx, const char *songPath, FILE *f) {
    if (access(songPath, F_OK) == -1) {
        printf("Erro: o arquivo %s não existe\n", songPath);
        fclose(f);
        return NULL;
    }

    Music tempMusic = LoadMusicStream(songPath);

    if (!tempMusic.ctxData) {
        printf("Erro ao carregar musica: %s\n", songPath);
        fclose(f);
        return NULL;
    }

    Music *musicPtr = arena_alloc(ctx->arenaSongs.arena, sizeof(Music));

    if (!musicPtr) {
        UnloadMusicStream(tempMusic);
        fclose(f);
        return NULL;
    }

    memcpy(musicPtr, &tempMusic, sizeof(Music));
    return musicPtr;
}


// PLAYSONG HELPER: espera até que a música seja carregada
static bool m_waitUntilMusicReady(Music *musicPtr) {
    int attempts = 0;
    while (!IsMusicReady(*musicPtr) && attempts < MAX_START_ATTEMPTS) {
        usleep(MUSIC_READY_SLEEP_TIME);
        attempts++;
    }

    if (!IsMusicReady(*musicPtr)) {
        UnloadMusicStream(*musicPtr);
        return false;
    }
    return true;
}


// PLAYSONG HELPER: prepara e toca a música
static void m_prepareAndPlayMusic(AppContext *ctx, Music *musicPtr) {
    resetTimer(&ctx->status->player.timer);
    PlayMusicStream(*musicPtr);
    SetMusicVolume(*musicPtr, ctx->status->player.isMuted 
        ? 0.0f 
        : ctx->status->player.volume
    );
}


// PLAYSONG HELPER: carrega as TAGS ID3
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


// toca a música
void m_playSong(AppContext *ctx, const char *songPath) {
    if (!ctx || !songPath) {
        printf("Erro: Contexto ou caminho da música inválido.\n");
        return;
    }

    FILE *f = m_openSongFile(songPath);
    if (!f) {
        printf("Erro: Não foi possível abrir o arquivo de música.\n");
        return;
    }

    m_stopAndUnloadPreviousSong(ctx);

    Music *musicPtr = m_loadNewSong(ctx, songPath, f);

    if (!musicPtr || !musicPtr->ctxData) {
        printf("Erro: Não foi possível carregar a nova música.\n");
        fclose(f);
        return;
    }

    if (!m_waitUntilMusicReady(musicPtr)) {
        printf("Erro: A música não ficou pronta.\n");
        fclose(f);
        return;
    }

    ctx->status->music.currentSong = musicPtr;
    m_prepareAndPlayMusic(ctx, musicPtr);
    m_loadAndPrintTags(ctx, f, songPath);
}


// THREAD PRINCIPAL: atualiza a música em background
void *m_musicThreadFn(void *context) {
    AppContext *ctx = (AppContext*) context;
    if (!ctx) return NULL;

    Music *music = NULL;
    bool isPlaying = false;

    while (!WindowShouldClose()) {

        music = ctx->status->music.currentSong;
        isPlaying = music && !ctx->status->player.timer.isPaused;

        if (isPlaying && IsMusicReady(*music)) {
            UpdateMusicStream(*music);

            float played = getTimePlayed(ctx->status->player.timer);
            float total = getTimeLength(*music);

            if (!isnan(played) && !isnan(total) && isfinite(played) && isfinite(total)) {
                if (played > 0.1f && (total - played) < 0.1f) {
                    m_cmdPlayNextSong(ctx);
                    printf("next fucking song!\n");
                    continue;
                }
            }
        }


        usleep(MUSIC_THREAD_SLEEP_TIME);
    }

    return NULL;
}



