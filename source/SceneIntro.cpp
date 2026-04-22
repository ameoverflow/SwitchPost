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
#include "InpostAPI.h"
#include "json.hpp"
#include "Config.h"

std::string splashes[] = {
    "now 100% more snow on the roof of amberexpo",
    "paczkomat shitting extraordinaire",
    "better than friday night funkin' fight me ninjamuffin",
    "dev is fast asleep",
    "nepnepnepnepnepnepnepnepnepnep",
    "energy drink induced development",
    "nintendo dont sent ninjas plz",
    "se lecę w klapkach najka",
    "open source forever",
    "der polnische adventskalender",
    "delayed splash, didnt have time",
    "rate 5 stars and subscribe",
    "inspired by toilet pic"
};

std::shared_ptr<ResponseBuffer> bufferPointer = std::make_shared<ResponseBuffer>();

void SceneIntro::SceneInit() {
    pakuj = LoadTexture(AssetLoader::ResolveResource("sprites/pakuj.png").c_str());
    logo = LoadTexture(AssetLoader::ResolveResource("sprites/logo.png").c_str());
    introSound = LoadSound(AssetLoader::ResolveResource("sounds/intro.wav").c_str());
    introTimer = 0;
    mainFont = LoadFontEx("romfs:/fonts/Ubuntu-Regular.ttf", 90, 0, 381);
    logoFont = LoadFontEx("romfs:/fonts/ComicHelvetic_Heavy.otf", 90, 0, 381);
    introStage = 0;
    at = appletGetAppletType();
    voice = Config::GetProperty("voice");

    logoFadeIn = tweeny::from(0.0f).to(0.8f).during(644);
    errorBgFade = tweeny::from(32).to(64).during(2000).via(tweeny::easing::sinusoidalInOut);
    ameLogoFadeIn = tweeny::from(0).to(255).during(100).via(tweeny::easing::quadraticIn);
    logoFadeIn.seek(0);
    ameLogoFadeIn.seek(0);

    std::srand(std::time(NULL));
    if (std::size(splashes) > 0) {
        int randomIndex = std::rand() % std::size(splashes);
        line = splashes[randomIndex];
    }

    randomNum = std::rand() % 500;
    SPDLOG_DEBUG("random number choosen is {}", randomNum);
}

void SceneIntro::SceneUpdate(float dt) {
    errorBgFade.step((int)(dt * 1000.0f));
    if (errorBgFade.progress() >= 1.0f && errorBgFade.direction() == 1) {
        errorBgFade.backward();
    } else if (errorBgFade.progress() <= 0.0f && errorBgFade.direction() == -1) {
        errorBgFade.forward();
    }

    if (error != 0) {
        switch (error) {
            case NetworkError:
                errorCode = "Błąd sieci, sprawdź połączenie z internetem";
                break;
            case SDError:
                errorCode = "Błąd zapisywania konfiguracji";
                break;
            case AppletError:
                errorCode = "Aplikacja uruchomiona w trybie apletu";
                break;
        }
        return;
    }

    if (randomNum != 67) {
        if (introStage == 0) {
            // initialize shit
            // check if its not applet mode first
            if (at != AppletType_Application && at != AppletType_SystemApplication) {
                error = AppletError;
                SPDLOG_CRITICAL("application running as applet");
            } else {
                if (bufferPointer->status == NotStarted) {
                    status = "Sprawdzanie sieci...";
                    Request::QueueRequest("https://api-inmobile-pl.easypack24.net", "", {  }, bufferPointer);
                    bufferPointer->status = InProgress;
                } else if (bufferPointer->status == Error) {
                    if (bufferPointer->result != CURLE_OK) {
                        error = NetworkError;
                        SPDLOG_CRITICAL("networking error, curl returned {} ", std::to_string(bufferPointer->result));
                    }
                } else if (bufferPointer->status == InProgress) {
                    status = "";
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
            if (introTimer > 2 && bufferPointer->status == Done) {
                introTimer = 0;
                introStage = 3;
            }
        } else if (introStage == 3) {
            if (std::filesystem::exists("sdmc:/config/switchpost/token.json")) {
                introStage = 4;
                return;
            }

            // log in
            if (!std::filesystem::exists("sdmc:/config/switchpost/token.json") && bufferPointer->status == Done) {
                if (InpostAPI::sendSMSCodeBuffer->status == NotStarted) {
                    while (true) {
                        swkbdCreate(&kbd, 0);
                        swkbdConfigSetType(&kbd, SwkbdType_NumPad);
                        swkbdConfigSetStringLenMax(&kbd, 9);
                        swkbdConfigSetStringLenMin(&kbd, 9);
                        swkbdConfigSetHeaderText(&kbd, "Wprowadź numer telefonu");
                        swkbdConfigSetGuideText(&kbd, "600100100");

                        rc = swkbdShow(&kbd, phoneNumber, sizeof(phoneNumber));
                        swkbdClose(&kbd);

                        if (R_SUCCEEDED(rc)) break;
                    }

                    InpostAPI::SendSMSCode(std::string(phoneNumber));
                }

                if (InpostAPI::sendSMSCodeBuffer->status == Done &&
                    InpostAPI::verifySMSCodeBuffer->status == NotStarted) {
                    while (true) {
                        swkbdCreate(&kbd, 0);
                        swkbdConfigSetType(&kbd, SwkbdType_NumPad);
                        swkbdConfigSetStringLenMax(&kbd, 6);
                        swkbdConfigSetStringLenMin(&kbd, 6);
                        swkbdConfigSetHeaderText(&kbd, "Wprowadź kod SMS");
                        swkbdConfigSetGuideText(&kbd, "123456");

                        rc = swkbdShow(&kbd, code, sizeof(code));
                        swkbdClose(&kbd);

                        if (R_SUCCEEDED(rc)) break;
                    }

                    InpostAPI::VerifySMSCode(std::string(phoneNumber), std::string(code));
                } else if (InpostAPI::sendSMSCodeBuffer->status == Error ||
                           (InpostAPI::sendSMSCodeBuffer->status == Done &&
                            InpostAPI::sendSMSCodeBuffer->code != 200)) {
                    error = NetworkError;
                    SPDLOG_CRITICAL("network error, curl code is {}, http code is {}", std::to_string(bufferPointer->result), std::to_string(InpostAPI::sendSMSCodeBuffer->code));
                    return;
                }

                if (InpostAPI::verifySMSCodeBuffer->status == Done && InpostAPI::verifySMSCodeBuffer->code == 200) {
                    std::string loginData(InpostAPI::verifySMSCodeBuffer->data.begin(),
                                          InpostAPI::verifySMSCodeBuffer->data.end());
                    if (nlohmann::json::accept(loginData)) {
                        nlohmann::json data = nlohmann::json::parse(loginData);
                        std::string authToken = data.value("authToken", "");
                        std::string refreshToken = data.value("refreshToken", "");

                        if (!authToken.empty() && !refreshToken.empty()) {
                            std::ofstream file("sdmc:/config/switchpost/token.json");
                            if (file.is_open()) {
                                file << loginData;
                                file.close();
                                SPDLOG_INFO("login data saved to SD");
                                introStage = 4;
                            } else {
                                SPDLOG_ERROR("couldnt open token.json for writing");
                                error = SDError;
                                return;
                            }
                        } else {
                            error = SDError;
                            return;
                        }
                    } else {
                        error = SDError;
                        return;
                    }
                } else if (InpostAPI::verifySMSCodeBuffer->status == Error ||
                           (InpostAPI::verifySMSCodeBuffer->status == Done &&
                            InpostAPI::verifySMSCodeBuffer->code != 200)) {
                    error = NetworkError;
                    SPDLOG_CRITICAL("network error, curl code is {}, http code is {}", std::to_string(bufferPointer->result), std::to_string(InpostAPI::sendSMSCodeBuffer->code));
                    return;
                }
            }
        } else if (introStage == 4) {
            if (voice != "") {
                introStage = 999;
                MusicManager::PlayMusic("music/menu_music.ogg");
                return;
            }

            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) ||
            IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP) ||
            IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
                    Config::SetProperty("voice", "male");
                } else if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP)) {
                    Config::SetProperty("voice", "female");
                } else {
                    Config::SetProperty("voice", "none");
                }
                SPDLOG_TRACE("voice set to {}", Config::GetProperty("voice"));
                MusicManager::PlayMusic("music/menu_music.ogg");
                introStage = 999;
                return;
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
        if (error != 0) {
            DrawRectangleGradientV(0, 0, 1280, 720, BLACK, {errorBgFade.peek(),0,0,255});
            Vector2 textSize = MeasureTextEx(mainFont, errorCode.c_str(), 34, 0);
            DrawTextOutlineEx(mainFont, errorCode.c_str(), {1280/2, 720/2},
                              {textSize.x / 2.0f, textSize.y / 2.0f}, 34, 0, RED, BLACK, 2);
            return;
        }

        if (introStage == 1 || introStage == 2) {
            Vector2 textSize = MeasureTextEx(logoFont, text.c_str(), 60, 4);
            DrawTextOutlineEx(logoFont, text.c_str(), {1280/2, 720/2},
                              {textSize.x / 2.0f, textSize.y / 2.0f}, 60, 4, {14, 21, 49, ameLogoFadeIn.peek()}, {147, 86, 234, ameLogoFadeIn.peek()}, 7);

            textSize = MeasureTextEx(mainFont, line.c_str(), 34, 0);
            DrawTextPro(mainFont, line.c_str(), {1280/2, 720/2 + 250},
                        {textSize.x / 2.0f, textSize.y / 2.0f}, 0, 34, 0, WHITE);

            textSize = MeasureTextEx(mainFont, status.c_str(), 28, 0);
            DrawTextPro(mainFont, status.c_str(), {1280/2, 720/2 + 300},
                        {textSize.x / 2.0f, textSize.y / 2.0f}, 0, 28, 0, GRAY);
        } else if (introStage == 4) {
            if (voice != "") return;
            Vector2 textSize = MeasureTextEx(mainFont, "Wybierz głos nawigatora głosowego\n\n(A) Mężczyzna\n(X) Kobieta\n(B) Brak", 34, 0);
            DrawTextPro(mainFont, "Wybierz głos nawigatora głosowego\n\n(A) Mężczyzna\n(X) Kobieta\n(B) Brak",
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
}