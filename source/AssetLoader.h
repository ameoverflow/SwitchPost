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
class AssetLoader{
public:
    static std::string ResolveResource(std::string path);
    static bool SetResourcePack(std::string path);
    static std::unordered_map<std::string, ResourcePack> RegisteredPacks;
    static void ResolvePacks();
private:
    static ResourcePack* SelectedPack;
};


#endif //SWITCHPOST_ASSETLOADER_H
