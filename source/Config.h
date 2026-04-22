//
// Created by void on 05/04/2026.
//

#ifndef SWITCHPOST_CONFIG_H
#define SWITCHPOST_CONFIG_H

#include <string>
#include "json.hpp"
#include <fstream>

class Config{
    public:
    static void LoadConfigFile(std::string filename);
    static std::string GetProperty(std::string name);
    static void SetProperty(std::string name, std::string value);
    private:
    static std::string filename;
};


#endif //SWITCHPOST_CONFIG_H
