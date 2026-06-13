//
// Created by void on 29/05/2026.
//

#ifndef SWITCHPOST_SCENELOADING_H
#define SWITCHPOST_SCENELOADING_H

#include "raylib.h"
#include "Scene.h"

class SceneLoading : public Scene {
public:
    void SceneInit() override;
    void SceneDraw() override;
    void SceneUpdate(float dt) override;
    void SceneExit() override;
    std::string SceneIdentify() override { return "loading"; }
private:
    float spinnerRotation = 0;
    Texture2D loadingCircle;
};


#endif //SWITCHPOST_SCENELOADING_H
