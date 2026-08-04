//
// Created by void on 05/04/2026.
//

#ifndef SWITCHPOST_CONFIG_H
#define SWITCHPOST_CONFIG_H

#include <string>
#include "json.hpp"
#include <fstream>

struct ConfigFile {
    std::unordered_map<std::string, std::string> parcelNames;
    int background;
    std::string resourcePack;
    bool tutorialDone;
    std::string voice;
    std::string filename;
    std::string language;
    bool playMusic;
};

namespace Config {
    inline ConfigFile openedFile;
    void OpenFile(std::string filename);
    void SaveFile();
    std::string LegacyGetConfigProperty(std::string name);
};


#endif //SWITCHPOST_CONFIG_H
