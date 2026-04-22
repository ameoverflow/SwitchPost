#include "SceneOptions.h"

#include <filesystem>
#include "raylib.h"
#include "Helpers.h"
#include "SceneManager.h"
#include "SceneTitle.h"
#include "AssetLoader.h"
#include "Config.h"
#include "SceneCredits.h"
#include <switch.h>

#include "MusicManager.h"
#include "SceneIntro.h"
#include "SceneTutorial.h"

void SceneOptions::SceneInit() {
    mainFont = LoadFontEx("romfs:/fonts/Ubuntu-Bold.ttf", 50, 0, 381);
    promptFont = LoadFontEx("romfs:/fonts/Ubuntu-Regular.ttf", 50, 0, 381);
    change = LoadSound(AssetLoader::ResolveResource("sounds/change.wav").c_str());
    done = LoadSound(AssetLoader::ResolveResource("sounds/go.wav").c_str());
    options = {
            "Głos: ",
            "Paczka zasobów: ",
            "Tło: ",
            "Pokaż samouczek",
            "Wyczyść dane",
            "Autorzy"
    };

    voices = {
            "Brak",
            "Męski",
            "Damski"
    };

    packList = {
            {"", "domyślna", "ameOverflow"},
    };

    for (std::pair<std::string, ResourcePack> kvp : AssetLoader::RegisteredPacks) {
        packList.push_back(kvp.second);
    }

    voice = Config::GetProperty("voice");
    currentResourcePack = Config::GetProperty("resourcePack");
    oldPack = currentResourcePack;
}

void SceneOptions::SceneUpdate(float dt) {
    if (inResourcePackOptions || inVoiceOptions && !inDeleteData && !inNoVoicePopup) {
        float itemHeight = 40.0f;
        float spacing = 10.0f;

        // each item starts 50px (height + spacing) after the previous one
        float selectedOptionY = 100.0f + (itemHeight + spacing) * selectedSubOption;

        float padding = 20.0f; // extra breathing room at top/bottom of screen

        // check bottom: itemTop + itemHeight + padding
        if (selectedOptionY - targetOffset + itemHeight + padding > GetScreenHeight()) {
            targetOffset = selectedOptionY + itemHeight - GetScreenHeight() + padding;
        }
            // check top: itemTop - padding
        else if (selectedOptionY - targetOffset < padding) {
            targetOffset = selectedOptionY - padding;
        }

        if (selectedSubOption < 12) {
            targetOffset = 0;
        }

        // smooth it out
        if (std::fabs((targetOffset - scrollOffset) * 10.0f * dt) <= 0.1f)
            scrollOffset = targetOffset;

        scrollOffset += (targetOffset - scrollOffset) * 10.0f * dt;
    }

    if (inResourcePackOptions && !inDeleteData && !inNoVoicePopup) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) && !inputLock) {
            inResourcePackOptions = false;
            return;
        }

        float currentStickValue = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);

        if (currentStickValue > 0.5f && !stickMovedY && selectedSubOption > 0 && !inputLock) {
            stickMovedY = true;
            selectedSubOption--;
            PlaySound(change);
        }

        if (currentStickValue < -0.5f && !stickMovedY && selectedSubOption < std::size(packList) - 1 && !inputLock) {
            stickMovedY = true;
            selectedSubOption++;
            PlaySound(change);
        }

        if (currentStickValue > -0.3f && currentStickValue < 0.3f) {
            stickMovedY = 0;
        }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP) && selectedSubOption > 0 && !inputLock) {
            selectedSubOption--;
            PlaySound(change);
        }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN) && selectedSubOption < std::size(packList) - 1 && !inputLock) {
            selectedSubOption++;
            PlaySound(change);
        }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) && !inputLock) {
            if (selectedSubOption == 0) {
                currentResourcePack = "";
            } else {
                currentResourcePack = packList[selectedSubOption].directory;
            }

            //reset voice to none if voice doesnt exist in current pack
            if (currentResourcePack == "") {
                if (voice != "none" && voice != "male" && voice != "female") voice = "none";
            } else {
                if (voice != "none" && voice != "male" && voice != "female") {
                    std::vector<std::string> voicesList = AssetLoader::RegisteredPacks[currentResourcePack].voices;
                    if (std::find(voicesList.begin(), voicesList.end(), voice) == voicesList.end()) voice = "none";
                }
            }

            PlaySound(done);
            inResourcePackOptions = false;
        }
    } else if (inVoiceOptions && !inDeleteData && !inNoVoicePopup) {
        float currentStickValue = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);

        if (currentStickValue > 0.5f && !stickMovedY && selectedSubOption > 0 && !inputLock) {
            stickMovedY = true;
            selectedSubOption--;
            PlaySound(change);
        }

        if (currentStickValue < -0.5f && !stickMovedY && selectedSubOption < std::size(voices) - 1 && !inputLock) {
            stickMovedY = true;
            selectedSubOption++;
            PlaySound(change);
        }

        if (currentStickValue > -0.3f && currentStickValue < 0.3f) {
            stickMovedY = 0;
        }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) && !inputLock) {
            inVoiceOptions = false;
            return;
        }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP) && selectedSubOption > 0 && !inputLock) {
            selectedSubOption--;
            PlaySound(change);
        }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN) && selectedSubOption < std::size(voices) - 1 && !inputLock) {
            selectedSubOption++;
            PlaySound(change);
        }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) && !inputLock) {
            if (selectedSubOption == 0) {
                voice = "none";
            } else if (selectedSubOption == 1) {
                voice = "male";
            } else if (selectedSubOption == 2) {
                voice = "female";
            } else {
                voice = voices[selectedSubOption];
            }
            PlaySound(done);
            inVoiceOptions = false;
        }
    } else if (inDeleteData) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
            std::filesystem::remove("sdmc:/config/switchpost/config.json");
            std::filesystem::remove("sdmc:/config/switchpost/token.json");
            MusicManager::Stop();
            SceneManager::ChangeScene(std::make_unique<SceneIntro>());
            return;
        }
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
            inDeleteData = false;
        }
    } else if (inNoVoicePopup) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
            inNoVoicePopup = false;
        }
    } else {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) && !inputLock) {
            inputLock = true;
            SceneManager::ChangeScene(std::make_unique<SceneTitle>());
            return;
        }

        // read stick up down
        {
            float currentStickValue = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);

            if (currentStickValue > 0.5f && !stickMovedY && selectedOption > 0 && !inputLock) {
                stickMovedY = true;
                selectedOption--;
                PlaySound(change);
            }

            if (currentStickValue < -0.5f && !stickMovedY && selectedOption < std::size(options) - 1 && !inputLock) {
                stickMovedY = true;
                selectedOption++;
                PlaySound(change);
            }

            if (currentStickValue > -0.3f && currentStickValue < 0.3f) {
                stickMovedY = 0;
            }
        }

        // read stick left right
        {
            float currentStickValue = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);

            if (currentStickValue < -0.5f && !stickMovedX && !inputLock && currentBackground > 0 && selectedOption == 2) {
                stickMovedX = true;
                currentBackground--;
                PlaySound(change);
            }

            if (currentStickValue > 0.5f && !stickMovedX && !inputLock && currentBackground < std::size(backgrounds) - 1 && selectedOption == 2) {
                stickMovedX = true;
                currentBackground++;
                PlaySound(change);
            }

            if (currentStickValue > -0.3f && currentStickValue < 0.3f) {
                stickMovedX = false;
            }
        }

        // read dpad up down
        {
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP) && selectedOption > 0 && !inputLock) {
                selectedOption--;
                PlaySound(change);
            }

            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN) && selectedOption < std::size(options) - 1 && !inputLock) {
                selectedOption++;
                PlaySound(change);
            }
        }

        // read dpad left right
        {
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT) && currentBackground > 0 && !inputLock && selectedOption == 2) {
                currentBackground--;
                PlaySound(change);
            }

            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) && currentBackground < std::size(backgrounds) - 1 && !inputLock && selectedOption == 2) {
                currentBackground++;
                PlaySound(change);
            }
        }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) && !inputLock) {
            if (selectedOption == 2) return;
            PlaySound(done);
            switch (selectedOption) {
                case 0:
                    voices.clear();
                    voices = {
                            "Brak",
                            "Męski",
                            "Damski"
                    };

                    for (std::string packVoice : AssetLoader::RegisteredPacks[currentResourcePack].voices) {
                        voices.push_back(packVoice);
                    }

                    selectedSubOption = 0;
                    inVoiceOptions = true;
                    break;
                case 1:
                    targetOffset = 0;
                    scrollOffset = 0;
                    selectedSubOption = 0;
                    inResourcePackOptions = true;
                    break;
                case 3:
                    if (voice.empty() || voice == "none" || !std::filesystem::exists(AssetLoader::ResolveResource("tutorial/" + voice + "/data.json"))) {
                        inNoVoicePopup = true;
                    } else {
                        SceneManager::ChangeScene(std::make_unique<SceneTutorial>(true));
                    }
                    break;
                case 4:
                    inDeleteData = true;
                    break;
                case 5:
                    targetOffset = 0;
                    scrollOffset = 0;
                    inputLock = true;
                    SceneManager::ChangeScene(std::make_unique<SceneCredits>());
                    break;
            }
        }
    }
}

void SceneOptions::SceneDraw() {
    if (inResourcePackOptions) {
        Vector2 textSize = MeasureTextEx(mainFont, "Wybierz paczkę zasobów", 70, 2);
        DrawTextOutlineEx(mainFont, "Wybierz paczkę zasobów", {GetScreenWidth()/2, 20 - scrollOffset}, {textSize.x/2, 0}, 70, 2, WHITE, BLACK, 4);
        drawOffset = 100;
        for (int i = 0; i < packList.size(); i++) {
            textSize = MeasureTextEx(mainFont, std::string(packList[i].author + " - " + packList[i].name).c_str(), 40, 0);
            DrawTextOutlineEx(mainFont, std::string(packList[i].author + " - " + packList[i].name).c_str(),
                              {GetScreenWidth()/2, drawOffset - scrollOffset}, {textSize.x/2, 0}, 40, 0, selectedSubOption == i ? YELLOW : WHITE, BLACK, 3);
            drawOffset += textSize.y + 10;
        }
    } else if (inVoiceOptions) {
        Vector2 textSize = MeasureTextEx(mainFont, "Wybierz głos", 70, 2);
        DrawTextOutlineEx(mainFont, "Wybierz głos", {GetScreenWidth()/2, 20 - scrollOffset}, {textSize.x/2, 0}, 70, 2, WHITE, BLACK, 4);
        drawOffset = 100;
        for (int i = 0; i < voices.size(); i++) {
            textSize = MeasureTextEx(mainFont, voices[i].c_str(), 40, 0);
            DrawTextOutlineEx(mainFont, voices[i].c_str(),
                              {GetScreenWidth()/2, drawOffset - scrollOffset}, {textSize.x/2, 0}, 40, 0, selectedSubOption == i ? YELLOW : WHITE, BLACK, 3);
            drawOffset += textSize.y + 10;
        }
    } else {
        Vector2 textSize = MeasureTextEx(mainFont, "Opcje", 100, 0);
        DrawTextOutlineEx(mainFont, "Opcje", {GetScreenWidth()/2, 100}, {textSize.x/2, textSize.y/2}, 100, 2, WHITE, BLACK, 6);
        if (oldPack != currentResourcePack) {
            textSize = MeasureTextEx(mainFont, "Zmiana paczki zasobów wymaga restartu aplikacji", 42, 0);
            DrawTextOutlineEx(mainFont, "Zmiana paczki zasobów wymaga restartu aplikacji", {GetScreenWidth()/2, GetScreenHeight()/2 + 300}, {textSize.x/2, textSize.y/2}, 42, 0, RED, BLACK, 4);
        }
        int offset = 250;
        for (int i = 0; i < options.size(); i++) {
            Vector2 textSize = MeasureTextEx(mainFont, options[i].c_str(), 50, 0);
            if (i == 0) {
                std::string text = "Głos: ";
                if (voice == "male") {
                    text += "męski";
                } else if (voice == "female") {
                    text += "damski";
                } else if (voice == "none") {
                    text += "brak";
                } else {
                    text += "inny (" + voice + ")";
                }
                textSize = MeasureTextEx(mainFont, text.c_str(), 50, 0);
                DrawTextOutlineEx(mainFont, text.c_str(), {GetScreenWidth()/2, offset}, {textSize.x/2, textSize.y/2}, 50, 0, selectedOption == i ? YELLOW : WHITE, BLACK, 4);
            } else if (i == 1) {
                std::string text = "Paczka zasobów: ";
                if (currentResourcePack == "" || !AssetLoader::RegisteredPacks.contains(currentResourcePack)) {
                    text += "domyślna";
                } else {
                    text += AssetLoader::RegisteredPacks[currentResourcePack].name;
                }
                textSize = MeasureTextEx(mainFont, text.c_str(), 50, 0);
                DrawTextOutlineEx(mainFont, text.c_str(), {GetScreenWidth()/2, offset}, {textSize.x/2, textSize.y/2}, 50, 0, selectedOption == i ? YELLOW : WHITE, BLACK, 4);
            } else if (i == 2) {
                std::string text = "Tło: < " + std::to_string(currentBackground + 1) + " >";
                textSize = MeasureTextEx(mainFont, text.c_str(), 50, 0);
                DrawTextOutlineEx(mainFont, text.c_str(), {GetScreenWidth()/2, offset}, {textSize.x/2, textSize.y/2}, 50, 0, selectedOption == i ? YELLOW : WHITE, BLACK, 4);
            } else {
                DrawTextOutlineEx(mainFont, options[i].c_str(), {GetScreenWidth()/2, offset}, {textSize.x/2, textSize.y/2}, 50, 0, selectedOption == i ? YELLOW : WHITE, BLACK, 4);
            }
            offset += textSize.y + 10;
        }

        if (inDeleteData) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0, 0, 0, 192});
            Vector2 textSize = MeasureTextEx(promptFont, "Czy chcesz usunąć WSZYSTKIE dane?\nAplikacja zostanie uruchomiona ponownie, i będzie wymagane\nponowne zalogowanie się.\n\n(A) Tak      (B) Nie", 32, 0);
            DrawRectangle(GetScreenWidth()/2 - textSize.x/2 - 50, GetScreenHeight()/2 - textSize.y/2 - 50, textSize.x + 100, textSize.y + 100, WHITE);
            DrawTextPro(promptFont, "Czy chcesz usunąć WSZYSTKIE dane?\nAplikacja zostanie uruchomiona ponownie, i będzie wymagane\nponowne zalogowanie się.\n\n(A) Tak      (B) Nie",
                {GetScreenWidth()/2, GetScreenHeight()/2}, {textSize.x/2, textSize.y/2}, 0, 32, 0, BLACK);
        }

        if (inNoVoicePopup) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0, 0, 0, 192});
            Vector2 textSize = MeasureTextEx(promptFont, "Brak samouczka dla aktualnie wybranego głosu.\n(A) Ok", 32, 0);
            DrawRectangle(GetScreenWidth()/2 - textSize.x/2 - 50, GetScreenHeight()/2 - textSize.y/2 - 50, textSize.x + 100, textSize.y + 100, WHITE);
            DrawTextPro(promptFont, "Brak samouczka dla aktualnie wybranego głosu.\n(A) Ok",
                {GetScreenWidth()/2, GetScreenHeight()/2}, {textSize.x/2, textSize.y/2}, 0, 32, 0, BLACK);
        }
    }
}

void SceneOptions::SceneExit() {
    if (!inDeleteData) {
        Config::SetProperty("voice", voice);
        Config::SetProperty("resourcePack", currentResourcePack);
        Config::SetProperty("background", std::to_string(currentBackground));
    }
    StopSound(done);
    StopSound(change);
    UnloadFont(mainFont);
    UnloadFont(promptFont);
    UnloadSound(done);
    UnloadSound(change);
}