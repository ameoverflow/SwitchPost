//
// Created by void on 03/03/2026.
//

#include "Helpers.h"

#include <string>
#include <algorithm>

bool showFakePackages = false;
bool shouldQuit = false;
int currentBackground = 0;
std::vector<Texture2D> backgrounds = {};

void DrawTextOutlineEx(Font font, const char* text, Vector2 position, Vector2 origin, float fontSize, float spacing, Color textColor, Color outlineColor, int thickness) {
    // calculate the actual top-left starting point based on the origin
    Vector2 drawPos = { position.x - origin.x, position.y - origin.y };

    // draw the outline circle
    for (int x = -thickness; x <= thickness; x++) {
        for (int y = -thickness; y <= thickness; y++) {
            if (x != 0 || y != 0) {
                Vector2 currentPos = { drawPos.x + x, drawPos.y + y };
                DrawTextEx(font, text, currentPos, fontSize, spacing, outlineColor);
            }
        }
    }

    // draw the main text
    DrawTextEx(font, text, drawPos, fontSize, spacing, textColor);
}

float GetMappedAxis(float raw_val, float max_out, float deadzone) {
    if (raw_val < deadzone) return 0.0f;

    float val = std::clamp(raw_val, deadzone, 1.0f);

    return (val - deadzone) / (1.0f - deadzone) * max_out;
}