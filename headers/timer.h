#ifndef TIMER_H
#define TIMER_H

#include "ui.h"

#include <stdio.h>
#include <stdint.h>
#include "raylib.h"

typedef struct {
    uint16_t secPlayed; // tocaods segundos
    uint16_t secTotal;  // min_played:sec_played / min_total:sec_total

    double startTime;
    double pauseTime;
    double lastPause;
    double seekOffset;
    bool   isPaused;
} Timer;

void updateTimer(Timer *t, float played, float total);
void updateTimerFromClick(Timer *timer, Music *music, Rectangle bar);

void resetTimer(Timer *t);

void formatTimerString(Timer *t, char *buffer, size_t bufferSize);

void drawTimer(Timer *timer, Music *music, Rectangle rect);

void pauseTimer(Timer *timer);
void resumeTimer(Timer *timer);

float getTimeLength(const Music m);
float getTimePlayed(const Timer t);
float getTimerProgress(const Timer *t);

void printTimer(const Timer *t);

#endif
