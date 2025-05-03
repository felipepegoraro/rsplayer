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


// #define MAX_INPUT_SEQUENCE 8

static int musicCardPage;

int handleMusicUserInput(AppContext *ctx, Commands *cmd, int cmd_count) {
    int *repeatCmdFactor = &ctx->keyLog.localRepeater;

    for (int key = KEY_ZERO; key <= KEY_NINE; key++) {
        if (IsKeyPressed(key)) {
            int digit = key - KEY_ZERO;
            int next = (*repeatCmdFactor) * 10 + digit;

            if (next > 10) return 0;

            *repeatCmdFactor = next;
            return 1;
        }
    }

    int ret = 0;

    for (int i = 0; i < cmd_count; i++) {
        if (IsKeyPressed(cmd[i].key)) {
            int times = *repeatCmdFactor > 0 ? *repeatCmdFactor : 1;
            for (int r = 0; r < times; r++) {
                cmd[i].fn(ctx);

                if (ctx->keyLog.count < ctx->keyLog.capacity) {
                    ctx->keyLog.key[ctx->keyLog.count++] = i;
                } else {
                    for (int j = 1; j < ctx->keyLog.capacity; j++)
                        ctx->keyLog.key[j - 1] = ctx->keyLog.key[j];
                    ctx->keyLog.key[ctx->keyLog.capacity - 1] = i;
                }
            }
    
            *repeatCmdFactor = 0;
            ret = 1;
            break;
        }
    }

    return ret;
}

#endif // KEYMAP_H
