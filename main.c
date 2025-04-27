// RSPLAYER: um tocador de mp3
// 
// autor        |  @felipepegoraro
// data         |  meados de 2025
// dependencias |  linux, gcc e raylib
// 
// afazeres: 
// 1. barra de pesquisa funcional + uso da hashmap + trie
// 2. ideia de playlist (add,remove,order,etc)
// 3. esconder, loop, repetir 1x
// 4. deixar a ui mais bonita
// 5. alternar entre lista de música e vu meter
// 6. mostrar as tags id3 no card music em vez no path


#include <pthread.h>
#include "./headers/ui.h"
#include "./headers/keymap.h"


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


Button buttons[] = {
    { .bounds = (Rectangle){SCREEN_WIDTH/2.0-50     , SCREEN_HEIGHT-100, 100, 40}, .color = RED,      .label = "Pause", .onClick = m_cmdPlayPause    },
    { .bounds = (Rectangle){SCREEN_WIDTH/2.0+50+GAP , SCREEN_HEIGHT-100, 100, 40}, .color = DARKBLUE, .label = "Next",  .onClick = m_cmdPlayNextSong },
    { .bounds = (Rectangle){SCREEN_WIDTH/2.0-150-GAP, SCREEN_HEIGHT-100, 100, 40}, .color = DARKBLUE, .label = "Prev",  .onClick = m_cmdPlayPrevSong }
};


#include <stdio.h>

#define CARD_WIDTH 570
#define CARD_HEIGHT 55
#define MAX_MUSICS_PER_PAGE 5
#define MUSIC_CARD_START_Y 110
#define MAX_DISPLAY_NAME_SIZE 128
#define CARD_SPACING 70

void drawMusicCard(Vector2 pos, const char *musicName, size_t index, int current) {
    char indexStr[4];
    snprintf(indexStr, sizeof(indexStr), "%zu", index);

    char displayName[MAX_DISPLAY_NAME_SIZE];

    m_truncateTextWithEllipsis(musicName, displayName, CARD_WIDTH - 125, FONT_SIZE);

    Rectangle card = {pos.x + GAP, pos.y, CARD_WIDTH, CARD_HEIGHT};

    DrawRectangleRounded(card, 0.4, 24, current ? GREEN : LIGHTGRAY);

    DrawText(indexStr, pos.x + 3 * GAP, pos.y + 2 * GAP, FONT_SIZE, BLACK);
    DrawText(displayName, pos.x + 4 * GAP + 30, pos.y + 2 * GAP, FONT_SIZE, BLACK);
}


int handleClickOnMusicCard(AppContext *ctx, Vector2 mousePos) {
    if (!ctx) return -1;

    if (mousePos.y < MUSIC_CARD_START_Y || mousePos.y >= MUSIC_CARD_START_Y + CARD_SPACING * MAX_MUSICS_PER_PAGE) {
        return -1;
    }

    int musicIndex = (mousePos.y - MUSIC_CARD_START_Y) / CARD_SPACING;
    if (musicIndex >= MAX_MUSICS_PER_PAGE) musicIndex = MAX_MUSICS_PER_PAGE - 1;
    
    return musicIndex + musicCardPage; 
}

void handleKeyboardPageMusicCardNavigation(int nOfPages) {
    if (IsKeyPressed(KEY_PREV_CARD_PAGE)) {
        musicCardPage -= MAX_MUSICS_PER_PAGE;
        if (musicCardPage < 0) musicCardPage = 0;
    }

    if (IsKeyPressed(KEY_NEXT_CARD_PAGE)) {
        musicCardPage += MAX_MUSICS_PER_PAGE;
        if (musicCardPage >= nOfPages * MAX_MUSICS_PER_PAGE) musicCardPage = (nOfPages - 1) * MAX_MUSICS_PER_PAGE;
    }
}

void handleMouseScrollPageMusicCardNavigation(int nOfPages) {
    float wheel = GetMouseWheelMove();
    float smooth = MAX_MUSICS_PER_PAGE * 0.4f;

    if (wheel > 0) {
        musicCardPage -= (int)smooth;
        if (musicCardPage < 0) musicCardPage = 0;
    }

    if (wheel < 0) {
        musicCardPage += (int)smooth;
        if (musicCardPage >= nOfPages * MAX_MUSICS_PER_PAGE) musicCardPage = (nOfPages - 1) * MAX_MUSICS_PER_PAGE;
    }
}


void drawMenuDisplay(AppContext *ctx) {
    const int startX = GAP * 6 + 45;
    const int startY = MUSIC_CARD_START_Y;
    const int screenHeightLimit = SCREEN_WIDTH * 0.47 + 100;
    const int nOfPages = (ctx->arenaSongs.total + MAX_MUSICS_PER_PAGE - 1) / MAX_MUSICS_PER_PAGE;

    // DrawRectangle(GAP * 5 + 50, 100, SCREEN_WIDTH * 0.75, SCREEN_WIDTH * 0.47, GRAY);

    handleKeyboardPageMusicCardNavigation(nOfPages);
    handleMouseScrollPageMusicCardNavigation(nOfPages);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
        int newIndex = handleClickOnMusicCard(ctx, GetMousePosition());
        if (newIndex != -1) {
            m_playSong(ctx, arena_get_by_index(&ctx->arenaSongs, newIndex));
            ctx->status->player.indexSong = newIndex;
        }
    }

    int musicDrawn = 0;
    Vector2 start = { startX, startY };

    for (size_t i = musicCardPage; i < ctx->arenaSongs.total && musicDrawn < MAX_MUSICS_PER_PAGE; i++) {
        if (start.y + CARD_SPACING > screenHeightLimit) break;

        drawMusicCard(
            start,
            arena_get_by_index(&ctx->arenaSongs, i),
            i,
            (size_t)ctx->status->player.indexSong == i
        );
        
        start.y += CARD_SPACING;
        musicDrawn++;
    }
}


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
    };


    // Playlist playlist = playlist_init("Playlist", false, &ctx.status->music);
    // playlist_free(&playlist);

    // TODO: fuzzy finder para selecionar mppusica (Levenshtein Distance ou Trie)
    const char *song = arena_get_by_index(&ctx.arenaSongs, 0);
    m_playSong(&ctx, song);

    pthread_t music_thread;
    pthread_create(&music_thread, NULL, m_musicThreadFn, &ctx);

    while (!WindowShouldClose()){
        handleMusicUserInput(&ctx, cmd, sizeof(cmd) / sizeof(cmd[0]));

        BeginDrawing();
            ClearBackground(BLACK);

            drawMenuDisplay(&ctx);
            DrawText(PLAYER_NAME, GAP*2, GAP*2, FONT_SIZE*1.5, RAYWHITE);
            DrawRectangle(SCREEN_WIDTH*0.75-50-MeasureText(PLAYER_NAME,FONT_SIZE*1.5), GAP*2.5-2, SCREEN_WIDTH/2.5, 20, RAYWHITE);
            DrawText(
                strlen(ctx.status->music.tags.title) > 0 
                    ? ctx.status->music.tags.title
                    : arena_get_by_index(&ctx.arenaSongs, ctx.status->player.indexSong),
                GAP*2,
                FONT_SIZE*2.4+GAP, FONT_SIZE, RAYWHITE
            );
            DrawRectangle(
                SCREEN_WIDTH-25,
                GAP*1.5f,
                GAP, GAP,
                IsMusicReady(*ctx.status->music.currentSong) ? GREEN : RED
            );

            m_drawTrackCounter(
                ctx.status->player.indexSong,
                ctx.arenaSongs.total,
                (Vector2){
                    .x = SCREEN_WIDTH - GAP * 2.8,
                    .y = FONT_SIZE/2.0
                }
            );

            drawTimer(
                &ctx.status->player.timer,
                ctx.status->music.currentSong,
                (Rectangle){(int)(SCREEN_WIDTH/2)-200, SCREEN_HEIGHT-4*GAP, 400, 20}
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
