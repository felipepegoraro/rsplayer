#ifndef TIMER_H
#define TIMER_H

#include <stdio.h>
#include "raylib.h"

typedef struct {
    int sec_played; // segundos
    int min_played; // minutos
    int min_total;
    int sec_total; // min_played:sec_played / min_total:sec_total
} Timer;

void updateTimer(Timer *t, float played, float total);
Timer newTimer(float played, float total);
void resetTimer(Timer *t);
void updateTimer(Timer *t, float played, float total);
void formatTimerString(Timer *t, char *buffer, size_t bufferSize);
float getTimerProgress(Timer *t);
void updateTimerFromClick(Timer *timer, Music *music, Rectangle bar);
void drawTimer(Timer *timer, Music *music, Rectangle rect);

#endif
