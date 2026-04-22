#ifndef SWITCHPOST_SCENEOPTIONS_H
#define SWITCHPOST_SCENEOPTIONS_H

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
    Font mainFont, promptFont;
    int selectedOption, selectedSubOption;
    Sound change, done;
    bool inputLock;

    std::vector<std::string> options, voices;
    std::vector<ResourcePack> packList;

    std::string voice, currentResourcePack, oldPack;

    bool inResourcePackOptions, inVoiceOptions, inDeleteData, stickMovedY, stickMovedX, inNoVoicePopup;

    float scrollOffset, drawOffset, targetOffset;
};

#endif //SWITCHPOST_SCENEOPTIONS_H