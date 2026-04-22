#ifndef SWITCHPOST_SCENEMANAGER_H
#define SWITCHPOST_SCENEMANAGER_H

#include <memory>
#include "Scene.h"

class SceneManager {
public:
    static void Update(float dt);
    static void Draw();
    static void ChangeScene(std::unique_ptr<Scene>&& scene);
    static void Init(std::unique_ptr<Scene>&& init_scene);
    static void Exit();
private:
    static std::unique_ptr<Scene> currentScene;
};


#endif //SWITCHPOST_SCENEMANAGER_H