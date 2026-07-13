//
// Created by void on 08/06/2026.
//

#include "i18n.h"

#include <fstream>
#include "AssetLoader.h"
#include "json.hpp"

std::unordered_map<std::string, std::string> languageMap;
std::string languageCode;

bool i18n::SetLanguage(std::string language) {
    std::ifstream file(AssetLoader::ResolveResource("lang/" + language + ".json"));
    if (!file.is_open()) return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    if (!nlohmann::json::accept(buffer.str())) return false;

    nlohmann::json data = nlohmann::json::parse(buffer.str());

    languageMap.clear();
    for (auto& item : data.items()) {
        languageMap.insert_or_assign(item.key(), item.value().get<std::string>());
    }

    languageCode = language;
    return true;
}

std::string i18n::GetLanguage() {
    return languageCode;
}

std::string i18n::GetString(std::string key) {
    auto it = languageMap.find(key);
    if (it != languageMap.end()) {
        return it->second;
    }
    return key;
}
