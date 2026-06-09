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
    void SceneInit() override;
    void SceneDraw() override;
    void SceneUpdate(float dt) override;
    void SceneExit() override;
private:
    void HandleSelectMenu(size_t listSize, bool &activeMenu, std::function<void()> onConfirm);

    Font mainFont, promptFont;
    int selectedOption, selectedSubOption;
    Sound change, done;
    bool inputLock;

    std::vector<std::string> options, voices, languages;
    std::vector<ResourcePack> packList;

    std::string oldPack, buildInfo;

    bool inResourcePackOptions, inVoiceOptions, inDeleteData, stickMovedY, stickMovedX, inNoVoicePopup, inBuildPopup, inLanguageOptions;

    float scrollOffset, drawOffset, targetOffset;
};

#endif //SWITCHPOST_SCENEOPTIONS_H