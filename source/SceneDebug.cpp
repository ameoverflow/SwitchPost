#include "SceneDebug.h"

#include <filesystem>
#include "raylib.h"
#include "Helpers.h"
#include "SceneManager.h"
#include "SceneTitle.h"
#include "AssetLoader.h"
#include "SceneTutorial.h"
#include <switch.h>

#include "InpostAPI.h"
#include "spdlog/spdlog.h"

void SceneDebug::SceneInit() {
    mainFont = LoadFontEx("romfs:/fonts/Ubuntu-Bold.ttf", 50, 0, 381);
    change = LoadSound(AssetLoader::ResolveResource("sounds/change.wav").c_str());
    done = LoadSound(AssetLoader::ResolveResource("sounds/go.wav").c_str());
    pakuj = LoadTexture(AssetLoader::ResolveResource("sprites/pakuj.png").c_str());
    options = {
            "Pokazuj testowe paczki",
            "Spakuj się do więzienia :troll:",
            "Włącz tutorial",
            "Ustaw adres serwera InPost Mobile",
            "Zresetuj dane"
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
        InpostAPI::baseUrl = std::string(baseUrl);
        SPDLOG_DEBUG("base address set to {}", InpostAPI::baseUrl);
        askingForUrl = false;
    }

    if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) && !inputLock) {
        inputLock = true;
        SceneManager::ChangeScene(std::make_unique<SceneTitle>());
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
                showFakePackages = true;
                break;
            case 1:
                pakujOn = true;
                break;
            case 2:
                inputLock = true;
                SceneManager::ChangeScene(std::make_unique<SceneTutorial>());
                break;
            case 3:
                askingForUrl = true;
                break;
            case 4:
                std::filesystem::remove("sdmc:/config/switchpost/token.json");
                break;
        }
        PlaySound(done);
    }
}

void SceneDebug::SceneDraw() {
    DrawTextOutlineEx(mainFont, "Ultra tajne debugowe menu", {10, 10}, {0, 0}, 50, 2, WHITE, BLACK, 4);
    int offset = 70;
    for (int i = 0; i < options.size(); i++) {
        DrawTextOutlineEx(mainFont, options[i].c_str(), {10, offset}, {0, 0}, 40, 0, selectedOption == i ? YELLOW : WHITE, BLACK, 2);
        offset += MeasureTextEx(mainFont, options[i].c_str(), 40, 0).y + 10;
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