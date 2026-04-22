//
// Created by void on 09/04/2026.
//

#include "SceneTutorial.h"

#include <string>
#include <vector>
#include <json.hpp>
#include "AssetLoader.h"
#include "raylib.h"
#include <fstream>
#include "spdlog/spdlog.h"
#include "SceneManager.h"
#include "Helpers.h"
#include "Config.h"
#include "SceneMain.h"
#include "SceneOptions.h"

std::vector<std::string> SplitString(const std::string& str,
                                     const std::string& delimiter)
{
    std::vector<std::string> strings;

    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = str.find(delimiter, prev)) != std::string::npos)
    {
        strings.push_back(str.substr(prev, pos - prev));
        prev = pos + delimiter.size();
    }

    // To get the last substring (or only, if delimiter is not found)
    strings.push_back(str.substr(prev));

    return strings;
}

void SceneTutorial::SceneInit() {
    characterAnim = tweeny::from(-25.0f).to(0.0f).during(200).via(tweeny::easing::sinusoidalInOut);
    mainFont = LoadFontEx("romfs:/fonts/ComicHelvetic_Light.otf", 42, 0, 381);
    textbox = LoadTexture(AssetLoader::ResolveResource("sprites/textbox.png").c_str());
    tut = Config::GetProperty("voice");

    Vector2 lineSize = MeasureTextEx(mainFont, "M", 32, 1);
    lineHeight = lineSize.y;

    SPDLOG_TRACE("tutorial file: {}", AssetLoader::ResolveResource("tutorial/" + tut + "/data.json"));
    std::ifstream file(AssetLoader::ResolveResource("tutorial/" + tut + "/data.json"));
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        if (nlohmann::json::accept(buffer.str())) {
            nlohmann::json data = nlohmann::json::parse(buffer.str());
            for (nlohmann::json frameJson : data) {
                TutorialFrame frame;
                if (!frameJson.contains("speakingSprite") || frameJson["speakingSprite"].is_null() ||
                    !frameJson.contains("idleSprite") || frameJson["idleSprite"].is_null() ||
                    !frameJson.contains("text") || frameJson["text"].is_null() ||
                    !frameJson.contains("voiceClip") || frameJson["voiceClip"].is_null()) {
                    SPDLOG_WARN("frame not valid");
                    continue;
                }

                SPDLOG_TRACE("idle sprite: {}", AssetLoader::ResolveResource("tutorial/" + tut + "/sprites/" + frameJson["idleSprite"].get<std::string>()));
                frame.idleSprite = AssetLoader::ResolveResource("tutorial/" + tut + "/sprites/" + frameJson["idleSprite"].get<std::string>());

                SPDLOG_TRACE("speaking sprite: {}", AssetLoader::ResolveResource("tutorial/" + tut + "/sprites/" + frameJson["speakingSprite"].get<std::string>()));
                frame.speakingSprite = AssetLoader::ResolveResource("tutorial/" + tut + "/sprites/" + frameJson["speakingSprite"].get<std::string>());

                SPDLOG_TRACE("text: {}", frameJson["text"].get<std::string>());
                frame.text = frameJson["text"].get<std::string>();

                SPDLOG_TRACE("voice clip: {}", AssetLoader::ResolveResource("tutorial/" + tut + "/voice/" + frameJson["voiceClip"].get<std::string>()));
                frame.voiceClip = AssetLoader::ResolveResource("tutorial/" + tut + "/voice/" + frameJson["voiceClip"].get<std::string>());

                if (frameJson.contains("background") && !frameJson["background"].is_null()) {
                    SPDLOG_TRACE("background: {}", AssetLoader::ResolveResource("tutorial/backgrounds/" + frameJson["background"].get<std::string>()));
                    frame.background = AssetLoader::ResolveResource("tutorial/backgrounds/" + frameJson["background"].get<std::string>());
                }

                Frames.push_back(frame);
                SPDLOG_INFO("registered frame");
            }
        } else {
            SPDLOG_WARN("tutorial file not valid");
        }
    } else {
        SPDLOG_WARN("tutorial file not valid");
    }
    speakingSprite = LoadTexture(Frames[currentFrame].speakingSprite.c_str());
    idleSprite = LoadTexture(Frames[currentFrame].idleSprite.c_str());
    background = LoadTexture(Frames[currentFrame].background.c_str());
    voiceClip = LoadSound(Frames[currentFrame].voiceClip.c_str());

    PlaySound(voiceClip);

    playCharacterAnim = true;
}

void SceneTutorial::SceneUpdate(float dt) {
    if (playCharacterAnim) {
        characterAnim.step((int)(dt * 1000.0f));
    }
    if (characterAnim.progress() >= 1.0f && characterAnim.direction() == 1) {
        characterAnim.backward();
    } else if (characterAnim.progress() <= 0.0f && characterAnim.direction() == -1) {
        characterAnim.forward();
        characterAnim.seek(0);
        playCharacterAnim = false;
    }

    if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) {
        currentFrame++;
        if (currentFrame == Frames.size()) {
            Config::SetProperty("tutorialDone", "true");
            if (comingFromOptions) {
                SceneManager::ChangeScene(std::make_unique<SceneOptions>());
            } else {
                SceneManager::ChangeScene(std::make_unique<SceneMain>());
            }
        } else {
            StopSound(voiceClip);
            UnloadTexture(background);
            UnloadTexture(speakingSprite);
            UnloadTexture(idleSprite);
            UnloadSound(voiceClip);

            speakingSprite = LoadTexture(Frames[currentFrame].speakingSprite.c_str());
            idleSprite = LoadTexture(Frames[currentFrame].idleSprite.c_str());
            background = LoadTexture(Frames[currentFrame].background.c_str());
            voiceClip = LoadSound(Frames[currentFrame].voiceClip.c_str());

            PlaySound(voiceClip);
            playCharacterAnim = true;
        }
    }
}

void SceneTutorial::SceneDraw() {
    DrawTextureEx(background, {(GetScreenWidth() - speakingSprite.width)/2 +
                 speakingSprite.width - background.width/2,
                  (GetScreenHeight() - textbox.height)/2 - background.height/2}, 0, 1, WHITE);
    DrawTextureEx(textbox, {0, GetScreenHeight() - textbox.height}, 0, 1, WHITE);

    if (IsSoundPlaying(voiceClip)) {
        DrawTextureEx(speakingSprite, { 10, GetScreenHeight() - speakingSprite.height - characterAnim.peek()}, 0, 1, WHITE);
    } else {
        DrawTextureEx(idleSprite, { 10, GetScreenHeight() - idleSprite.height - characterAnim.peek()}, 0, 1, WHITE);
    }

    int lineOffset = 0;
    Vector2 textSize = MeasureTextEx(mainFont, Frames[currentFrame].text.c_str(), 32, 0);
    DrawTextOutlineEx(mainFont, Frames[currentFrame].text.c_str(), {speakingSprite.width + 10, GetScreenHeight() - textbox.height + 25}, {0, 0}, 32, 0, WHITE, BLACK, 2);
    textSize = MeasureTextEx(mainFont, "Naciśnij (A), aby kontynuować", 22, 0);
    DrawTextOutlineEx(mainFont, "Naciśnij (A), aby kontynuować", {483, GetScreenHeight() - textSize.y}, {0, 0}, 22, 0, {255, 204, 0, 255}, BLACK, 1);
}

void SceneTutorial::SceneExit() {
    UnloadTexture(speakingSprite);
    UnloadTexture(idleSprite);
    UnloadTexture(background);
    UnloadTexture(textbox);
    StopSound(voiceClip);
    UnloadSound(voiceClip);
    UnloadFont(mainFont);
}