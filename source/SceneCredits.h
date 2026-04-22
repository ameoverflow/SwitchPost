//
// Created by void on 25/03/2026.
//

#ifndef SWITCHPOST_SCENECREDITS_H
#define SWITCHPOST_SCENECREDITS_H

#include "Scene.h"
#include <string>
#include "raylib.h"
#include <vector>

class SceneCredits : public Scene {
public:
    void SceneInit() override;
    void SceneDraw() override;
    void SceneUpdate(float dt) override;
    void SceneExit() override;
private:
    Texture2D logoBg;
    Font mainFont;
    std::vector<std::string> credits;
    float scroll = 720;
    float lineOffset = 0;
    float lineHeight;
    bool inputLock;
};


#endif //SWITCHPOST_SCENECREDITS_H
