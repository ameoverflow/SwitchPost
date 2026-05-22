//
// Created by void on 05/04/2026.
//

#ifndef SWITCHPOST_CONFIG_H
#define SWITCHPOST_CONFIG_H

#include <string>
#include "json.hpp"
#include <fstream>

namespace Config {
    void LoadConfigFile(std::string filename);
    std::string GetProperty(std::string name);
    void SetProperty(std::string name, std::string value);
};


#endif //SWITCHPOST_CONFIG_H
