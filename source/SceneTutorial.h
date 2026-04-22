//
// Created by void on 09/04/2026.
//

#ifndef SWITCHPOST_SCENETUTORIAL_H
#define SWITCHPOST_SCENETUTORIAL_H

#include "Scene.h"
#include <string>
#include <vector>
#include "raylib.h"
#include "tweeny.h"

struct TutorialFrame {
    std::string speakingSprite, idleSprite, voiceClip, text, background;
};

class SceneTutorial : public Scene {
public:
    SceneTutorial(bool fromOptions = false) {
        comingFromOptions = fromOptions;
    }
    void SceneInit() override;
    void SceneDraw() override;
    void SceneUpdate(float dt) override;
    void SceneExit() override;
private:
    std::vector<TutorialFrame> Frames;
    tweeny::tween<float> characterAnim;
    Texture2D textbox, speakingSprite, idleSprite, background;
    Sound voiceClip;
    std::vector<std::string> lines;
    int lineHeight;
    Font mainFont;
    int currentFrame;
    bool playCharacterAnim, comingFromOptions;

    std::string tut;
};


#endif //SWITCHPOST_SCENETUTORIAL_H
