#ifndef UI_H
#define UI_H

#include "raylib.h"

#define PLAYER_NAME "RSPLAYER"

#define GAP 10
#define MAIN_BORDER_RADIUS 0.05f

#define BTN_WIDTH 150
#define BTN_HEIGHT 50
#define BTN_RADIUS 0.3f
#define BTN_SEGMENTS 10

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define FONT_SIZE 20

typedef struct {
    Rectangle bounds;
    Color color;
    const char *label;
    void (*onClick)(void*);
} Button;

void m_DrawButton(Button button);
bool m_CallbackButtonClicked(Button button);

#endif
