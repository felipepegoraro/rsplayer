#ifndef UI_H
#define UI_H

#include "raylib.h"

#define PLAYER_NAME "RSPLAYER"

#define GAP 10
#define MAIN_BORDER_RADIUS 0.05f

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define FONT_SIZE 20

#define TIP_COLOR     ((Color){ 100, 149, 237, 255 }) // cornflower blue
#define ERROR_COLOR   ((Color){ 220,  20,  60, 255 }) // crimson
#define WARNING_COLOR ((Color){ 255, 165,   0, 255 }) // laranja
#define SUCCESS_COLOR ((Color){  60, 179, 113, 255 }) // medium sea green

typedef struct {
    Rectangle bounds;
    Color color;
    const char *label;
    void (*onClick)(void*);
} Button;

typedef struct Tooltip {
    Color color;
    float timeShown;
    bool visible;
    char message[32];
    // const char* message;
} Tooltip;


// BOTAO ------------------------------------------------------
#define BTN_WIDTH 150
#define BTN_HEIGHT 50
#define BTN_RADIUS 0.3f
#define BTN_SEGMENTS 10

void m_DrawButton(Button button);
bool m_CallbackButtonClicked(Button button);



// TOOLTIP ----------------------------------------------------
#define TOOLTIP_WIDTH  230
#define TOOLTIP_HEIGHT  FONT_SIZE+GAP*2
#define TOOLTIP_DURATION 1.4f

void m_DrawToolTip(Tooltip *t, float deltaTime);


// TRACK COUNTER ----------------------------------------------
#include <stdio.h>
void m_drawTrackCounter(size_t current, size_t total, Vector2 pos);


#include <string.h>
// MUSIC CARD -------------------------------------------------
void m_truncateTextWithEllipsis(const char *, char *, size_t, int);

#endif
