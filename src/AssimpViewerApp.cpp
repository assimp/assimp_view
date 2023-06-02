#pragma once

#include "AssimpViewerApp.h"
//#include "Actions/ImportAction.h"

#include <osre/RenderBackend/MeshBuilder.h>
#include <osre/App/Stage.h>
#include <osre/App/TransformController.h>
#include <osre/IO/Uri.h>
#include <osre/App/ServiceProvider.h>
#include <osre/Common/Logger.h>
#include <osre/App/Entity.h>
#include <osre/App/AssimpWrapper.h>

namespace OSRE {
namespace Editor {

using namespace OSRE::App;
using namespace OSRE::RenderBackend;
using namespace OSRE::Common;

static constexpr char Tag[] = "OsreEdApp";

Entity *importAssimp(const std::string &path, Ids *ids, World *world) {
    IO::Uri loc(path);
    AssimpWrapper assimpWrapper(*ids, world);
    if (!assimpWrapper.importAsset(loc, 0)) {
        return nullptr;
    }
    
    return assimpWrapper.getEntity();
}


static void createTitleString(const String &projectName, String &titleString) {
    titleString.clear();
    titleString += "AssimpViewer!";

    titleString += " Project: ";
    titleString += projectName;
}

static Project *createProject(const String &name) {
    Project *project = new App::Project();
    project->setProjectName(name);

    return project;
}

AssimpViewerApp::AssimpViewerApp(int argc, char *argv[]) : 
        AppBase(argc, (const char **)argv, "api", "The render API"),
//        mProject(nullptr), 
        mWindowsRect(), 
        mIds() {
    // empty
}

Camera *AssimpViewerApp::setupCamera(World *world) {
    Entity *camEntity = new Entity("camera", *getIdContainer(), world);
    world->addEntity(camEntity);
    Camera *camera = (Camera *)camEntity->createComponent(ComponentType::CameraComponentType);
    world->setActiveCamera(camera);
    ui32 w, h;
    AppBase::getResolution(w, h);
    camera->setProjectionParameters(60.f, (f32)w, (f32)h, 0.001f, 1000.f);

    return camera;
}

void AssimpViewerApp::loadAsset(const IO::Uri &modelLoc) {
    Platform::AbstractWindow *rootWindow = getRootWindow();
    if (rootWindow == nullptr) {
        return;
    }

    World *world = getStage()->getActiveWorld(0);
    Entity *entity = importAssimp(modelLoc.getAbsPath(), getIdContainer(), world);
    if (entity == nullptr) {
        return;
    }

    RenderBackendService *rbSrv = ServiceProvider::getService<RenderBackendService>(ServiceType::RenderService);
    if (nullptr == rbSrv) {
        return;
    }

//    Rect2ui windowsRect;
    rootWindow->getWindowsRect(mWindowsRect);
    /* if (mProject == nullptr) {
        mProject = createProject(modelLoc.getAbsPath());
    }*/
    Entity *camEntity = new Entity(std::string("camera_1"), *getIdContainer(), world);
    Camera *camera = (Camera *)camEntity->createComponent(ComponentType::CameraComponentType);
    world->setActiveCamera(camera);
    mSceneData.mCamera = camera;
    mSceneData.mCamera->setProjectionParameters(
        60.f, (f32)mWindowsRect.width, (f32)mWindowsRect.height, 0.01f, 1000.f);

    world->addEntity(entity);
    mSceneData.mCamera->observeBoundingBox(entity->getAABB());
    mSceneData.m_modelNode = entity->getNode();

    String asset = modelLoc.getResource();
    //mProject->addAsset(asset);
    String title;
    createTitleString(modelLoc.getResource(), title);
    rootWindow->setWindowsTitle(title);
}

bool AssimpViewerApp::onCreate() {
    if (!AppBase::onCreate()) {
        return false;
    }

    AppBase::setWindowsTitle("Hello-World sample! Rotate with keyboard: w, a, s, d, scroll with q, e");
    World *world = getStage()->addActiveWorld("hello_world");
    mEntity = new Entity("entity", *AppBase::getIdContainer(), world);
    Camera *camera = setupCamera(world);

    MeshBuilder meshBuilder;
    RenderBackend::Mesh *mesh = meshBuilder.createCube(VertexType::ColorVertex, .5, .5, .5, BufferAccessType::ReadOnly).getMesh();
    if (nullptr != mesh) {
        RenderComponent *rc = (RenderComponent *)mEntity->getComponent(ComponentType::RenderComponentType);
        rc->addStaticMesh(mesh);

        Time dt;
        world->update(dt);
        camera->observeBoundingBox(mEntity->getAABB());
    }
    mKeyboardTransCtrl = AppBase::getTransformController(mTransformMatrix);

    osre_info(Tag, "Creation finished.");

    return true;
}

void AssimpViewerApp::onUpdate() {
    Platform::Key key = AppBase::getKeyboardEventListener()->getLastKey();
    mKeyboardTransCtrl->update(TransformController::getKeyBinding(key));

    RenderBackendService *rbSrv = ServiceProvider::getService<RenderBackendService>(ServiceType::RenderService);
    rbSrv->beginPass(RenderPass::getPassNameById(RenderPassId));
    rbSrv->beginRenderBatch("b1");

    rbSrv->setMatrix(MatrixType::Model, mTransformMatrix.m_model);

    rbSrv->endRenderBatch();
    rbSrv->endPass();

    AppBase::onUpdate();
}

} // namespace Editor
} // namespace OSRE
