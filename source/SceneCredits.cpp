//
// Created by void on 25/03/2026.
//

#include "SceneCredits.h"
#include <string>
#include <fstream>
#include "SceneManager.h"
#include "MusicManager.h"
#include "SceneOptions.h"
#include "Helpers.h"
#include "AssetLoader.h"

void SceneCredits::SceneInit() {
    logoBg = LoadTexture(AssetLoader::ResolveResource("sprites/logo.png").c_str());
    mainFont = LoadFontEx("romfs:/fonts/ComicHelvetic_Light.otf", 42, 0, 381);

    std::ifstream creditsFile(AssetLoader::ResolveResource("text/credits.txt").c_str());

    std::string line;
    while (std::getline(creditsFile, line)) {
        credits.push_back(line);
    }
    creditsFile.close();

    MusicManager::PlayMusic("music/credits.ogg");
    Vector2 lineSize = MeasureTextEx(mainFont, "M", 35, 0);
    lineHeight = lineSize.y;
}

void SceneCredits::SceneUpdate(float dt) {
    scroll -= (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN) ? 140 : 35) * dt;
    if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) && !inputLock) {
        inputLock = true;
        SceneManager::ChangeScene(std::make_unique<SceneOptions>());
    }
}

void SceneCredits::SceneDraw() {
    lineOffset = 0;
    DrawRectangleGradientV(0, 0, 1280, 720, BLACK, {10,10,10,255});

    DrawTexturePro(logoBg,
                   {0, 0, logoBg.width, logoBg.height},
                   {GetScreenWidth()/2, scroll + lineOffset, logoBg.width, logoBg.height},
                   {logoBg.width/2, logoBg.height/2},
                   0, WHITE);
    lineOffset = logoBg.height/2;

    for (std::string line : credits) {
        Vector2 textSize = MeasureTextEx(mainFont, line.c_str(), 35, 1);
        DrawTextPro(mainFont, line.c_str(), {GetScreenWidth()/2, scroll + lineOffset}, {textSize.x/2, lineHeight/2}, 0, 35, 1, WHITE);
        lineOffset += lineHeight + 10;
    }

    Vector2 textSize = MeasureTextEx(mainFont, "Wciśnij (B) żeby wrócić do menu głównego", 28, 0);
    DrawRectangle(0, GetScreenHeight() - textSize.y, GetScreenWidth(), textSize.y, {10, 10, 10, 255});
    DrawTextOutlineEx(mainFont, "Wciśnij (B) żeby wrócić do menu głównego", {GetScreenWidth()/2, GetScreenHeight() - textSize.y/2}, {textSize.x/2, textSize.y/2}, 28, 0, WHITE, BLACK, 3);
}

void SceneCredits::SceneExit() {
    UnloadTexture(logoBg);
    UnloadFont(mainFont);
    MusicManager::PlayMusic("music/menu_music.ogg");
}