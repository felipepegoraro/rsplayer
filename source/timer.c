#include "../headers/timer.h"
#include <math.h>

void updateTimer(Timer *t, float played, float total){
    if (!t) return;

    t->secPlayed = (uint16_t)played;
    t->secTotal = (uint16_t) total;;
}

void resetTimer(Timer *t) {
    if (!t) return;

    *t = (Timer) {
        .startTime = GetTime(),
        .pauseTime = 0.0,
        .lastPause = 0.0,
        .seekOffset = 0.0,
        .secPlayed = 0,
        .secTotal = 0,
        .isPaused = 0
    };
    printf("==========!\n");
}

void formatTimerString(Timer *t, char *buffer, size_t bufferSize) {
    if (!t) {
        printf("???????\n");
        return;
    }

    snprintf(
        buffer, bufferSize,
        "%d:%02d / %d:%02d",
        t->secPlayed / 60,
        t->secPlayed % 60,
        t->secTotal  / 60,
        t->secTotal  % 60 
    );
}


float getTimerProgress(const Timer *t){
    if (!t || t->secTotal <= 0.0f) return 0.0f;

    float played = getTimePlayed(*t);  // tempo real agora agora
    float total  = (float)t->secTotal;

    if (played <= 0.0f || isnan(total) || isinf(total) || isnan(played) || isinf(played))
        return 0.0f;

    return (played / total) * 100.0f;
}

#include <stdio.h> // para printf

void updateTimerFromClick(Timer *timer, Music *music, Rectangle bar) {
    if (!timer || !music) return;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mousePos = GetMousePosition();

        if (CheckCollisionPointRec(mousePos, bar)) {
            float porc = (mousePos.x - bar.x) / bar.width;
            float totalSeconds = getTimeLength(*music);
            float newTime = porc * totalSeconds;

            if (newTime < totalSeconds) {
                PauseMusicStream(*music);
                SeekMusicStream(*music, newTime);
                ResumeMusicStream(*music);

                updateTimer(timer, newTime, totalSeconds);

                timer->seekOffset = newTime;
                double now = GetTime();
                timer->startTime = now;
                timer->lastPause = now;
                timer->pauseTime = 0.0f;
            }
        }
    }
}

// todo: mover para ui.c
void drawTimer(Timer *timer, Music *music, Rectangle rect){
    if (!timer || !music) {
        printf("okokokok\n");
        return;
    }

    float played = getTimePlayed(*timer);
    float total = getTimeLength(*music);
    updateTimer(timer, played, total);

    char percBuffer[5];
    char timerBuffer[32];

    float musicProgress = getTimerProgress(timer);
    snprintf(percBuffer, sizeof(percBuffer), "%.0f%%", musicProgress);
    formatTimerString(timer, timerBuffer, sizeof(timerBuffer));

    DrawText(timerBuffer, 20, rect.y-40, 20, WHITE);
    DrawText(percBuffer, 430, rect.y, 20, WHITE);

    DrawRectangle(rect.x, rect.y, rect.width, rect.height, GRAY);

    DrawRectangle(
        rect.x,
        rect.y,
        (int)(rect.width * (musicProgress/100)),
        rect.height,
        GREEN
    );

    updateTimerFromClick(timer, music, rect);
}


void pauseTimer(Timer *timer){
    if (!timer) return;

    timer->lastPause = GetTime();
    timer->isPaused = true;
}

void resumeTimer(Timer *timer){
    if (!timer) return;

    timer->pauseTime += (GetTime() - timer->lastPause);
    timer->isPaused = false;
}

float getTimeLength(const Music m){
    // return (float)m.frameCount / (float)(m.stream.sampleRate * m.stream.channels);
    return (float)m.frameCount / (float)(m.stream.sampleRate);
}

float getTimePlayed(const Timer t){
    double now = GetTime();
    double base = t.seekOffset;

    return (float)((t.isPaused)
        ? base + (t.lastPause - t.startTime)
        : base + (now - t.startTime - t.pauseTime));
}
