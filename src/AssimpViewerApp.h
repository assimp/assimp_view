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

namespace OSRE {
namespace Editor {

class AssimpViewerApp : public App::AppBase {
public:
    AssimpViewerApp(int argc, char *argv[]);
    ~AssimpViewerApp() override = default;
    App::Camera *setupCamera(App::World *world);
    bool onCreate() override;
    void onUpdate() override;
    void loadAsset(const IO::Uri &modelLoc);

private:
    App::Project *mProject;
    Common::Ids mIds;
    RenderBackend::TransformMatrixBlock mTransformMatrix;
    App::Entity *mEntity;
    Animation::AnimationControllerBase *mKeyboardTransCtrl;
    SceneData mSceneData;
};

} // namespace Editor
} // namespace OSRE
