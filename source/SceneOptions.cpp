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

#include "i18n.h"
#include "MusicManager.h"
#include "SceneIntro.h"
#include "SceneTutorial.h"
#include "SoundManager.h"

void SceneOptions::SceneInit() {
    mainFont = LoadFontEx("romfs:/fonts/Ubuntu-Bold.ttf", 50, 0, 381);
    promptFont = LoadFontEx("romfs:/fonts/Ubuntu-Regular.ttf", 50, 0, 381);
    options = {
            i18n::GetString("options.language"),
            i18n::GetString("options.voice"),
            i18n::GetString("options.resource_pack"),
            i18n::GetString("options.background"),
            i18n::GetString("options.show_tutorial"),
            i18n::GetString("options.clear_data"),
            i18n::GetString("options.credits")
    };

    voices = {
            i18n::GetString("options.voice.none"),
            i18n::GetString("options.voice.male"),
            i18n::GetString("options.voice.female")
    };

    languages = {
        "English",
        "Polski"
    };

    packList = {
            {"", i18n::GetString("options.resource_pack.default"), "ameOverflow"},
    };

    for (std::pair<std::string, ResourcePack> kvp : AssetLoader::RegisteredPacks) {
        packList.push_back(kvp.second);
    }

    oldPack = Config::openedFile.resourcePack;

    buildInfo = std::string(APP_TITLE) + " " + std::string(APP_VERSION) + "\n\n";
    buildInfo += "Build date: " + std::string(__DATE__) + " " + std::string(__TIME__) + "\n";
    buildInfo += "Compiled with GCC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "." + std::to_string(__GNUC_PATCHLEVEL__) + "\n";buildInfo += "raylib " + std::string(RAYLIB_VERSION) + "\n";
    buildInfo += "Horizon OS " + std::to_string(HOSVER_MAJOR(hosversionGet())) + "." + std::to_string(HOSVER_MINOR(hosversionGet())) + "." + std::to_string(HOSVER_MICRO(hosversionGet())) + "\n\n(A) Ok";
}

void SceneOptions::HandleSelectMenu(size_t listSize, bool& activeMenu, std::function<void()> onConfirm) {
    if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) && !inputLock) {
        activeMenu = false;
        return;
    }

    float currentStickValue = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);

    if (currentStickValue > 0.5f && !stickMovedY && selectedSubOption > 0 && !inputLock) {
        stickMovedY = true;
        selectedSubOption--;
        SoundManager::PlaySound(ChangeSound);
    }

    if (currentStickValue < -0.5f && !stickMovedY && selectedSubOption < listSize - 1 && !inputLock) {
        stickMovedY = true;
        selectedSubOption++;
        SoundManager::PlaySound(ChangeSound);
    }

    if (currentStickValue > -0.3f && currentStickValue < 0.3f) {
        stickMovedY = 0;
    }

    if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP) && selectedSubOption > 0 && !inputLock) {
        selectedSubOption--;
        SoundManager::PlaySound(ChangeSound);
    }

    if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN) && selectedSubOption < listSize - 1 && !inputLock) {
        selectedSubOption++;
        SoundManager::PlaySound(ChangeSound);
    }

    if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
        onConfirm();
        SoundManager::PlaySound(GoSound);
        activeMenu = false;
    }
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
        HandleSelectMenu(std::size(packList), inResourcePackOptions, [this]() {
            if (selectedSubOption == 0) {
                Config::openedFile.resourcePack = "";
            } else {
                Config::openedFile.resourcePack = packList[selectedSubOption].directory;
            }

            //reset voice to none if voice doesnt exist in current pack
            if (Config::openedFile.resourcePack == "") {
                if (Config::openedFile.voice != "none" && Config::openedFile.voice != "male" && Config::openedFile.voice != "female") Config::openedFile.voice = "none";
            } else {
                if (Config::openedFile.voice != "none" && Config::openedFile.voice != "male" && Config::openedFile.voice != "female") {
                    std::vector<std::string> voicesList = AssetLoader::RegisteredPacks[Config::openedFile.resourcePack].voices;
                    if (std::find(voicesList.begin(), voicesList.end(), Config::openedFile.voice) == voicesList.end()) Config::openedFile.voice = "none";
                }
            }
        });
    } else if (inVoiceOptions && !inDeleteData && !inNoVoicePopup) {
        HandleSelectMenu(std::size(voices), inVoiceOptions, [this]() {
            if (selectedSubOption == 0) {
                 Config::openedFile.voice = "none";
             } else if (selectedSubOption == 1) {
                 Config::openedFile.voice = "male";
             } else if (selectedSubOption == 2) {
                 Config::openedFile.voice = "female";
             } else {
                 Config::openedFile.voice = voices[selectedSubOption];
             }
        });
    } else if (inLanguageOptions && !inDeleteData && !inNoVoicePopup) {
        HandleSelectMenu(std::size(languages), inLanguageOptions, [this]() {
            if (selectedSubOption == 0) {
                Config::openedFile.language = "en";
            } else if (selectedSubOption == 1) {
                Config::openedFile.language = "pl";
            }
            i18n::SetLanguage(Config::openedFile.language);

            options = {
                i18n::GetString("options.language"),
                i18n::GetString("options.voice"),
                i18n::GetString("options.resource_pack"),
                i18n::GetString("options.background"),
                i18n::GetString("options.show_tutorial"),
                i18n::GetString("options.clear_data"),
                i18n::GetString("options.credits")
            };
            packList = {
                {"", i18n::GetString("options.resource_pack.default"), "ameOverflow"},
            };
        });
    } else if (inDeleteData) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
            std::filesystem::remove("sdmc:/config/switchpost/config.cfg");
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
    } else if (inBuildPopup) {
        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
            inBuildPopup = false;
        }
    } else  {
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
                SoundManager::PlaySound(ChangeSound);
            }

            if (currentStickValue < -0.5f && !stickMovedY && selectedOption < std::size(options) - 1 && !inputLock) {
                stickMovedY = true;
                selectedOption++;
                SoundManager::PlaySound(ChangeSound);
            }

            if (currentStickValue > -0.3f && currentStickValue < 0.3f) {
                stickMovedY = 0;
            }
        }

        // read stick left right
        {
            float currentStickValue = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);

            if (currentStickValue < -0.5f && !stickMovedX && !inputLock && Config::openedFile.background > 0 && selectedOption == 2) {
                stickMovedX = true;
                Config::openedFile.background--;
                SoundManager::PlaySound(ChangeSound);
            }

            if (currentStickValue > 0.5f && !stickMovedX && !inputLock && Config::openedFile.background < std::size(backgrounds) - 1 && selectedOption == 2) {
                stickMovedX = true;
                Config::openedFile.background++;
                SoundManager::PlaySound(ChangeSound);
            }

            if (currentStickValue > -0.3f && currentStickValue < 0.3f) {
                stickMovedX = false;
            }
        }

        // read dpad up down
        {
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP) && selectedOption > 0 && !inputLock) {
                selectedOption--;
                SoundManager::PlaySound(ChangeSound);
            }

            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN) && selectedOption < std::size(options) - 1 && !inputLock) {
                selectedOption++;
                SoundManager::PlaySound(ChangeSound);
            }
        }

        // read dpad left right
        {
            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT) && Config::openedFile.background > 0 && !inputLock && selectedOption == 3) {
                Config::openedFile.background--;
                SoundManager::PlaySound(ChangeSound);
            }

            if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) && Config::openedFile.background < std::size(backgrounds) - 1 && !inputLock && selectedOption == 3) {
                Config::openedFile.background++;
                SoundManager::PlaySound(ChangeSound);
            }
        }

        if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) && !inputLock) {
            if (selectedOption == 3) return;
            SoundManager::PlaySound(GoSound);
            switch (selectedOption) {
                case 0:
                    selectedSubOption = 0;
                    targetOffset = 0;
                    scrollOffset = 0;
                    inLanguageOptions = true;
                    break;
                case 1:
                    voices.clear();
                    voices = {
                        i18n::GetString("options.voice.none"),
                        i18n::GetString("options.voice.male"),
                        i18n::GetString("options.voice.female")
                    };

                    for (std::string packVoice : AssetLoader::RegisteredPacks[Config::openedFile.resourcePack].voices) {
                        voices.push_back(packVoice);
                    }

                    selectedSubOption = 0;
                    inVoiceOptions = true;
                    break;
                case 2:
                    targetOffset = 0;
                    scrollOffset = 0;
                    selectedSubOption = 0;
                    inResourcePackOptions = true;
                    break;
                case 4:
                    if (Config::openedFile.voice.empty() || Config::openedFile.voice == "none" || !std::filesystem::exists(AssetLoader::ResolveResource("tutorial/" + Config::openedFile.voice + "/data.json"))) {
                        inNoVoicePopup = true;
                    } else {
                        SceneManager::ChangeScene(std::make_unique<SceneTutorial>(true));
                    }
                    break;
                case 5:
                    inDeleteData = true;
                    break;
                case 6:
                    targetOffset = 0;
                    scrollOffset = 0;
                    inputLock = true;
                    SceneManager::ChangeScene(std::make_unique<SceneCredits>());
                    break;
            }
        }

        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_MIDDLE_LEFT)) {
            inBuildPopup = true;
        }
    }
}

void SceneOptions::SceneDraw() {
    if (inResourcePackOptions) {
        Vector2 textSize = MeasureTextEx(mainFont, i18n::GetString("options.resource_pack.select").c_str(), 70, 2);
        DrawTextOutlineEx(mainFont, i18n::GetString("options.resource_pack.select").c_str(), {GetScreenWidth()/2, 20 - scrollOffset}, {textSize.x/2, 0}, 70, 2, WHITE, BLACK, 4);
        drawOffset = 100;
        for (int i = 0; i < packList.size(); i++) {
            textSize = MeasureTextEx(mainFont, std::string(packList[i].author + " - " + packList[i].name).c_str(), 40, 0);
            DrawTextOutlineEx(mainFont, std::string(packList[i].author + " - " + packList[i].name).c_str(),
                              {GetScreenWidth()/2, drawOffset - scrollOffset}, {textSize.x/2, 0}, 40, 0, selectedSubOption == i ? YELLOW : WHITE, BLACK, 3);
            drawOffset += textSize.y + 10;
        }
    } else if (inVoiceOptions) {
        Vector2 textSize = MeasureTextEx(mainFont, i18n::GetString("options.voice.select").c_str(), 70, 2);
        DrawTextOutlineEx(mainFont, i18n::GetString("options.voice.select").c_str(), {GetScreenWidth()/2, 20 - scrollOffset}, {textSize.x/2, 0}, 70, 2, WHITE, BLACK, 4);
        drawOffset = 100;
        for (int i = 0; i < voices.size(); i++) {
            textSize = MeasureTextEx(mainFont, voices[i].c_str(), 40, 0);
            DrawTextOutlineEx(mainFont, voices[i].c_str(),
                              {GetScreenWidth()/2, drawOffset - scrollOffset}, {textSize.x/2, 0}, 40, 0, selectedSubOption == i ? YELLOW : WHITE, BLACK, 3);
            drawOffset += textSize.y + 10;
        }
    } else if (inLanguageOptions) {
        Vector2 textSize = MeasureTextEx(mainFont, i18n::GetString("options.language.select").c_str(), 70, 2);
        DrawTextOutlineEx(mainFont, i18n::GetString("options.language.select").c_str(), {GetScreenWidth()/2, 20 - scrollOffset}, {textSize.x/2, 0}, 70, 2, WHITE, BLACK, 4);
        drawOffset = 100;
        for (int i = 0; i < languages.size(); i++) {
            textSize = MeasureTextEx(mainFont, languages[i].c_str(), 40, 0);
            DrawTextOutlineEx(mainFont, languages[i].c_str(),
                              {GetScreenWidth()/2, drawOffset - scrollOffset}, {textSize.x/2, 0}, 40, 0, selectedSubOption == i ? YELLOW : WHITE, BLACK, 3);
            drawOffset += textSize.y + 10;
        }
    } else {
        Vector2 textSize = MeasureTextEx(mainFont, i18n::GetString("options").c_str(), 100, 0);
        DrawTextOutlineEx(mainFont, i18n::GetString("options").c_str(), {GetScreenWidth()/2, 100}, {textSize.x/2, textSize.y/2}, 100, 2, WHITE, BLACK, 6);
        if (oldPack != Config::openedFile.resourcePack) {
            textSize = MeasureTextEx(mainFont, i18n::GetString("options.resource_pack.restart").c_str(), 42, 0);
            DrawTextOutlineEx(mainFont, i18n::GetString("options.resource_pack.restart").c_str(), {GetScreenWidth()/2, GetScreenHeight()/2 + 300}, {textSize.x/2, textSize.y/2}, 42, 0, RED, BLACK, 4);
        }
        int offset = 225;
        for (int i = 0; i < options.size(); i++) {
            Vector2 textSize = MeasureTextEx(mainFont, options[i].c_str(), 50, 0);
            if (i == 0) {
                std::string text = i18n::GetString("options.language") + ": " + i18n::GetString("self");
                textSize = MeasureTextEx(mainFont, text.c_str(), 50, 0);
                DrawTextOutlineEx(mainFont, text.c_str(), {GetScreenWidth()/2, offset}, {textSize.x/2, textSize.y/2}, 50, 0, selectedOption == i ? YELLOW : WHITE, BLACK, 4);
            } else if (i == 1) {
                std::string text = i18n::GetString("options.voice");
                if (Config::openedFile.voice == "male") {
                    text += i18n::GetString("options.voice.male");
                } else if (Config::openedFile.voice == "female") {
                    text +=  i18n::GetString("options.voice.female");
                } else if (Config::openedFile.voice == "none") {
                    text +=  i18n::GetString("options.voice.none");
                } else {
                    text +=  i18n::GetString("options.voice.other") + " (" + Config::openedFile.voice + ")";
                }
                textSize = MeasureTextEx(mainFont, text.c_str(), 50, 0);
                DrawTextOutlineEx(mainFont, text.c_str(), {GetScreenWidth()/2, offset}, {textSize.x/2, textSize.y/2}, 50, 0, selectedOption == i ? YELLOW : WHITE, BLACK, 4);
            } else if (i == 2) {
                std::string text =  i18n::GetString("options.resource_pack");
                if (Config::openedFile.resourcePack == "" || !AssetLoader::RegisteredPacks.contains(Config::openedFile.resourcePack)) {
                    text +=  i18n::GetString("options.resource_pack.default");
                } else {
                    text += AssetLoader::RegisteredPacks[Config::openedFile.resourcePack].name;
                }
                textSize = MeasureTextEx(mainFont, text.c_str(), 50, 0);
                DrawTextOutlineEx(mainFont, text.c_str(), {GetScreenWidth()/2, offset}, {textSize.x/2, textSize.y/2}, 50, 0, selectedOption == i ? YELLOW : WHITE, BLACK, 4);
            } else if (i == 3) {
                std::string text = i18n::GetString("options.background") + ": < " + std::to_string(Config::openedFile.background + 1) + " >";
                textSize = MeasureTextEx(mainFont, text.c_str(), 50, 0);
                DrawTextOutlineEx(mainFont, text.c_str(), {GetScreenWidth()/2, offset}, {textSize.x/2, textSize.y/2}, 50, 0, selectedOption == i ? YELLOW : WHITE, BLACK, 4);
            } else {
                DrawTextOutlineEx(mainFont, options[i].c_str(), {GetScreenWidth()/2, offset}, {textSize.x/2, textSize.y/2}, 50, 0, selectedOption == i ? YELLOW : WHITE, BLACK, 4);
            }
            offset += textSize.y + 10;
        }

        if (inDeleteData || inNoVoicePopup || inBuildPopup) DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0, 0, 0, 192});

        if (inDeleteData) {
            Vector2 textSize = MeasureTextEx(promptFont, i18n::GetString("options.clear_data.warning").c_str(), 32, 0);
            DrawRectangle(GetScreenWidth()/2 - textSize.x/2 - 50, GetScreenHeight()/2 - textSize.y/2 - 50, textSize.x + 100, textSize.y + 100, WHITE);
            DrawTextPro(promptFont, i18n::GetString("options.clear_data.warning").c_str(),
                {GetScreenWidth()/2, GetScreenHeight()/2}, {textSize.x/2, textSize.y/2}, 0, 32, 0, BLACK);
        }

        if (inNoVoicePopup) {
            Vector2 textSize = MeasureTextEx(promptFont, i18n::GetString("options.tutorial.no_voice").c_str(), 32, 0);
            DrawRectangle(GetScreenWidth()/2 - textSize.x/2 - 50, GetScreenHeight()/2 - textSize.y/2 - 50, textSize.x + 100, textSize.y + 100, WHITE);
            DrawTextPro(promptFont, i18n::GetString("options.tutorial.no_voice").c_str(),
                {GetScreenWidth()/2, GetScreenHeight()/2}, {textSize.x/2, textSize.y/2}, 0, 32, 0, BLACK);
        }

        if (inBuildPopup) {
            Vector2 textSize = MeasureTextEx(promptFont,  buildInfo.c_str(), 32, 0);
            DrawRectangle(GetScreenWidth()/2 - textSize.x/2 - 50, GetScreenHeight()/2 - textSize.y/2 - 50, textSize.x + 100, textSize.y + 100, WHITE);
            DrawTextPro(promptFont, buildInfo.c_str(),
                {GetScreenWidth()/2, GetScreenHeight()/2}, {textSize.x/2, textSize.y/2}, 0, 32, 0, BLACK);
        }
    }
}

void SceneOptions::SceneExit() {
    UnloadFont(mainFont);
    UnloadFont(promptFont);
}