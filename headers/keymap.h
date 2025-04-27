#ifndef KEYMAP_H
#define KEYMAP_H

#include "raylib.h"
#include "music_manager.h"

#define KEY_NEXT_CARD_PAGE KEY_D
#define KEY_PREV_CARD_PAGE KEY_A

typedef enum {
    KEY_TOGGLE_PLAY_PAUSE = KEY_P,
    KEY_SEEK_FORWARD      = KEY_F,
    KEY_SEEK_BACKWARD     = KEY_B,
    KEY_PLAY_NEXT         = KEY_PERIOD, // '.'
    KEY_PLAY_PREVIOUS     = KEY_COMMA,  // ','
    KEY_MUTE_VOLUME       = KEY_M,
} KeyCommand;

typedef struct Commands {
    KeyCommand key;
    void (*fn) (void*);
} Commands;


static int musicCardPage = 0;

// gerencia o input do usuário exclusivamente para controle da música
void handleMusicUserInput(AppContext *ctx, Commands *cmd, int cmd_count) {
    for (int i = 0; i < cmd_count; i++) {
        if (IsKeyPressed(cmd[i].key)) {
            cmd[i].fn(ctx);
            break;
        }
    }
}

#endif // KEYMAP_H
