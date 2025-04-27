#include "../headers/ui.h"

void m_DrawButton(Button button) {
    Rectangle bounds = button.bounds;
    bool isHovered = CheckCollisionPointRec(GetMousePosition(), bounds);

    Color buttonColor = isHovered ? ColorAlpha(button.color, 0.80f) : button.color;
    DrawRectangleRounded(bounds, BTN_RADIUS, BTN_SEGMENTS, buttonColor);

    DrawText(
        button.label,
        bounds.x + (int)(bounds.width/2) - (int)(MeasureText(button.label, FONT_SIZE)/2),
        bounds.y + (bounds.height / 2) - 10,
        FONT_SIZE, WHITE
    );
}


bool m_CallbackButtonClicked(Button button) {
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
           && CheckCollisionPointRec(GetMousePosition(), button.bounds);
}


float m_getColorBrightness(Color color) {
    // if (color.r > 0 || color.b == 0 || color.g == 0) return 0.0f;
    return (0.299f * color.r + 0.587f * color.g + 0.114f * color.b) / 255.0f;
}


void m_DrawToolTip(Tooltip *t, float deltaTime){
    if (!t) return;

    t->timeShown += deltaTime;

    float remaining = TOOLTIP_DURATION - t->timeShown;
    if (remaining <= 0.0f){
        t->visible = false;
        t->timeShown = 0.1f;
        return;
    } 

    float alpha = remaining <= 0.5 ? remaining/0.5f : 1.0f;

    Color baseColor = ColorAlpha(t->color, alpha);
    Color textColor = m_getColorBrightness(baseColor) > 0.5f 
        ? ColorAlpha(BLACK, alpha) 
        : ColorAlpha(WHITE, alpha);

    int gap  = GAP*2;
    int posX = SCREEN_WIDTH-TOOLTIP_WIDTH-gap;

    DrawRectangleRounded(
        (Rectangle){posX, gap, TOOLTIP_WIDTH, TOOLTIP_HEIGHT},
        BTN_RADIUS,
        BTN_SEGMENTS,
        baseColor
    );

    DrawText(
        t->message,
        posX+GAP,
        gap + (TOOLTIP_HEIGHT - FONT_SIZE) / 2,
        FONT_SIZE,
        textColor
    );
}

void m_drawTrackCounter(size_t current, size_t total, Vector2 pos){
    char trackInfo[16];
    snprintf(trackInfo, sizeof(trackInfo), "%zu/%zu", current+1, total);

    int textWidth = MeasureText(trackInfo, FONT_SIZE);
    int textX = pos.x - textWidth - 5;
    DrawText(trackInfo, textX, pos.y, FONT_SIZE, WHITE);
}


void m_truncateTextWithEllipsis(
    const char *input,
    char *output,
    size_t maxWidth,
    int fontSize
) {
    size_t estimatedCharWidth = fontSize / 2; 
    size_t maxChars = maxWidth / estimatedCharWidth;
    size_t inputLen = strlen(input);

    if (inputLen <= maxChars) strcpy(output, input);
    else {
        if (maxChars > 3) maxChars -= 3;
        else maxChars = 0;

        strncpy(output, input, maxChars);
        output[maxChars] = '\0';
        strcat(output, "...");
    }
}
