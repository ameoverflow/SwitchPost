#include "SceneDebug.h"

#include <filesystem>
#include "raylib.h"
#include "Helpers.h"
#include "SceneManager.h"
#include "SceneTitle.h"
#include "AssetLoader.h"
#include <switch.h>

#include "InPostAPI.h"
#include "SceneIntro.h"
#include "spdlog/spdlog.h"

void SceneDebug::SceneInit() {
    mainFont = LoadFontEx("romfs:/fonts/Ubuntu-Bold.ttf", 50, 0, 381);
    change = LoadSound(AssetLoader::ResolveResource("sounds/change.wav").c_str());
    done = LoadSound(AssetLoader::ResolveResource("sounds/go.wav").c_str());
    pakuj = LoadTexture(AssetLoader::ResolveResource("sprites/pakuj.png").c_str());
    options = {
            "Pokazuj testowe paczki",
            "Spakuj się do więzienia :troll:",
            "Ustaw adres serwera InPost Mobile",
            "Blokada zapisu na kartę SD"
    };
}

void SceneDebug::SceneUpdate(float dt) {
    if (pakujOn) return;

    if (askingForUrl) {
        while (true) {
            swkbdCreate(&kbd, 0);
            swkbdConfigSetType(&kbd, SwkbdType_Normal);
            swkbdConfigSetStringLenMax(&kbd, 64);
            swkbdConfigSetStringLenMin(&kbd, 7);
            swkbdConfigSetHeaderText(&kbd, "Wprowadź adres URL serwera");
            swkbdConfigSetGuideText(&kbd, "http://127.0.0.1:8000");

            rc = swkbdShow(&kbd, baseUrl, sizeof(baseUrl));
            swkbdClose(&kbd);

            if (R_SUCCEEDED(rc)) break;
        }
        InPostAPI::baseUrl = std::string(baseUrl);
        SPDLOG_DEBUG("base address set to {}", InPostAPI::baseUrl);
        askingForUrl = false;
    }

    if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) && !inputLock) {
        inputLock = true;
        SceneManager::ChangeScene(std::make_unique<SceneIntro>());
        return;
    }

    if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP) && selectedOption > 0 && !inputLock) {
        selectedOption--;
        PlaySound(change);
    }

    if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN) && selectedOption < std::size(options) - 1 && !inputLock) {
        selectedOption++;
        PlaySound(change);
    }

    if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) && !inputLock) {
        switch (selectedOption) {
            case 0:
                showFakePackages = !showFakePackages;
                break;
            case 1:
                pakujOn = true;
                break;
            case 2:
                askingForUrl = true;
                break;
            case 3:
                disableSavingToSD = !disableSavingToSD;
                break;
        }
        PlaySound(done);
    }
}

void SceneDebug::SceneDraw() {
    DrawRectangle(0, 0, 1280, 720, BLACK);
    DrawTextOutlineEx(mainFont, "Opcje uruchamiania", {10, 30}, {0, 0}, 28, 2, WHITE, BLACK, 2);
    int offset = 64;
    for (int i = 0; i < options.size(); i++) {
        if (i == 0) {
            DrawTextOutlineEx(mainFont, std::string(options[i] + (showFakePackages ? ": TAK": ": NIE")).c_str(), {10, offset}, {0, 0}, 28, 0, selectedOption == i ? YELLOW : WHITE, BLACK, 2);
        } else if (i == 3) {
            DrawTextOutlineEx(mainFont, std::string(options[i] + (disableSavingToSD ? ": TAK": ": NIE")).c_str(), {10, offset}, {0, 0}, 28, 0, selectedOption == i ? YELLOW : WHITE, BLACK, 2);
        } else if (i == 2) {
            DrawTextOutlineEx(mainFont, std::string(options[i] + ": " + InPostAPI::baseUrl).c_str(), {10, offset}, {0, 0}, 28, 0, selectedOption == i ? YELLOW : WHITE, BLACK, 2);
        } else {
            DrawTextOutlineEx(mainFont, options[i].c_str(), {10, offset}, {0, 0}, 28, 0, selectedOption == i ? YELLOW : WHITE, BLACK, 2);
        }
        offset += MeasureTextEx(mainFont, options[i].c_str(), 28, 0).y + 3;
    }
    if (askingForUrl) DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0, 0, 0, 192});
    if (pakujOn) DrawTexture(pakuj, 0, 0, WHITE);
}

void SceneDebug::SceneExit() {
    StopSound(done);
    StopSound(change);
    UnloadFont(mainFont);
    UnloadSound(done);
    UnloadSound(change);
    UnloadTexture(pakuj);
}