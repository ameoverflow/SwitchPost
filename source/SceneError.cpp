//
// Created by void on 25/05/2026.
//

#include "SceneError.h"

#include "SceneIntro.h"
#include "SceneManager.h"
#include "tweeny.h"
#include "spdlog/spdlog.h"
#include <switch.h>

#include "i18n.h"
#include "MusicManager.h"
#include "SceneLoading.h"

void SceneError::SceneInit() {
    mainFont = LoadFontEx("romfs:/fonts/Ubuntu-Regular.ttf", 90, 0, 381);
    errorBgFade = tweeny::from(32).to(64).during(2000).via(tweeny::easing::sinusoidalInOut);
    errorDesc = i18n::GetString("error.title");
    switch (errorCode) {
        case NetworkError:
            errorDesc += i18n::GetString("error.network");
            break;
        case NotConnectedError:
            errorDesc += i18n::GetString("error.not_connected");
            break;
        case SDError:
            errorDesc += i18n::GetString("error.sd");
            break;
        case AppletError:
            errorDesc += i18n::GetString("error.applet");
            break;
        case JSONError:
            errorDesc = i18n::GetString("error.json");
            break;
    }

    if (errorCode == NotConnectedError) {
        Result rc = nifmInitialize(NifmServiceType_User);
        if (R_FAILED(rc)) {
            SPDLOG_CRITICAL("failed to initialize nifm");
        }
    }
}

void SceneError::SceneUpdate(float dt) {
    errorBgFade.step((int)(dt * 1000.0f));
    if (errorBgFade.progress() >= 1.0f && errorBgFade.direction() == 1) {
        errorBgFade.backward();
    } else if (errorBgFade.progress() <= 0.0f && errorBgFade.direction() == -1) {
        errorBgFade.forward();
    }

    if (errorCode == NotConnectedError) {
        if (networkCheckTimeout <= 0.0f) {
            rc = nifmGetInternetConnectionStatus(&type, &wifi_strength, &status);

            if (R_SUCCEEDED(rc)) {
                if (status == NifmInternetConnectionStatus_Connected) {
                    networkTested = false;
                    SceneManager::ChangeScene(std::make_unique<SceneIntro>());
                }
            }

            networkCheckTimeout = 2.0f;
        } else {
            networkCheckTimeout -= dt;
        }
    }
}

void SceneError::SceneDraw() {
    DrawRectangleGradientV(0, 0, 1280, 720, BLACK, {errorBgFade.peek(),0,0,255});
    Vector2 textSize = MeasureTextEx(mainFont, errorDesc.c_str(), 34, 0);
    DrawTextOutlineEx(mainFont, errorDesc.c_str(), {1280/2, 720/2},
                      {textSize.x / 2.0f, textSize.y / 2.0f}, 34, 0, RED, BLACK, 2);
}

void SceneError::SceneExit() {
    if (errorCode == NotConnectedError) {
        nifmExit();
    }
    UnloadFont(mainFont);
}
