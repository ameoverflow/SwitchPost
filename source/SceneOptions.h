#ifndef SWITCHPOST_SCENEOPTIONS_H
#define SWITCHPOST_SCENEOPTIONS_H

#include <functional>
#include "Scene.h"
#include "raylib.h"
#include <string>
#include <vector>
#include "AssetLoader.h"

class SceneOptions : public Scene {
public:
    SceneOptions(bool returnToMain = false) {
        returnToMainScene = returnToMain;
    }
    void SceneInit() override;
    void SceneDraw() override;
    void SceneUpdate(float dt) override;
    void SceneExit() override;
    std::string SceneIdentify() override { return "options"; }
private:
    void HandleSelectMenu(size_t listSize, bool &activeMenu, std::function<void()> onConfirm);

    Font mainFont, promptFont;
    int selectedOption = 0;
    int selectedSubOption = 0;
    bool inputLock = false;
    bool returnToMainScene;

    std::vector<std::string> options, voices, languages;
    std::vector<ResourcePack> packList;

    std::string oldPack, buildInfo;

    bool inResourcePackOptions = false;
    bool inVoiceOptions = false;
    bool inDeleteData = false;
    bool stickMovedY = false;
    bool stickMovedX = false;
    bool inNoVoicePopup = false;
    bool inBuildPopup = false;
    bool inLanguageOptions = false;

    float scrollOffset, drawOffset, targetOffset;
};

#endif //SWITCHPOST_SCENEOPTIONS_H