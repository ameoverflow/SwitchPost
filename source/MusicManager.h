//
// Created by void on 07/03/2026.
//

#ifndef SWITCHPOST_MUSICMANAGER_H
#define SWITCHPOST_MUSICMANAGER_H

#include "raylib.h"

namespace MusicManager{
    void PlayMusic(const char* file);
    void Update();
    void Stop();
    void Destroy();
    void SetVolume(float volume);
};


#endif //SWITCHPOST_MUSICMANAGER_H
