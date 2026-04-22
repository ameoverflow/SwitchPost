#include "SceneManager.h"
#include "Scene.h"

std::unique_ptr<Scene> SceneManager::currentScene;

void SceneManager::Update(float dt) {
    if (currentScene != nullptr) {
        currentScene->SceneUpdate(dt);
    }
}

void SceneManager::Draw() {
    if (currentScene != nullptr) {
        currentScene->SceneDraw();
    }
}

void SceneManager::ChangeScene(std::unique_ptr<Scene>&& nextScene) {
    if (currentScene != nullptr) {
        currentScene->SceneExit();
        currentScene.reset();
    }

    currentScene = std::move(nextScene);

    if (currentScene != nullptr) {
        currentScene->SceneInit();
    }
}

void SceneManager::Init(std::unique_ptr<Scene>&& init_scene) {
    currentScene = std::move(init_scene);

    if (currentScene != nullptr) {
        currentScene->SceneInit();
    }
}

void SceneManager::Exit() {
    if (currentScene != nullptr) {
        currentScene->SceneExit();
    }
}
