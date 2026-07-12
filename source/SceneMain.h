#ifndef SWITCHPOST_SCENEMAIN_H
#define SWITCHPOST_SCENEMAIN_H

#include "Scene.h"
#include "raylib.h"
#include "tweeny.h"
#include "InPostAPI.h"
#include <string>
#include "Helpers.h"
#include <switch.h>

class SceneMain : public Scene {
public:
    void SceneInit() override;
    void SceneDraw() override;
    void SceneUpdate(float dt) override;
    void SceneExit() override;
    std::string SceneIdentify() override { return "main"; }
private:
    void ResetRemoteLockerData();
    void ReloadScene();
    void RenderRemoteOpenButton();

    // resources
    Texture2D GenerateQrTexture(const char* qrData);
    void GenerateSenderNameRenderTexture();
    RenderTexture2D senderName, openButton;
    Texture2D poststamp, package, loadingCircle, promptY, promptX, promptPlus, selectorCorner, reloadButton, archiveButton, delivered, readyForPickup, renameButton;
    Font mainFont;
    Sound confirmOpen, confirmClosed;
    int textAreaWidth = 404;

    // tweens
    tweeny::tween<float> poststampFade, packagesFade, sceneChangeFade, detailsFade, detailsScrollUp, selectorFadePulse, modeChangeAnim;
    float textScrollAnimDelay = 5.0f;
    float textScrollAnim = 0;
    bool textScrollDirection = true;

    // state
    bool inDetails, inQR, inOpenPaczkomat, inConfirmClosed, stickMoved, screenTouched, playModeChangeAnim;
    std::string sessionUuid;
    float scrollOffset = 0;
    RenderTexture2D packageDetails;
    Texture2D qrCode;
    std::vector<Package>* currentDisplay;
    bool offlineMode;

    // input
    int selectedPackage;
    float cameraOffset = 0;
    float targetOffset = 0;
    bool useTouch;
    Vector2 touchStartPos, previousTouch;
    bool isDragging;
    bool isTouching;
    const float dragThreshold = 10.0f;

    float spinnerRotation = 0;
    bool inputLock;

    bool tokensLoaded, loadingDone;

    // keyboard input
    Result rc;
    char parcelName[20];
    bool askForParcelName;
    SwkbdConfig kbd;
};

#endif //SWITCHPOST_SCENEMAIN_H