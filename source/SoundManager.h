//
// Created by void on 07/03/2026.
//

#ifndef SWITCHPOST_SOUNDMANAGER_H
#define SWITCHPOST_SOUNDMANAGER_H

#include "raylib.h"

enum SoundInstance {
    ChangeSound,
    CollectedAllSound,
    GoSound
};

namespace SoundManager {
    void PlaySound(SoundInstance sound);
    void Stop();
    void Destroy();
    void Init();
};


#endif //SWITCHPOST_SOUNDMANAGER_H
