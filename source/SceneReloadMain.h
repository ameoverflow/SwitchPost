//
// Created by void on 16/04/2026.
//

#ifndef SWITCHPOST_SCENERELOADMAIN_H
#define SWITCHPOST_SCENERELOADMAIN_H
#include "Scene.h"


class SceneReloadMain : public Scene {
public:
    void SceneInit() override;
    void SceneDraw() override;
    void SceneUpdate(float dt) override;
    void SceneExit() override;
};



#endif //SWITCHPOST_SCENERELOADMAIN_H
