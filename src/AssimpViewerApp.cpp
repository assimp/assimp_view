#pragma once

#include "AssimpViewerApp.h"

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
        mTitle(),
        mWindowsRect(), 
        mIds(),
        mTransformMatrix(),
        mEntity(nullptr),
        mAssimpWrapper(nullptr),
        mKeyboardTransCtrl(nullptr),
        mSceneData() {
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
    Entity *entity = nullptr;
    const aiScene *scene = importAssimp(modelLoc.getAbsPath(), getIdContainer(), world, &entity);    
    if (entity == nullptr) {
        return;
    }
    mSceneData.mScene = scene;

    RenderBackendService *rbSrv = ServiceProvider::getService<RenderBackendService>(ServiceType::RenderService);
    if (nullptr == rbSrv) {
        return;
    }

    rootWindow->getWindowsRect(mWindowsRect);
    Entity *camEntity = new Entity(std::string("camera_1"), *getIdContainer(), world);
    Camera *camera = (Camera *)camEntity->createComponent(ComponentType::CameraComponentType);
    world->setActiveCamera(camera);
    mSceneData.mCamera = camera;
    mSceneData.mCamera->setProjectionParameters(
        60.f, (f32)mWindowsRect.width, (f32)mWindowsRect.height, 0.01f, 1000.f);

    world->addEntity(entity);
    mSceneData.mCamera->observeBoundingBox(entity->getAABB());
    mSceneData.m_modelNode = entity->getNode();

    createTitleString(modelLoc.getResource(), mTitle);
    rootWindow->setWindowsTitle(mTitle);
}

void AssimpViewerApp::clearScene() {
    delete mAssimpWrapper;
    mAssimpWrapper = nullptr;
}

const aiScene *AssimpViewerApp::importAssimp(const std::string &path, Ids *ids, World *world, Entity **entity) {
    IO::Uri loc(path);
    if (mAssimpWrapper != nullptr) {
        clearScene();
    }
    mAssimpWrapper = new AssimpWrapper(*ids, world);
    if (!mAssimpWrapper->importAsset(loc, 0)) {
        return nullptr;
    }
    *entity = mAssimpWrapper->getEntity();
    return mAssimpWrapper->getScene();
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
    /* RenderBackend::Mesh *mesh = meshBuilder.createCube(VertexType::ColorVertex, .5, .5, .5, BufferAccessType::ReadOnly).getMesh();
    if (nullptr != mesh) {
        RenderComponent *rc = (RenderComponent *)mEntity->getComponent(ComponentType::RenderComponentType);
        rc->addStaticMesh(mesh);

        Time dt;
        world->update(dt);
        camera->observeBoundingBox(mEntity->getAABB());
    }*/
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
