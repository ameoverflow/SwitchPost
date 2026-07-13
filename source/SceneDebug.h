#ifndef SWITCHPOST_SCENEDEBUG_H
#define SWITCHPOST_SCENEDEBUG_H

#include "Scene.h"
#include "raylib.h"
#include <string>
#include <vector>
#include <switch.h>

class SceneDebug : public Scene {
public:
    void SceneInit() override;
    void SceneDraw() override;
    void SceneUpdate(float dt) override;
    void SceneExit() override;
    std::string SceneIdentify() override { return "debug"; }
private:
    Font mainFont;
    int selectedOption;
    Texture2D pakuj;
    bool inputLock, pakujOn;
    std::vector<std::string> options;
    Result rc;
    char baseUrl[64];
    SwkbdConfig kbd;
    bool askingForUrl;
};

#endif //SWITCHPOST_SCENEDEBUG_H