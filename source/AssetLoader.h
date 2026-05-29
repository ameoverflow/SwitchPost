//
// Created by void on 26/03/2026.
//

#ifndef SWITCHPOST_ASSETLOADER_H
#define SWITCHPOST_ASSETLOADER_H

#include <string>
#include <unordered_map>
#include <vector>

struct ResourcePack {
    std::string directory, name, author;
    std::vector<std::string> voices;
};

// to support resource packs
namespace AssetLoader {
    std::string ResolveResource(std::string path);
    bool SetResourcePack(std::string path);
    inline std::unordered_map<std::string, ResourcePack> RegisteredPacks;
    void ResolvePacks();
};


#endif //SWITCHPOST_ASSETLOADER_H
