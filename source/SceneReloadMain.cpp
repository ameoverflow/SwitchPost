//
// Created by void on 16/04/2026.
//

#include "SceneReloadMain.h"
#include "InpostAPI.h"
#include "raylib.h"
#include "SceneMain.h"
#include "SceneManager.h"

void SceneReloadMain::SceneInit() {

}

void SceneReloadMain::SceneUpdate(float dt) {
    InpostAPI::packages.clear();
    InpostAPI::getPaczkasBuffer->data.clear();
    InpostAPI::getPaczkasBuffer->status = NotStarted;
    InpostAPI::getPaczkasBuffer->code = 0;
    InpostAPI::authToken = "";
    InpostAPI::refreshToken = "";
    SceneManager::ChangeScene(std::make_unique<SceneMain>());
}

void SceneReloadMain::SceneDraw() {
    DrawRectangleGradientV(0, 0, 1320, 720, BLACK, {10, 10, 10, 255});
}

void SceneReloadMain::SceneExit() {

}
