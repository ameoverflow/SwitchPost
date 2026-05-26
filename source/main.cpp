#include <cmath>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include "Request.h"
#include "switch.h"
#include "raylib.h"
#include "SceneIntro.h"
#include "SceneManager.h"
#include "Helpers.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <string>
#include "AssetLoader.h"
#include "json.hpp"
#include "MusicManager.h"
#include "Config.h"
#include "spdlog/sinks/basic_file_sink.h"

float bgX = 0;
float bgY = 0;
Music menuMusic;
std::string versionString;

void SpdlogRaylibCallback(int logLevel, const char *text, va_list args) {
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), text, args);

    switch (logLevel) {
        case LOG_TRACE:   SPDLOG_TRACE(buffer); break;
        case LOG_DEBUG:   SPDLOG_DEBUG(buffer); break;
        case LOG_INFO:    SPDLOG_INFO(buffer); break;
        case LOG_WARNING:
            // stfu
            if (std::string(buffer).rfind("FONT", 0) != 0) {
                SPDLOG_WARN(buffer);
            }
            break;
        case LOG_ERROR:   SPDLOG_ERROR(buffer); break;
        case LOG_FATAL:   SPDLOG_CRITICAL(buffer); break;
        default:          SPDLOG_INFO(buffer); break;
    }
}

std::string getLogFileName() {
    std::chrono::local_time localTime = std::chrono::locate_zone("Europe/Warsaw")->to_local(std::chrono::system_clock::now());
    std::chrono::local_days days = std::chrono::floor<std::chrono::days>(localTime);
    std::chrono::year_month_day ymd{days};

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << ymd.year() << "-"
        << std::setw(2) << (unsigned)ymd.month() << "-"
        << ymd.day();

    int count = 1;
    std::string name;

    // find the first available number
    do {
        name = "sdmc:/config/switchpost/logs/" + oss.str() + "-" + std::to_string(count) + ".log";
        count++;
    } while (std::filesystem::exists(name));

    return name;
}

int main()
{
    // switch init shit
    appletLockExit();
    romfsInit();
    std::filesystem::create_directory("sdmc:/config");
    std::filesystem::create_directory("sdmc:/config/switchpost");
    std::filesystem::create_directory("sdmc:/config/switchpost/logs");
    std::filesystem::create_directory("sdmc:/config/switchpost/resourcepacks");
    socketInitializeDefault();

    // log init shit
    std::shared_ptr<spdlog::sinks::sink> consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    std::shared_ptr<spdlog::sinks::sink> fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(getLogFileName().c_str());
    std::vector<std::shared_ptr<spdlog::sinks::sink>> sinks { consoleSink, fileSink };
    std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
    logger->set_pattern("[%T.%e] [%s:%#] [%^%l%$] %v");
    logger->set_level(spdlog::level::trace);
    spdlog::set_default_logger(logger);
    spdlog::flush_on(spdlog::level::trace);

    SPDLOG_INFO("welcome to {}", APP_TITLE);
    SPDLOG_DEBUG("debug mode - have fun");
    fflush(stdout);

    SPDLOG_INFO("reading config file...");
    Config::LoadConfigFile("sdmc:/config/switchpost/config.json");

    SPDLOG_INFO("resolving resource packs...");
    AssetLoader::ResolvePacks();

    std::string resourcePack = Config::GetProperty("resourcePack");
    std::string voice = Config::GetProperty("voice");

    //reset pack to default if it doesnt exist and isnt default
    if (!resourcePack.empty() && !AssetLoader::RegisteredPacks.contains(resourcePack)) {
        SPDLOG_ERROR("resource pack {} not found", resourcePack);
        Config::SetProperty("resourcePack", "");
        resourcePack = "";
    } else if (!resourcePack.empty() && AssetLoader::RegisteredPacks.contains(resourcePack)) {
        AssetLoader::SetResourcePack(resourcePack);
        SPDLOG_INFO("resource pack set to {}", resourcePack);
    }

    // reset voice to none if invalid config (voice doesnt exist in a pack)
    if (!resourcePack.empty()) {
        if (!voice.empty() && voice != "none" && voice != "male" && voice != "female") {
            std::vector<std::string> voicesList = AssetLoader::RegisteredPacks[resourcePack].voices;
            if (std::find(voicesList.begin(), voicesList.end(), voice) == voicesList.end()) {
                Config::SetProperty("voice", "");
                SPDLOG_ERROR("voice {} not found in currently selected pack {}", voice, resourcePack);
                voice = "";
            }
        }
    } else {
        if (!voice.empty() && voice != "none" && voice != "male" && voice != "female") {
            Config::SetProperty("voice", "");
            SPDLOG_ERROR("voice {} not found in default pack", voice);
            voice = "";
        }
    }

    Request::StartThread();

    std::setprecision(17);

    SPDLOG_INFO("setting up window");
    SetTraceLogCallback(SpdlogRaylibCallback);
    InitWindow(1280, 720, APP_TITLE);
    InitAudioDevice();
    SetExitKey(0);
    SetTargetFPS(60);

    backgrounds.push_back(LoadTexture(AssetLoader::ResolveResource("sprites/bg.png").c_str()));
    backgrounds.push_back(LoadTexture(AssetLoader::ResolveResource("sprites/bg2.png").c_str()));
    backgrounds.push_back(LoadTexture(AssetLoader::ResolveResource("sprites/bg10.png").c_str()));
    backgrounds.push_back(LoadTexture(AssetLoader::ResolveResource("sprites/bg3.png").c_str()));
    backgrounds.push_back(LoadTexture(AssetLoader::ResolveResource("sprites/bg4.png").c_str()));
    backgrounds.push_back(LoadTexture(AssetLoader::ResolveResource("sprites/bg7.png").c_str()));
    backgrounds.push_back(LoadTexture(AssetLoader::ResolveResource("sprites/bg9.png").c_str()));
    backgrounds.push_back(LoadTexture(AssetLoader::ResolveResource("sprites/bg5.png").c_str()));
    backgrounds.push_back(LoadTexture(AssetLoader::ResolveResource("sprites/bg6.png").c_str()));
    backgrounds.push_back(LoadTexture(AssetLoader::ResolveResource("sprites/bg8.png").c_str()));

    for (Texture2D background : backgrounds) {
        SetTextureWrap(background, TEXTURE_WRAP_REPEAT);
    }

    std::string bg = Config::GetProperty("background");

    std::from_chars_result toint = std::from_chars(bg.data(), bg.data() + bg.size(), currentBackground);

    if (toint.ec != std::errc()) {
        SPDLOG_ERROR("invalid background selected: {}", currentBackground);
        currentBackground = 0;
        Config::SetProperty("background", "0");
    }

    if (currentBackground > std::size(backgrounds) - 1) {
        SPDLOG_ERROR("invalid background selected: selected background > {}", currentBackground);
        currentBackground = 0;
        Config::SetProperty("background", "0");
    }

    SceneManager::Init(std::make_unique<SceneIntro>());

    while (!WindowShouldClose())
    {
        if (shouldQuit) break;

        float frameTime = GetFrameTime();

        fflush(stdout);

#ifdef DEBUG
        versionString = APP_TITLE;
        versionString += " ";
        versionString += APP_VERSION;
        versionString += " (";
        versionString += std::string(BUILD_TYPE);
        versionString += "), ";
        versionString += std::to_string(GetFPS());
        versionString += " FPS";
#endif

        bgX -= 25 * frameTime;
        bgY -= 25 * frameTime;

        MusicManager::Update();
        SceneManager::Update(frameTime);

        BeginDrawing();

            ClearBackground(BLACK);
            DrawTexturePro(backgrounds[currentBackground], { bgX, bgY, GetScreenWidth() - bgX, GetScreenHeight() - bgY}, {0, 0, GetScreenWidth() - bgX, GetScreenHeight() - bgY}, {0, 0}, 0, WHITE);

            SceneManager::Draw();

#ifdef DEBUG
        DrawTextOutlineEx(GetFontDefault(), versionString.c_str(), {0, 0}, {0, 0}, 24, 2, WHITE, BLACK, 2);

        for (int i = 0; i < GetTouchPointCount(); i++) {
            Vector2 touch = GetTouchPosition(i);
            DrawCircle(touch.x, touch.y, 50, {255, 0, 0, 128});
        }
#endif

        EndDrawing();
    }
    SceneManager::Exit();
    Request::EndThread();

    for (Texture2D background : backgrounds) {
        UnloadTexture(background);
    }
    UnloadMusicStream(menuMusic);

    CloseAudioDevice();

    socketExit();
    romfsExit();

    CloseWindow();
    appletUnlockExit();
    return 0;
}