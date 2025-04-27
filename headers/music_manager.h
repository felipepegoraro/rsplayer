#ifndef MUSIC_MANAGER
#define MUSIC_MANAGER

#include "arena.h"
#include "id3.h"
#include "timer.h"

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
    // HashTable *songs
} AppContext;

#define MUSIC_READY_SLEEP_TIME 100000   // 100ms esperar musica ficar pronta
#define MUSIC_THREAD_SLEEP_TIME 10000   // 10ms loop do thread de musica
#define MAX_START_ATTEMPTS 20
#define DEFAULT_DIR "/home/felipe/Músicas/"
#define MAX_SONGS 100

void m_cmdPlayNextSong(void *context);
void m_cmdPlayPrevSong(void *context);
void m_cmdPlayPause(void *context);
void m_cmdSeekForward(void *context);
void m_cmdSeekBackward(void *context);
void m_cmdMuteVolume(void *context);

void m_readFileToArena(arena_t *arena, int max_songs);
void m_playSong(AppContext *ctx, const char *songPath);

void *m_musicThreadFn(void *context);

#endif // MUSIC_MANAGER
