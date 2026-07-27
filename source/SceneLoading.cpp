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
}

void SceneLoading::SceneUpdate(float dt) {
    spinnerRotation += 180 * dt;
    if (showFakePackages) {
        std::ifstream fakePackages("romfs:/text/test_data.json");
        if (!fakePackages.is_open()) {
            SceneManager::ChangeScene(std::make_unique<SceneError>(JSONError, true));
            return;
        }
        std::stringstream buffer;
        buffer << fakePackages.rdbuf();

        if (!nlohmann::json::accept(buffer.str())) {
            SPDLOG_ERROR("failed to load fake packages");
            SPDLOG_DEBUG("{}", buffer.str());
            SceneManager::ChangeScene(std::make_unique<SceneError>(JSONError, true));
            return;
        }

        if (!InPostAPI::ParsePaczkas(buffer.str())) {
            SceneManager::ChangeScene(std::make_unique<SceneError>(JSONError, true));
        } else {
            if (!alreadyLoggedIn) {
                SPDLOG_INFO("Good evening professor. I see you have driven here in your Ferrari.");
                alreadyLoggedIn = true;
            }
            SceneManager::ChangeScene(std::make_unique<SceneMain>());
        }
        fakePackages.close();
    } else {
        if (IsConnected()) {
            if (InPostAPI::getPaczkasBuffer.status == NotStarted) {
                InPostAPI::GetPaczkas();
            } else if (InPostAPI::getPaczkasBuffer.status == Done) {
                if (InPostAPI::getPaczkasBuffer.code == 200) {
                    if (!InPostAPI::ParsePaczkas(std::string(InPostAPI::getPaczkasBuffer.data.begin(), InPostAPI::getPaczkasBuffer.data.end()))) {
                        SceneManager::ChangeScene(std::make_unique<SceneError>(JSONError, true));
                        return;
                    }

                    std::ofstream file("sdmc:/config/switchpost/offline.json");
                    if (!file.is_open()) {
                        SceneManager::ChangeScene(std::make_unique<SceneError>(SDError));
                        return;
                    }

                    file << std::string(InPostAPI::getPaczkasBuffer.data.begin(), InPostAPI::getPaczkasBuffer.data.end());
                    file.close();
                    if (!alreadyLoggedIn) {
                        SPDLOG_INFO("Good evening professor. I see you have driven here in your Ferrari.");
                        alreadyLoggedIn = true;
                    }
                    SceneManager::ChangeScene(std::make_unique<SceneMain>());
                } else if (InPostAPI::getPaczkasBuffer.code == 304) {
                    if (!alreadyLoggedIn) {
                        SPDLOG_INFO("Good evening professor. I see you have driven here in your Ferrari.");
                        alreadyLoggedIn = true;
                    }
                    SceneManager::ChangeScene(std::make_unique<SceneMain>());
                } else {
                    SceneManager::ChangeScene(std::make_unique<SceneError>(NetworkError, true));
                }
            } else if (InPostAPI::getPaczkasBuffer.status == Error) {
                SceneManager::ChangeScene(std::make_unique<SceneError>(NetworkError, true));
            }
        } else {
            std::ifstream offlinePackages("sdmc:/config/switchpost/offline.json");
            if (!offlinePackages.is_open()) {
                SceneManager::ChangeScene(std::make_unique<SceneError>(NotConnectedError));
                return;
            }

            std::stringstream buffer;
            buffer << offlinePackages.rdbuf();
            if (!nlohmann::json::accept(buffer.str())) {
                SPDLOG_ERROR("failed to load fake packages");
                SPDLOG_DEBUG("{}", buffer.str());
                SceneManager::ChangeScene(std::make_unique<SceneError>(JSONError, true));
                return;
            }

            if (!InPostAPI::ParsePaczkas(buffer.str())) {
                SceneManager::ChangeScene(std::make_unique<SceneError>(JSONError, true));
            } else {
                SceneManager::ChangeScene(std::make_unique<SceneMain>());
            }
            offlinePackages.close();
        }
    }
}

void SceneLoading::SceneDraw() {
    DrawRectangleGradientV(0, 0, 1320, 720, BLACK, {10, 10, 10, 255});
    Rectangle source = { 0.0f, 0.0f, (float)loadingCircle.width, (float)loadingCircle.height };
    Rectangle dest = { 1197, 637, (float)loadingCircle.width/2, (float)loadingCircle.height/2};
    Vector2 origin = { (float)loadingCircle.width/4.0f, (float)loadingCircle.height/4.0f};
    DrawTexturePro(loadingCircle, source, { dest.x, dest.y, dest.width, dest.height}, origin, spinnerRotation, {255, 255, 255, 100});
}

void SceneLoading::SceneExit() {
    UnloadTexture(loadingCircle);
}
