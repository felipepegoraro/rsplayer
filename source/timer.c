#include "../headers/timer.h"
#include <math.h>

void updateTimer(Timer *t, float played, float total){
    if (!t) return;

    t->secPlayed = (int)played;
    t->secTotal = (int)total;;
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
    if (!t) return 0.0f;

    float pl = t->secPlayed; 
    float tt = t->secTotal;

    if (pl <= 0.0f || tt <= 0.0f || isnan(tt) || isnan(pl) || isinf(tt) || isinf(pl))
        return 0.0f;

    return (pl / tt * 100.0f);
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

            // DEBUG OUTPUT
            printf("Mouse position: x=%.2f, y=%.2f\n", mousePos.x, mousePos.y);
            printf("Bar: x=%.2f, width=%.2f\n", bar.x, bar.width);
            printf("Clicked percent on bar: %.2f%%\n", porc * 100.0f);
            printf("Total seconds of music: %.2f\n", totalSeconds);
            printf("New time (to seek): %.2f\n", newTime);

            if (newTime < totalSeconds) {
                PauseMusicStream(*music);
                SeekMusicStream(*music, newTime);
                ResumeMusicStream(*music);
                updateTimer(timer, newTime, totalSeconds);

                printf("Timer updated:\n");
                printf("  secPlayed: %d\n", timer->secPlayed);
                printf("  secTotal:  %d\n", timer->secTotal);
                printf("  startTime: %.2f\n", timer->startTime);
                printf("  pauseTime: %.2f\n", timer->pauseTime);
                printf("  lastPause: %.2f\n", timer->lastPause);
                printf("  seekOffset: %.2f\n", timer->seekOffset);
                printf("  isPaused: %s\n", timer->isPaused ? "true" : "false");
            }

            timer->seekOffset = newTime;
            if (timer->isPaused){
                timer->lastPause = GetTime();
                timer->pauseTime = 0.0;
            }
        }
    }
}

void drawTimer(Timer *timer, Music *music, Rectangle rect){
    if (!timer || !music) {
        printf("okokokok\n");
        return;
    }

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
    timer->isPaused = 1;
}

void resumeTimer(Timer *timer){
    if (!timer) return;

    timer->pauseTime += (GetTime() - timer->lastPause);
    timer->isPaused = false;
}

float getTimeLength(const Music m){
    return (float)m.frameCount / (float)(m.stream.sampleRate * m.stream.channels);
}

float getTimePlayed(const Timer t){
    double now = GetTime();
    double base = t.seekOffset;

    return (int)((t.isPaused)
        ? base + (t.lastPause - t.startTime)
        : base + (now - t.startTime - t.pauseTime));
}
