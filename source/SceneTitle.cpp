//
// Created by void on 21/02/2026.
//

#include "SceneTitle.h"
#include "raylib.h"
#include "Helpers.h"
#include <string>
#include "tweeny.h"
#include "SceneManager.h"
#include "SceneMain.h"
#include "easing.h"
#include "AssetLoader.h"
#include "Config.h"
#include "SceneDebug.h"
#include "SceneOptions.h"
#include "SceneTutorial.h"


void SceneTitle::SceneInit() {
    mainFont = LoadFontEx("romfs:/fonts/Ubuntu-Bold.ttf", 50, 0, 381);
    smallFont = LoadFontEx("romfs:/fonts/Ubuntu-Regular.ttf", 24, 0, 381);
    logo = LoadTexture(AssetLoader::ResolveResource("sprites/logo.png").c_str());
    logoRender = LoadRenderTexture(logo.width, logo.height);
    collectedAll = LoadSound(AssetLoader::ResolveResource("sounds/collectedall.wav").c_str());
    change = LoadSound(AssetLoader::ResolveResource("sounds/change.wav").c_str());

    transititon = tweeny::from(1.0f).to(0).during(500);
    rotationAnim = tweeny::from(-5.0f).to(5.0f).during(15000).via(tweeny::easing::sinusoidalInOut);
    rotationAnim.seek(0);

    options = {
            "Start",
            "Opcje"
    };

    version = "SwitchPost ";
    version += std::string(APP_VERSION);

    std::string tutorialViewed = Config::GetProperty("tutorialDone");
    std::string voice = Config::GetProperty("voice");
    std::string currentPack = Config::GetProperty("resourcePack");
    if ((tutorialViewed == "" || tutorialViewed != "true") && voice != "" && voice != "none"
        && std::filesystem::exists(AssetLoader::ResolveResource("tutorial/" + voice + "/data.json"))) {
        askForTutorial = true;
    }

    if (std::filesystem::exists(AssetLoader::ResolveResource("voice/" + voice + "/confirm_tutorial.ogg"))) {
        confirmTutorial = LoadSound(AssetLoader::ResolveResource("voice/" + voice + "/confirm_tutorial.ogg").c_str());
    }
}

void SceneTitle::SceneUpdate(float dt) {
    // render logo texture
    BeginTextureMode(logoRender);

    ClearBackground({0, 0, 0, 0});

    DrawTexture(logo, 0, 0, WHITE);

#ifdef DEBUG
    Vector2 textSize = MeasureTextEx(mainFont, bigVersion.c_str(), 60, 2);
    DrawTextOutlineEx(mainFont, bigVersion.c_str(), {logo.width/2 + 200, logo.height/2 + 75}, {textSize.x / 2.0f, textSize.y / 2.0f},
                      60, 2, versionColor, BLACK, 4);
#endif

    EndTextureMode();

    rotationAnim.step((int)(dt * 1000.0f));
    if (rotationAnim.progress() >= 1.0f && rotationAnim.direction() == 1)
    {
        rotationAnim.backward();
    } else if (rotationAnim.progress() <= 0.0f && rotationAnim.direction() == -1) {
        rotationAnim.forward();
    }

    if (!isTransitioning && transititon.progress() <= 1.0f) {
        transititon.step((int)(dt * 1000.0f));
    }

    float currentStickValue = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);

    if (currentStickValue > 0.5f && !stickMoved && selectedOption > 0 && !inputLock && !firstTimeUsingPrompt) {
        stickMoved = true;
        selectedOption--;
        PlaySound(change);
    }

    if (currentStickValue < -0.5f && !stickMoved && selectedOption < std::size(options) - 1 && !inputLock && !firstTimeUsingPrompt) {
        stickMoved = true;
        selectedOption++;
        PlaySound(change);
    }

    if (currentStickValue > -0.3f && currentStickValue < 0.3f) {
        stickMoved = 0;
    }

    if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP) && selectedOption > 0 && !inputLock && !firstTimeUsingPrompt) {
        selectedOption--;
        PlaySound(change);
    }

    if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN) && selectedOption < std::size(options) - 1 && !inputLock && !firstTimeUsingPrompt) {
        selectedOption++;
        PlaySound(change);
    }

    if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) && !isTransitioning && !inputLock && !firstTimeUsingPrompt) {
        if (selectedOption == 0) {
            if (askForTutorial) {
                if (confirmTutorial.frameCount > 0) {
                    PlaySound(confirmTutorial);
                }
                firstTimeUsingPrompt = true;
            } else {
                Config::SetProperty("tutorialDone", "true");
                inputLock = true;
                transititon.seek(0);
                isTransitioning = true;
            }
        } else {
            inputLock = true;
            SceneManager::ChangeScene(std::make_unique<SceneOptions>());
        }
        return;
    }

    if (firstTimeUsingPrompt && !inputLock && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
        Config::SetProperty("tutorialDone", "true");
        firstTimeUsingPrompt = false;
        transititon.seek(0);
        isTransitioning = true;
    }

    if (firstTimeUsingPrompt && !inputLock && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
        transititon.seek(0);
        isTransitioning = true;
    }

#ifdef DEBUG
    if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_LEFT) && !inputLock) {
        inputLock = true;
        SceneManager::ChangeScene(std::make_unique<SceneDebug>());
        return;
    }
#endif

    if (isTransitioning)
    {
        transititon.step((int)(dt * 1000.0f));
        transitionTimer += dt;
    }
    
    if (transitionTimer >= 1.0f) {
        if (firstTimeUsingPrompt) {
            SceneManager::ChangeScene(std::make_unique<SceneTutorial>());
        } else {
            SceneManager::ChangeScene(std::make_unique<SceneMain>());
        }
    }
}

void SceneTitle::SceneDraw() {
    DrawTexturePro(logoRender.texture, {0, 0, logoRender.texture.width, -logoRender.texture.height},
                   (Rectangle){GetScreenWidth()/2, GetScreenHeight()/2 - 150, logo.width, logo.height},
                   {logoRender.texture.width/2, logoRender.texture.height/2}, rotationAnim.peek(), WHITE);

    int offset = GetScreenHeight()/2 + 100;
    for (int i = 0; i < options.size(); i++) {
        Vector2 textSize = MeasureTextEx(mainFont, options[i].c_str(), 60, 0);
        DrawTextOutlineEx(mainFont, options[i].c_str(), {GetScreenWidth()/2, offset}, {textSize.x/2, textSize.y/2}, 60, 0, selectedOption == i ? YELLOW : WHITE, BLACK, 4);
        offset += textSize.y + 10;
    }

    Vector2 textSize = MeasureTextEx(smallFont, version.c_str(), 32, 0);
    DrawTextOutlineEx(smallFont, version.c_str(), {4, GetScreenHeight() - textSize.y - 2}, {0, 0}, 32, 0, WHITE, BLACK, 2);

    if (firstTimeUsingPrompt) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0, 0, 0, 192});
        Vector2 textSize = MeasureTextEx(smallFont, "Czy korzystasz z SwitchPost pierwszy raz?\n\n(A) Tak      (B) Nie", 32, 0);
        DrawRectangle(GetScreenWidth()/2 - textSize.x/2 - 50, GetScreenHeight()/2 - textSize.y/2 - 50, textSize.x + 100, textSize.y + 100, WHITE);
        DrawTextPro(smallFont, "Czy korzystasz z SwitchPost pierwszy raz?\n\n(A) Tak      (B) Nie",
            {GetScreenWidth()/2, GetScreenHeight()/2}, {textSize.x/2, textSize.y/2}, 0, 32, 0, BLACK);
    }

    if (isTransitioning)
        DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK, {10,10,10,255});
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(WHITE, transititon.peek()));

}

void SceneTitle::SceneExit() {
    StopSound(confirmTutorial);
    UnloadSound(confirmTutorial);
    StopSound(collectedAll);
    StopSound(change);
    UnloadFont(mainFont);
    UnloadFont(smallFont);
    UnloadTexture(logo);
    UnloadRenderTexture(logoRender);
    UnloadSound(collectedAll);
    UnloadSound(change);
}