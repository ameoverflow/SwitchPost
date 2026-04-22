//
// Created by void on 05/04/2026.
//

#include "Config.h"

#include <string>
#include "json.hpp"
#include <fstream>
#include <iostream>
#include "spdlog/spdlog.h"

std::string Config::filename;

void Config::LoadConfigFile(std::string path) {
    filename = path;
}

std::string Config::GetProperty(std::string name) {
    SPDLOG_TRACE("filename: {}", filename);
    std::ifstream file(filename);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        SPDLOG_TRACE("config file: {}", buffer.str());
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

void Config::SetProperty(std::string name, std::string value) {
    nlohmann::json configData;
    SPDLOG_TRACE("filename: {}", filename);
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

    SPDLOG_TRACE("config file: {}", configData.dump());
    std::ofstream outFile(filename);
    if (outFile.is_open()) {
        configData[name] = value;
        outFile << configData.dump();
        outFile.close();
        SPDLOG_DEBUG("property {} set to {}", name, value);
    }
}