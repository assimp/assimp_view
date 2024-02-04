#pragma once

#include "SceneData.h"
#include <osre/Common/Ids.h>
#include <osre/App/AppBase.h>
#include <osre/App/Project.h>
#include <osre/App/CameraComponent.h>
#include <osre/App/Entity.h>
#include <osre/App/World.h>
#include <osre/RenderBackend/RenderCommon.h>
#include <osre/RenderBackend/TransformMatrixBlock.h>
#include <osre/RenderBackend/RenderBackendService.h>

struct SDL_Window;

namespace OSRE {
    namespace App {
        class AssimpWrapper;
    }
} // namespace OSRE

namespace AssimpViewer {

class AssimpViewerApp : public OSRE::App::AppBase {
public:
    AssimpViewerApp(int argc, char *argv[]);
    ~AssimpViewerApp() override = default;
    OSRE::App::Camera *setupCamera(OSRE::App::World *world);
    void setWindow(SDL_Window *window);
    bool onCreate() override;
    void onUpdate() override;
    void loadAsset(const OSRE::IO::Uri &modelLoc);
    const aiScene *getScene() const;
    void clearScene();
    const aiScene *importAssimp(const std::string &path, OSRE::App::World *world, OSRE::App::Entity **entity);
    void renderFrame();

private:
    std::string mTitle;
    SDL_Window *mWindow;
    OSRE::Rect2ui mWindowsRect;
    OSRE::Common::Ids mIds;
    OSRE::RenderBackend::TransformMatrixBlock mTransformMatrix;
    OSRE::App::Entity *mEntity;
    OSRE::App::AssimpWrapper *mAssimpWrapper;
    OSRE::Animation::AnimationControllerBase *mKeyboardTransCtrl;
    SceneData mSceneData;
};

inline const aiScene *AssimpViewerApp::getScene() const {
    return mSceneData.mScene;
}

} // namespace AssimpViewer
