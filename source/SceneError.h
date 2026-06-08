//
// Created by void on 25/05/2026.
//

#ifndef SWITCHPOST_SCENEERROR_H
#define SWITCHPOST_SCENEERROR_H
#include <switch.h>
#include "Helpers.h"
#include "Scene.h"
#include "tween.h"

#endif //SWITCHPOST_SCENEERROR_H

class SceneError : public Scene {
public:
    SceneError(LoadingError error = UnknownError) {
        errorCode = error;
    }
    void SceneInit() override;
    void SceneDraw() override;
    void SceneUpdate(float dt) override;
    void SceneExit() override;
private:
    LoadingError errorCode;
    tweeny::tween<int> errorBgFade;
    Font mainFont;
    std::string errorDesc;
    Result rc;
    NifmInternetConnectionType type;
    u32 wifi_strength;
    NifmInternetConnectionStatus status;
    float networkCheckTimeout;
};