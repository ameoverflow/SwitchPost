//
// Created by void on 26/03/2026.
//

#include "AssetLoader.h"
#include <string>
#include <filesystem>
#include "spdlog/spdlog.h"
#include <vector>
#include <ranges>
#include <json.hpp>
#include <fstream>
#include <unordered_map>

ResourcePack* AssetLoader::SelectedPack = nullptr;
std::unordered_map<std::string, ResourcePack> AssetLoader::RegisteredPacks = {};

bool AssetLoader::SetResourcePack(std::string path) {
    if (RegisteredPacks.contains(path)) {
        SelectedPack = &RegisteredPacks[path];
        return true;
    } else {
        return false;
    }
}

void AssetLoader::ResolvePacks() {
    std::string root = "sdmc:/config/switchpost/";
    if (!std::filesystem::exists(root)) return;

    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(root)) {
        if (entry.is_directory()) {
            std::filesystem::path configPath = entry.path() / "pack.spmeta";

            if (!std::filesystem::exists(configPath) || !std::filesystem::is_regular_file(configPath)) {
                continue;
            }

            std::ifstream file(configPath.string());
            if (file.is_open()) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                file.close();
                if (nlohmann::json::accept(buffer.str())) {
                    nlohmann::json data = nlohmann::json::parse(buffer.str());
                    ResourcePack pack;
                    pack.directory = entry.path().filename().string();

                    if (!data.contains("name") || data["name"].is_null() ||
                        !data.contains("author") || data["author"].is_null() ||
                        !data.contains("version") || data["version"].is_null() ||
                        data["version"].get<unsigned int>() != 1) {
                        SPDLOG_WARN("pack {} is not valid", entry.path().filename().string());
                        continue;
                    }

                    pack.name = data["name"].get<std::string>();
                    pack.author = data["author"].get<std::string>();

                    std::string voicesPath = "sdmc:/config/switchpost/" + pack.directory + "/voice";
                    if (std::filesystem::exists(voicesPath) && std::filesystem::is_directory(voicesPath)) {
                        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(voicesPath)) {
                            if (std::filesystem::exists(entry.path().string()) && std::filesystem::is_directory(entry.path().string())) pack.voices.push_back(entry.path().filename().string());
                            SPDLOG_DEBUG("added voice {}", entry.path().filename().string());
                        }
                    }

                    RegisteredPacks.insert_or_assign(pack.directory, pack);
                    SPDLOG_INFO("registered pack {}", pack.name);
                } else {
                    SPDLOG_WARN("pack {} is not valid", entry.path().filename().string());
                }
            } else {
                SPDLOG_WARN("pack {} is not valid", entry.path().filename().string());
            }
        }
    }
}

std::string AssetLoader::ResolveResource(std::string path) {
    if (SelectedPack == nullptr) {
        return std::string("romfs:/" + path);
    } else {
        return std::filesystem::exists("sdmc:/config/switchpost/" + SelectedPack->directory + "/" + path) ?
               "sdmc:/config/switchpost/" + SelectedPack->directory + "/" + path :
               "romfs:/" + path;
    }
}