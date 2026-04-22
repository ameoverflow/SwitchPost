#ifndef SWITCHPOST_SCENEMAIN_H
#define SWITCHPOST_SCENEMAIN_H

#include "Scene.h"
#include "raylib.h"
#include "tweeny.h"
#include "InpostAPI.h"
#include <string>
#include "Helpers.h"

class SceneMain : public Scene {
public:
    void SceneInit() override;
    void SceneDraw() override;
    void SceneUpdate(float dt) override;
    void SceneExit() override;
private:
    void ResetRemoteLockerData();
    void ReloadScene();

    Texture2D GenerateQrTexture(const char* qrData);
    Texture2D poststamp, package, loadingCircle, promptY, promptX, selectorCorner, openButton, reloadButton, delivered, readyForPickup;
    Font mainFont;
    Sound change, confirmOpen, confirmClosed;

    tweeny::tween<float> poststampFade, packagesFade, loadingFade, sceneChangeFade, detailsFade, detailsScrollUp, selectorFadePulse;

    bool inDetails, inQR, inOpenPaczkomat, inConfirmClosed, stickMoved, screenTouched;
    std::string sessionUuid;
    float scrollOffset = 0;
    RenderTexture2D packageDetails;
    Texture2D qrCode;

    int selectedPackage;
    float cameraOffset = 0;
    float targetOffset = 0;

    float spinnerRotation = 0;
    bool isLoaded = false;
    bool inputLock;

    std::string errorDesc;
    enum LoadingError errorCode;
    bool tokensLoaded;
};

#endif //SWITCHPOST_SCENEMAIN_H