#include "../headers/ui.h"

void m_DrawButton(Button button) {
    bool isHovered = CheckCollisionPointRec(GetMousePosition(), button.bounds);

    Color buttonColor = isHovered ? ColorAlpha(button.color, 0.80f) : button.color;
    DrawRectangleRounded(button.bounds, BTN_RADIUS, BTN_SEGMENTS, buttonColor);

    DrawText(
        button.label,
        button.bounds.x + (int)(button.bounds.width/2) - (int)(MeasureText(button.label, FONT_SIZE)/2),
        button.bounds.y + (button.bounds.height / 2) - 10,
        FONT_SIZE, WHITE
    );
}

void m_ClickCallback(Vector2 mouse, Rectangle *rect, void (*callback)()){
    if (CheckCollisionPointRec(mouse, *rect)) callback();
}
