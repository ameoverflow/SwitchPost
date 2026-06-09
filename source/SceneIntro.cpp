#include "SceneIntro.h"

#include <string>
#include "raylib.h"
#include <cmath>
#include <iostream>
#include <vector>
#include <ctime>
#include <filesystem>
#include <fstream>
#include "SceneManager.h"
#include "SceneTitle.h"
#include "spdlog/spdlog.h"
#include "MusicManager.h"
#include "Request.h"
#include "curl/curl.h"
#include "Helpers.h"
#include <switch.h>
#include "AssetLoader.h"
#include "InPostAPI.h"
#include "json.hpp"
#include "Config.h"
#include "i18n.h"
#include "SceneDebug.h"
#include "SceneError.h"

ResponseBuffer bufferPointer;

bool SceneIntro::IsConnected() {
    Result rc = nifmInitialize(NifmServiceType_User);
    if (R_FAILED(rc)) {
        SPDLOG_CRITICAL("failed to initialize nifm");
        return false;
    }

    NifmInternetConnectionType type;
    u32 wifi_strength;
    NifmInternetConnectionStatus status;

    rc = nifmGetInternetConnectionStatus(&type, &wifi_strength, &status);

    nifmExit();

    if (R_SUCCEEDED(rc)) {
        if (status == NifmInternetConnectionStatus_Connected) {
            return true;
        }
    }

    return false;
}

void SceneIntro::SceneInit() {
    pakuj = LoadTexture(AssetLoader::ResolveResource("sprites/pakuj.png").c_str());
    logo = LoadTexture(AssetLoader::ResolveResource("sprites/logo.png").c_str());
    introSound = LoadSound(AssetLoader::ResolveResource("sounds/intro.wav").c_str());
    introLogo = LoadTexture("romfs:/sprites/intro_logo.png");
    introTimer = 0;
    mainFont = LoadFontEx("romfs:/fonts/Ubuntu-Regular.ttf", 90, 0, 381);
    logoFont = LoadFontEx("romfs:/fonts/ComicHelvetic_Heavy.otf", 90, 0, 381);
    introStage = 0;
    at = appletGetAppletType();
    voice = Config::openedFile.voice;
    language = Config::openedFile.language;

    logoFadeIn = tweeny::from(0.0f).to(0.8f).during(644);
    ameLogoFadeIn = tweeny::from(0.0f).to(1.0f).during(250).via(tweeny::easing::backOut);
    logoFadeIn.seek(0);
    ameLogoFadeIn.seek(0);

    std::ifstream splashesFile(AssetLoader::ResolveResource("text/splashes.txt").c_str());

    while (std::getline(splashesFile, line)) {
        SPDLOG_TRACE("splash added: {}", line);
        splashes.push_back(line);
    }
    splashesFile.close();

    std::srand(std::time(NULL));
    if (std::size(splashes) > 0) {
        int randomIndex = std::rand() % std::size(splashes);
        line = splashes[randomIndex];
    }

    randomNum = std::rand() % 500;
    SPDLOG_DEBUG("random number choosen is {}", randomNum);
}

void SceneIntro::SceneUpdate(float dt) {
    if (randomNum != 67) {
        // check for debug combination
#ifdef DEBUG
        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) {
            SceneManager::ChangeScene(std::make_unique<SceneDebug>());
        }
#endif
        if (introStage == 0) {
            // initialize shit
            // check if its not applet mode
            if (at != AppletType_Application && at != AppletType_SystemApplication) {
                SPDLOG_CRITICAL("application running as applet");
                SceneManager::ChangeScene(std::make_unique<SceneError>(AppletError));
                return;
            }

            if (!checkedNetwork) {
                if (!IsConnected()) {
                    SPDLOG_CRITICAL("not connected");
                    SceneManager::ChangeScene(std::make_unique<SceneError>(NotConnectedError));
                    return;
                }
                checkedNetwork = true;
            } else {
                if (!loadedTokens) {
                    if (std::filesystem::exists("sdmc:/config/switchpost/token.json") && !InPostAPI::LoadTokens()) {
                        SPDLOG_CRITICAL("unable to load tokens");
                        SceneManager::ChangeScene(std::make_unique<SceneError>(JSONError));
                        return;
                    }
                    loadedTokens = true;
                } else {
                    introStage = 1;
                }
            }

        } else if (introStage == 1) {
            ameLogoFadeIn.step((int)(dt * 1000.0f));
            if (ameLogoFadeIn.progress() >= 1.0f) {
                introTimer = 0;
                introStage = 2;
                PlaySound(introSound);
            }
        } else if (introStage == 2) {
            introTimer += dt;
            if (introTimer > 2) {
                introTimer = 0;
                introStage = 3;
            }
        } else if (introStage == 3) {
            SPDLOG_DEBUG("language is {}", language);
            if (language != "") {
                introStage = 4;
                return;
            }

            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) ||
            IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
                    Config::openedFile.language = "en";
                } else {
                    Config::openedFile.language = "pl";
                }
                i18n::SetLanguage(Config::openedFile.language);
                SPDLOG_TRACE("language set to {}", Config::openedFile.language);
                introStage = 4;
            }
        } else if (introStage == 4) {
            // log in
            if (!std::filesystem::exists("sdmc:/config/switchpost/token.json")) {
                if (InPostAPI::sendSMSCodeBuffer.status == NotStarted) {
                    while (true) {
                        swkbdCreate(&kbd, 0);
                        swkbdConfigSetType(&kbd, SwkbdType_NumPad);
                        swkbdConfigSetStringLenMax(&kbd, 9);
                        swkbdConfigSetStringLenMin(&kbd, 9);
                        swkbdConfigSetHeaderText(&kbd, i18n::GetString("intro.phone").c_str());
                        swkbdConfigSetGuideText(&kbd, "600100100");

                        rc = swkbdShow(&kbd, phoneNumber, sizeof(phoneNumber));
                        swkbdClose(&kbd);

                        if (R_SUCCEEDED(rc)) break;
                    }

                    InPostAPI::SendSMSCode(std::string(phoneNumber));
                }

                if (InPostAPI::sendSMSCodeBuffer.status == Done &&
                    InPostAPI::verifySMSCodeBuffer.status == NotStarted) {
                    while (true) {
                        swkbdCreate(&kbd, 0);
                        swkbdConfigSetType(&kbd, SwkbdType_NumPad);
                        swkbdConfigSetStringLenMax(&kbd, 6);
                        swkbdConfigSetStringLenMin(&kbd, 6);
                        swkbdConfigSetHeaderText(&kbd, i18n::GetString("intro.sms").c_str());
                        swkbdConfigSetGuideText(&kbd, "123456");

                        rc = swkbdShow(&kbd, code, sizeof(code));
                        swkbdClose(&kbd);

                        if (R_SUCCEEDED(rc)) break;
                    }

                    InPostAPI::VerifySMSCode(std::string(phoneNumber), std::string(code));
                } else if (InPostAPI::sendSMSCodeBuffer.status == Error ||
                           (InPostAPI::sendSMSCodeBuffer.status == Done &&
                            InPostAPI::sendSMSCodeBuffer.code != 200)) {
                    SPDLOG_CRITICAL("network error, curl code is {}, http code is {}", std::to_string(bufferPointer.result), std::to_string(InPostAPI::sendSMSCodeBuffer.code));
                    SceneManager::ChangeScene(std::make_unique<SceneError>(NetworkError));
                    return;
                }

                if (InPostAPI::verifySMSCodeBuffer.status == Done && InPostAPI::verifySMSCodeBuffer.code == 200) {
                    std::string loginData(InPostAPI::verifySMSCodeBuffer.data.begin(),
                                          InPostAPI::verifySMSCodeBuffer.data.end());
                    if (nlohmann::json::accept(loginData)) {
                        nlohmann::json data = nlohmann::json::parse(loginData);
                        std::string authToken = data.value("authToken", "");
                        std::string refreshToken = data.value("refreshToken", "");

                        if (!authToken.empty() && !refreshToken.empty()) {
                            if (!disableSavingToSD) {
                                std::ofstream file("sdmc:/config/switchpost/token.json");
                                if (file.is_open()) {
                                    file << loginData;
                                    file.close();
                                    SPDLOG_INFO("login data saved to SD");
                                } else {
                                    SPDLOG_ERROR("couldnt open token.json for writing");
                                    SceneManager::ChangeScene(std::make_unique<SceneError>(SDError));
                                }
                            } else {
                                SPDLOG_DEBUG("saving to sd disabled");
                            }
                        } else {
                            SceneManager::ChangeScene(std::make_unique<SceneError>(SDError));
                        }
                    } else {
                        SceneManager::ChangeScene(std::make_unique<SceneError>(SDError));
                    }
                } else if (InPostAPI::verifySMSCodeBuffer.status == Error ||
                           (InPostAPI::verifySMSCodeBuffer.status == Done &&
                            InPostAPI::verifySMSCodeBuffer.code != 200)) {
                    SceneManager::ChangeScene(std::make_unique<SceneError>(NetworkError));
                    SPDLOG_CRITICAL("network error, curl code is {}, http code is {}", std::to_string(bufferPointer.result), std::to_string(InPostAPI::sendSMSCodeBuffer.code));
                }
            } else {
                introStage = 5;
            }
        } else if (introStage == 5) {
            if (voice != "") {
                introStage = 999;
                MusicManager::PlayMusic("music/menu_music.ogg");
                return;
            }

            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) ||
            IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP) ||
            IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
                    Config::openedFile.voice = "male";
                } else if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP)) {
                    Config::openedFile.voice = "female";
                } else {
                    Config::openedFile.voice = "none";
                }
                SPDLOG_TRACE("voice set to {}", Config::openedFile.voice);
                MusicManager::PlayMusic("music/menu_music.ogg");
                introStage = 999;
            }
        } else if (introStage == 999) {
            logoFadeIn.step((int)(dt * 1000));
            if (logoFadeIn.progress() >= 1)
                SceneManager::ChangeScene(std::make_unique<SceneTitle>());
        }
    }
}

void SceneIntro::SceneDraw() {
    if (randomNum != 67) {
        DrawRectangleGradientV(0, 0, 1280, 720, BLACK, {10,10,10,255});

        if (introStage == 1 || introStage == 2) {
            DrawTexturePro(introLogo, {0, 0, introLogo.width, introLogo.height},
                {GetScreenWidth()/2, GetScreenHeight()/2, introLogo.width * ameLogoFadeIn.peek(), introLogo.height * ameLogoFadeIn.peek()},
                {(introLogo.width * ameLogoFadeIn.peek())/2, (introLogo.height * ameLogoFadeIn.peek())/2}, 0, WHITE);

            Vector2 textSize = MeasureTextEx(mainFont, line.c_str(), 34, 0);
            DrawTextPro(mainFont, line.c_str(), {1280/2, 720/2 + 250},
                        {textSize.x / 2.0f, textSize.y / 2.0f}, 0, 34, 0, WHITE);
        } else if (introStage == 3) {
            if (language != "") return;
            Vector2 textSize = MeasureTextEx(mainFont, "Select language\nWybierz język\n\n(A) English\n(B) Polski", 34, 0);
            DrawTextPro(mainFont, "Select language\nWybierz język\n\n(A) English\n(B) Polski",
                        {GetScreenWidth()/2, GetScreenHeight()/2}, {textSize.x/2, textSize.y/2}, 0, 34, 0, WHITE);
        } else if (introStage == 5) {
            if (voice != "") return;
            Vector2 textSize = MeasureTextEx(mainFont, i18n::GetString("intro.voice_select").c_str(), 34, 0);
            DrawTextPro(mainFont, i18n::GetString("intro.voice_select").c_str(),
                        {GetScreenWidth()/2, GetScreenHeight()/2}, {textSize.x/2, textSize.y/2}, 0, 34, 0, WHITE);
        } else if (introStage == 999) {
            Rectangle source = { 0.0f, 0.0f, (float)logo.width, (float)logo.height };
            Vector2 screenPos = { GetScreenWidth()/2.0f, GetScreenHeight()/2.0f };
            Rectangle dest = { screenPos.x, screenPos.y - 150, (float)logo.width, (float)logo.height };
            Vector2 origin = { (float)logo.width/2, (float)logo.height/2};
            DrawTexturePro(logo, source, dest, origin, 0.0f, ColorAlpha(WHITE, logoFadeIn.peek()));
        }
    } else {
        DrawTexture(pakuj, 0, 0, WHITE);
    }
}

void SceneIntro::SceneExit() {
    UnloadSound(introSound);
    UnloadFont(logoFont);
    UnloadFont(mainFont);
    UnloadTexture(pakuj);
    UnloadTexture(logo);
    UnloadTexture(introLogo);
}