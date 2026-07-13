//
// Created by void on 07/03/2026.
//

#include "SoundManager.h"
#include "raylib.h"
#include "AssetLoader.h"

Sound go, collectedAll, change;

void SoundManager::Init() {
    change = LoadSound(AssetLoader::ResolveResource("sounds/change.wav").c_str());
    go = LoadSound(AssetLoader::ResolveResource("sounds/go.wav").c_str());
    collectedAll = LoadSound(AssetLoader::ResolveResource("sounds/collectedall.wav").c_str());
}

void SoundManager::PlaySound(SoundInstance sound) {
    switch (sound) {
        case GoSound: PlaySound(go); break;
        case CollectedAllSound: PlaySound(collectedAll); break;
        case ChangeSound: PlaySound(change); break;
    }
}

void SoundManager::Stop() {
    StopSound(go);
    StopSound(collectedAll);
    StopSound(change);
}

void SoundManager::Destroy() {
    StopSound(go);
    StopSound(collectedAll);
    StopSound(change);
    UnloadSound(go);
    UnloadSound(collectedAll);
    UnloadSound(change);
}