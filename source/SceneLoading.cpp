//
// Created by void on 29/05/2026.
//

#include "SceneLoading.h"

#include "AssetLoader.h"
#include "Config.h"
#include "Helpers.h"
#include "InPostAPI.h"
#include "SceneError.h"
#include "SceneMain.h"
#include "SceneManager.h"
#include "spdlog/spdlog.h"

void SceneLoading::SceneInit() {
    InPostAPI::getPaczkasBuffer.data.clear();
    InPostAPI::getPaczkasBuffer.status = NotStarted;
    InPostAPI::getPaczkasBuffer.code = 0;

    loadingCircle = LoadTexture(AssetLoader::ResolveResource("sprites/loading_circle.png").c_str());

    // fade in from loading screen
    loadingFade = tweeny::from(1.0f).to(0.0f).during(100);
    loadingFade.seek(0);
}

void SceneLoading::SceneUpdate(float dt) {
    spinnerRotation += 180 * dt;
    if (showFakePackages) {
        std::ifstream fakePackages("romfs:/text/test_data.json");
        if (fakePackages.is_open()) {
            std::stringstream buffer;
            buffer << fakePackages.rdbuf();
            if (nlohmann::json::accept(buffer.str())) {
                if (!InPostAPI::ParsePaczkas(buffer.str())) {
                    SceneManager::ChangeScene(std::make_unique<SceneError>(JSONError));
                } else {
                    SceneManager::ChangeScene(std::make_unique<SceneMain>());
                }
            } else {
                SPDLOG_ERROR("failed to load fake packages");
                SPDLOG_DEBUG("{}", buffer.str());
                SceneManager::ChangeScene(std::make_unique<SceneError>(JSONError));
                return;
            }
        } else {
            SceneManager::ChangeScene(std::make_unique<SceneError>(JSONError));
            return;
        }
        fakePackages.close();
    } else {
        if (InPostAPI::getPaczkasBuffer.status == NotStarted) {
            InPostAPI::GetPaczkas();
        } else if (InPostAPI::getPaczkasBuffer.status == Done) {
            if (InPostAPI::getPaczkasBuffer.code == 200) {
                if (InPostAPI::ParsePaczkas(std::string(InPostAPI::getPaczkasBuffer.data.begin(), InPostAPI::getPaczkasBuffer.data.end()))) {
                    SceneManager::ChangeScene(std::make_unique<SceneMain>());
                } else {
                    SceneManager::ChangeScene(std::make_unique<SceneError>(JSONError));
                }
            } else if (InPostAPI::getPaczkasBuffer.code == 304) {
                SceneManager::ChangeScene(std::make_unique<SceneMain>());
            } else {
                SceneManager::ChangeScene(std::make_unique<SceneError>(NetworkError));
            }
        } else if (InPostAPI::getPaczkasBuffer.status == Error) {
            SceneManager::ChangeScene(std::make_unique<SceneError>(NetworkError));
        }
    }
}

void SceneLoading::SceneDraw() {
    if (loadingFade.progress() == 0.0f) {
        DrawRectangleGradientV(0, 0, 1320, 720, ColorAlpha(BLACK, loadingFade.peek()), ColorAlpha({10, 10, 10, 255}, loadingFade.peek()));
        Rectangle source = { 0.0f, 0.0f, (float)loadingCircle.width, (float)loadingCircle.height };
        Rectangle dest = { 1197, 637, (float)loadingCircle.width/2, (float)loadingCircle.height/2};
        Vector2 origin = { (float)loadingCircle.width/4.0f, (float)loadingCircle.height/4.0f};
        DrawTexturePro(loadingCircle, source, { dest.x, dest.y, dest.width, dest.height}, origin, spinnerRotation, {255, 255, 255, 100});

    } else if (loadingFade.progress() > 0.0f) {
        DrawRectangleGradientV(0, 0, 1320, 720, ColorAlpha(BLACK, loadingFade.peek()), ColorAlpha({10, 10, 10, 255}, loadingFade.peek()));
    }
}

void SceneLoading::SceneExit() {
    UnloadTexture(loadingCircle);
    if (!alreadyLoggedIn) {
        SPDLOG_INFO("Good evening professor. I see you have driven here in your Ferrari.");
        alreadyLoggedIn = true;
    }
}
