#include "../headers/timer.h"

void updateTimer(Timer *t, float played, float total){
    int playedSec = (int)played;
    int totalSec = (int)total;

    t->min_played = playedSec / 60;
    t->sec_played = playedSec % 60;
    t->min_total = totalSec / 60;
    t->sec_total = totalSec % 60;
}

Timer newTimer(float played, float total){
    Timer t = {0};
    updateTimer(&t, played, total);
    return t;
}


// nao usado.
// void resetTimer(Timer *t){
//     if (t){
//         *t = (Timer){
//             .min_total = 0,
//             .sec_total = 0,
//             .sec_played = 0,
//             .min_played = 0,
//         };
//     }
// }


void formatTimerString(Timer *t, char *buffer, size_t bufferSize) {
    if (t){
        snprintf(
            buffer, bufferSize,
            "%d:%02d / %d:%02d",
            t->min_played,
            t->sec_played,
            t->min_total,
            t->sec_total
        );
    }
}


float getTimerProgress(Timer *t){
    if (t){
        float pl = t->min_played * 60 + t->sec_played;
        float tt = t->min_total * 60 + t->sec_total;
        return tt == 0 ? 0.0f : (float)(pl / tt * 100.0f);
    }

    return 0.0f;
}



void updateTimerFromClick(Timer *timer, Music *music, Rectangle bar) {
    if (!timer || !music) return;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mousePos = GetMousePosition();

        if (CheckCollisionPointRec(mousePos, bar)) {
            float porc = (mousePos.x - bar.x) / bar.width;
            float totalSeconds = GetMusicTimeLength(*music);
            float newTime = porc * totalSeconds;

            if (newTime < totalSeconds) {
                PauseMusicStream(*music);
                SeekMusicStream(*music, newTime);
                ResumeMusicStream(*music);
            }
        }
    }
}

void drawTimer(Timer *timer, Music *music, Rectangle rect){
    if (!timer || !music) return;

    char percBuffer[5];
    char timerBuffer[32];

    float musicProgress = getTimerProgress(timer);
    snprintf(percBuffer, sizeof(percBuffer), "%.0f%%", musicProgress);
    formatTimerString(timer, timerBuffer, sizeof(timerBuffer));

    DrawText(timerBuffer, 20, 200, 20, WHITE);
    DrawText(percBuffer, 430, rect.y, 20, WHITE);

    DrawRectangle(rect.x, rect.y, rect.width, rect.height, GRAY);
    DrawRectangle(rect.x, rect.y, (int)(rect.width * (musicProgress/100)), rect.height, GREEN);

    updateTimerFromClick(timer, music, rect);
}
