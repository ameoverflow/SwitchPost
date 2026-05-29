//
// Created by void on 25/05/2026.
//

#include "SceneError.h"

#include <chrono>
#include "tweeny.h"

void SceneError::SceneInit() {
    mainFont = LoadFontEx("romfs:/fonts/Ubuntu-Regular.ttf", 90, 0, 381);
    errorBgFade = tweeny::from(32).to(64).during(2000).via(tweeny::easing::sinusoidalInOut);
    errorDesc = "Wystąpił błąd:\n";
    switch (errorCode) {
        case NetworkError:
            errorDesc += "Błąd sieci, sprawdź połączenie z internetem";
            break;
        case SDError:
            errorDesc += "Błąd zapisywania konfiguracji";
            break;
        case AppletError:
            errorDesc += "Aplikacja uruchomiona w trybie apletu";
            break;
        case JSONError:
            errorDesc = "Błąd danych";
            break;
    }
}

void SceneError::SceneUpdate(float dt) {
    errorBgFade.step((int)(dt * 1000.0f));
    if (errorBgFade.progress() >= 1.0f && errorBgFade.direction() == 1) {
        errorBgFade.backward();
    } else if (errorBgFade.progress() <= 0.0f && errorBgFade.direction() == -1) {
        errorBgFade.forward();
    }
}

void SceneError::SceneDraw() {
    DrawRectangleGradientV(0, 0, 1280, 720, BLACK, {errorBgFade.peek(),0,0,255});
    Vector2 textSize = MeasureTextEx(mainFont, errorDesc.c_str(), 34, 0);
    DrawTextOutlineEx(mainFont, errorDesc.c_str(), {1280/2, 720/2},
                      {textSize.x / 2.0f, textSize.y / 2.0f}, 34, 0, RED, BLACK, 2);
}

void SceneError::SceneExit() {
    UnloadFont(mainFont);
}
