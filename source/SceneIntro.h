#ifndef SWITCHPOST_SCENEINIT_H
#define SWITCHPOST_SCENEINIT_H

#include "Scene.h"
#include "raylib.h"
#include <string>
#include "tweeny.h"
#include <switch.h>

#include "Helpers.h"

class SceneIntro : public Scene {
public:
    void SceneInit() override;
    void SceneDraw() override;
    void SceneUpdate(float dt) override;
    void SceneExit() override;
private:
    Texture2D pakuj, logo;
    Font logoFont, mainFont;
    Sound introSound;

    tweeny::tween<float> logoFadeIn;
    tweeny::tween<int> errorBgFade, ameLogoFadeIn;

    int randomNum = 0;
    float introTimer = 0.0f;
    int introStage = 0;
    float logoAlpha = 0.0f;
    LoadingError error;

    std::string text = "ameOverflow";
    std::string line = "missingno.";
    std::string voice, errorCode, status;

    AppletType at;

    Result rc;
    char phoneNumber[10];
    char code[7];
    SwkbdConfig kbd;
};


#endif //SWITCHPOST_SCENEINIT_H