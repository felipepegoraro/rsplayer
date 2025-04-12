#include "../headers/timer.h"
#include <math.h>

void updateTimer(Timer *t, float played, float total){
    if (!t) return;

    t->secPlayed = (int)played;
    t->secTotal = (int)total;;
}


Timer newTimer(float played, float total){
    Timer t = {0};
    printf("played: %d", t.secPlayed);
    updateTimer(&t, played, total);
    printf("played: %d", t.secPlayed);
    return t;
}

void resetTimer(Timer *t) {
    if (t) {
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
    if (t){
        float pl = t->secPlayed; 
        float tt = t->secTotal;

        if (pl <= 0.0f || tt <= 0.0f || isnan(tt) || isnan(pl) || isinf(tt) || isinf(pl))
            return 0.0f;

        return (pl / tt * 100.0f);
    }

    return 0.0f;
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
                timer->seekOffset = newTime;

                // Print updated timer
                printf("Timer updated:\n");
                printf("  secPlayed: %d\n", timer->secPlayed);
                printf("  secTotal:  %d\n", timer->secTotal);
                printf("  startTime: %.2f\n", timer->startTime);
                printf("  pauseTime: %.2f\n", timer->pauseTime);
                printf("  lastPause: %.2f\n", timer->lastPause);
                printf("  seekOffset: %.2f\n", timer->seekOffset);
                printf("  isPaused: %s\n", timer->isPaused ? "true" : "false");
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

    DrawText(timerBuffer, 20, 200, 20, WHITE);
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

int getTimeLength(const Music m){
    return m.frameCount / (m.stream.sampleRate * m.stream.channels);
}

int getTimePlayed(Timer t){
    double now = GetTime();
    double base = t.seekOffset;

    return (int)((t.isPaused)
        ? base + (t.lastPause - t.startTime)
        : base + (now - t.startTime - t.pauseTime));
}
