//
// Created by void on 03/03/2026.
//

#ifndef SWITCHPOST_HELPERS_H
#define SWITCHPOST_HELPERS_H

#include <string>
#include <vector>

#include "raylib.h"

enum LoadingError {
    UnknownError,
    NetworkError,
    NotConnectedError,
    JSONError,
    SDError,
    AppletError
};

void DrawTextOutlineEx(Font font, const char* text, Vector2 position, Vector2 origin, float fontSize, float spacing, Color textColor, Color outlineColor, int thickness);
float GetMappedAxis(float raw_val, float max_out, float deadzone = 0.1f);
extern bool showFakePackages;
extern bool shouldQuit;
extern std::vector<Texture2D> backgrounds;
extern bool alreadyLoggedIn;
extern bool disableSavingToSD;
extern bool networkTested;
extern bool skipFlashbang;
#endif //SWITCHPOST_HELPERS_H
