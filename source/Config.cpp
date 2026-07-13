//
// Created by void on 05/04/2026.
//

#include "Config.h"

#include <string>
#include "json.hpp"
#include <fstream>
#include <iostream>
#include "Helpers.h"
#include "spdlog/spdlog.h"

void Config::OpenFile(std::string filename) {
    SPDLOG_TRACE("filename: {}", filename);
    nlohmann::json configData;
    std::ifstream file(filename);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        if (nlohmann::json::accept(buffer.str())) {
            configData = nlohmann::json::parse(buffer.str());
        } else {
            SPDLOG_DEBUG("new config file created");
            configData = nlohmann::json::object();
        }
    } else {
        SPDLOG_DEBUG("new config file created");
        configData = nlohmann::json::object();
    }

    ConfigFile newConfig;

    newConfig.background = configData.value("background", 0);
    newConfig.resourcePack = configData.value("resourcePack", "");
    newConfig.tutorialDone = configData.value("tutorialDone", false);
    newConfig.voice = configData.value("voice", "");
    newConfig.language = configData.value("language", "");

    if (configData.contains("parcelNames") && configData["parcelNames"].is_array()) {
        nlohmann::json parcelsArray = configData["parcelNames"];

        for (nlohmann::json parcel : parcelsArray) {
            if (!parcel.contains("parcelNumber") || !parcel["parcelNumber"].is_string() || !parcel.contains("name") || !parcel["name"].is_string()) {
                continue;
            }

            newConfig.parcelNames.insert_or_assign(parcel.value("parcelNumber", ""), parcel.value("name", ""));
        }
    }

    newConfig.filename = filename;
    openedFile = newConfig;
}

void Config::SaveFile() {
    nlohmann::json configData = nlohmann::json::object();

    configData["background"] = openedFile.background;
    configData["resourcePack"] = openedFile.resourcePack;
    configData["tutorialDone"] = openedFile.tutorialDone;
    configData["voice"] = openedFile.voice;
    configData["language"] = openedFile.language;
    configData["parcelNames"] = nlohmann::json::array();

    for (const std::pair<const std::string, std::string>& item : openedFile.parcelNames) {
        nlohmann::json parcelNameData;
        parcelNameData["parcelNumber"] = item.first;
        parcelNameData["name"] = item.second;

        configData["parcelNames"].push_back(parcelNameData);
    }

    if (!disableSavingToSD) {
        SPDLOG_DEBUG("saving to sd: {}", openedFile.filename);
        SPDLOG_TRACE(configData.dump());
        std::ofstream outFile(openedFile.filename);
        if (outFile.is_open()) {
            outFile << configData.dump();
            outFile.close();
        }
    } else {
        SPDLOG_DEBUG("saving to sd disabled");
    }
}


std::string Config::LegacyGetConfigProperty(std::string name) {
    SPDLOG_TRACE("[LEGACY] property: {}", name);
    std::ifstream file("sdmc:/config/switchpost/config.json");
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        if (nlohmann::json::accept(buffer.str())) {
            nlohmann::json config = nlohmann::json::parse(buffer.str());
            if (!config.contains(name) || config[name].is_null()) {
                SPDLOG_DEBUG("property doesnt exist");
                return "";
            } else {
                return config[name];
            }
        } else {
            SPDLOG_ERROR("file invalid");
            return "";
        }
    } else {
        SPDLOG_ERROR("file couldnt be opened");
        return "";
    }
}