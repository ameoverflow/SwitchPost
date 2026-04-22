//
// Created by void on 07/03/2026.
//

#include "MusicManager.h"
#include "raylib.h"
#include "AssetLoader.h"

Music MusicManager::music = { 0 };

void MusicManager::PlayMusic(const char *file) {
    if (music.frameCount) {
        StopMusicStream(music);
        UnloadMusicStream(music);
    }

    music = LoadMusicStream(AssetLoader::ResolveResource(file).c_str());
    music.looping = true;
    PlayMusicStream(music);
}

void MusicManager::Update() {
    if (music.frameCount) {
        UpdateMusicStream(music);
    }
}

void MusicManager::Stop() {
    if (music.frameCount) {
        StopMusicStream(music);
    }
}

void MusicManager::Destroy() {
    if (music.frameCount) {
        StopMusicStream(music);
        UnloadMusicStream(music);
    }
}

void MusicManager::SetVolume(float volume) {
    SetMusicVolume(music, volume);
}
