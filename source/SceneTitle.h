#ifndef SWITCHPOST_SCENETITLE_H
#define SWITCHPOST_SCENETITLE_H

#include "Scene.h"
#include "raylib.h"
#include "tweeny.h"
#include <string>
#include <vector>

class SceneTitle : public Scene {
public:
    void SceneInit() override;
    void SceneDraw() override;
    void SceneUpdate(float dt) override;
    void SceneExit() override;
private:
    Font mainFont;
    Font smallFont;
    std::string version = "";
#ifdef DEBUG
    std::string bigVersion = "DEBUG";
    Color versionColor = {81, 55, 138, 255};
#endif
    Texture2D logo;
    RenderTexture2D logoRender;
    Sound collectedAll, change, confirmTutorial;
    Camera3D camera;
    tweeny::tween<float> transititon;
    tweeny::tween<float> rotationAnim;
    bool isTransitioning; // :troll:
    bool inputLock, firstTimeUsingPrompt, askForTutorial, stickMoved;
    float transitionTimer = 0;
    std::vector<std::string> options;
    int selectedOption;
};

#endif //SWITCHPOST_SCENETITLE_H
