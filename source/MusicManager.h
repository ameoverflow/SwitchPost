//
// Created by void on 07/03/2026.
//

#ifndef SWITCHPOST_MUSICMANAGER_H
#define SWITCHPOST_MUSICMANAGER_H

#include "raylib.h"

class MusicManager{
public:
    static void PlayMusic(const char* file);
    static void Update();
    static void Stop();
    static void Destroy();
    static void SetVolume(float volume);
private:
    static Music music;
};


#endif //SWITCHPOST_MUSICMANAGER_H
