#ifndef SWITCHPOST_SCENE_H
#define SWITCHPOST_SCENE_H


class Scene {
public:
    virtual void SceneInit() = 0;
    virtual void SceneDraw() = 0;
    virtual void SceneUpdate(float dt) = 0;
    virtual void SceneExit() = 0;
};


#endif //SWITCHPOST_SCENE_H