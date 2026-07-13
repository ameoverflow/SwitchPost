#ifndef SWITCHPOST_SCENEMANAGER_H
#define SWITCHPOST_SCENEMANAGER_H

#include <memory>
#include "Scene.h"

namespace SceneManager {
    void Update(float dt);
    void Draw();
    void ChangeScene(std::unique_ptr<Scene>&& scene);
    void Init(std::unique_ptr<Scene>&& init_scene);
    void Exit();
    std::string Identify();
};


#endif //SWITCHPOST_SCENEMANAGER_H