/*-----------------------------------------------------------------------------------------------
The MIT License (MIT)

Copyright (c) 2015-2024 OSRE ( Open Source Render Engine ) by Kim Kulling

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
-----------------------------------------------------------------------------------------------*/
#include "ModelLoadingApp.h"
#include "App/App.h"
#include "App/Scene.h"
#include "IO/Uri.h"
#include "Common/BaseMath.h"
#include "Platform/AbstractWindow.h"
#include "Platform/PlatformOperations.h"
#include "Properties/Settings.h"
#include "RenderBackend/RenderBackendService.h"
#include "RenderBackend/RenderCommon.h"

#include <assimp/scene.h>
#include <assimp/Exporter.hpp>

using namespace ::OSRE;
using namespace ::OSRE::App;
using namespace ::OSRE::Common;
using namespace ::OSRE::RenderBackend;

/// @brief The log-tag
static constexpr c8 Tag[] = "Assimp-Viewer";

ModelLoadingApp::ModelLoadingApp(int argc, char *argv[]) :
        AppBase(argc, (const char **)argv, "api", "The render API"),
        mAssetFolder(),
        mCamera(nullptr),
        mTransformMatrix(),
        mModelNode(),
        mIntention(0),
        mAssimpWrapper(nullptr)  {
    // empty
}

ModelLoadingApp::~ModelLoadingApp() {
    delete mAssimpWrapper;
}

bool ModelLoadingApp::hasModel() const {
    return mModelNode.isValid();
}

void ModelLoadingApp::pushIntention() {
    mIntention++;
}

void ModelLoadingApp::popIntention() {
    mIntention--;
}

void ModelLoadingApp::checkName(String &name) {
    if (name.empty()) {
        name = "No Name";
    }
}

void ModelLoadingApp::dumpNode(aiNode &node) {
    String name = node.mName.C_Str();
    checkName(name);
    for (int i=0; i<mIntention; ++i) {
        std::cout << "  ";
    }
    
    std::cout << "Node name: " <<  name << "\n";
    for (unsigned int i=0; i < node.mNumChildren; ++i) {
        pushIntention();
        dumpNode(*node.mChildren[i]);
        popIntention();
    }
}

void ModelLoadingApp::showStatistics(const aiScene &scene) {
    std::cout << "Modelname: " << scene.mName.C_Str() << "\n";
    std::cout << "=============================================================\n";

    if (scene.mRootNode != nullptr) {
        dumpNode(*scene.mRootNode);
    }

    std::cout << "Number of meshes    : " << scene.mNumMeshes << "\n";
    std::cout << "Number of materials : " << scene.mNumMaterials << "\n";
}

void ModelLoadingApp::importAsset(const IO::Uri &modelLoc) {
    mAssimpWrapper = new AssimpWrapper(*getIdContainer(), getStage()->getActiveWorld(0));
    if (!mAssimpWrapper->importAsset(modelLoc, 0)) {
        return;
    }

    RenderBackendService *rbSrv = ServiceProvider::getService<RenderBackendService>(ServiceType::RenderService);
    if (nullptr == rbSrv) {
        return;
    }
    Platform::AbstractWindow *rootWindow = getRootWindow();
    if (nullptr == rootWindow) {
        return;
    }

    Rect2ui windowsRect;
    rootWindow->getWindowsRect(windowsRect);
    World *world = getStage()->addActiveWorld("model");
    Entity *entity = mAssimpWrapper->getEntity();
    Entity *camEntity = new Entity("camera", *getIdContainer(), world);
    mCamera = (CameraComponent*)camEntity->createComponent(ComponentType::CameraComponentType);
    mCamera->setProjectionParameters(60.f, (f32)windowsRect.width, (f32)windowsRect.height, 0.01f, 1000.f);
    world->setActiveCamera(mCamera);

    world->addEntity(entity);
    mCamera->observeBoundingBox(entity->getAABB());
    mModelNode = entity->getNode();

    showStatistics(*mAssimpWrapper->getScene());
}

void ModelLoadingApp::exportAsset(const IO::Uri &modelLoc) {
    if (modelLoc.isEmpty()) {
        return;
    }

    if (!hasModel()) {
        return;
    }

    Assimp::Exporter exporter;
    exporter.Export(mAssimpWrapper->getScene(), "obj", modelLoc.getAbsPath().c_str());
}

void ModelLoadingApp::onUpdate() {
    if (AppBase::isKeyPressed(Platform::KEY_I) || AppBase::isKeyPressed(Platform::KEY_i)) {
        IO::Uri modelLoc;
        Platform::PlatformOperations::getFileOpenDialog("Choose asset for import", "*", modelLoc);
        if ( modelLoc.isValid()) {
            importAsset(modelLoc);
        }
    }

    if (AppBase::isKeyPressed(Platform::KEY_E) || AppBase::isKeyPressed(Platform::KEY_e)) {
        IO::Uri modelLoc;
        Platform::PlatformOperations::getFileSaveDialog("Choose asset for export", "*", modelLoc);
        if ( modelLoc.isValid()) {
            exportAsset(modelLoc);
        }
    }

    glm::mat4 rot(1.0);
    if (AppBase::isKeyPressed(Platform::KEY_A) || AppBase::isKeyPressed(Platform::KEY_a)) {
        mTransformMatrix.mModel *= glm::rotate(rot, 0.01f, glm::vec3(1, 0, 0));

    }
    if (AppBase::isKeyPressed(Platform::KEY_S) || AppBase::isKeyPressed(Platform::KEY_s)) {
        mTransformMatrix.mModel *= glm::rotate(rot, -0.01f, glm::vec3(1, 0, 0));
    }

    if (AppBase::isKeyPressed(Platform::KEY_W) || AppBase::isKeyPressed(Platform::KEY_w)) {
        mTransformMatrix.mModel *= glm::rotate(rot, 0.01f, glm::vec3(0, 1, 0));
    }

    if (AppBase::isKeyPressed(Platform::KEY_D) || AppBase::isKeyPressed(Platform::KEY_d)) {
        mTransformMatrix.mModel *= glm::rotate(rot, -0.01f, glm::vec3(0, 1, 0));
    }
    RenderBackendService *rbSrv = ServiceProvider::getService<RenderBackendService>(ServiceType::RenderService);

    rbSrv->beginPass(RenderPass::getPassNameById(RenderPassId));
    rbSrv->beginRenderBatch("b1");

    rbSrv->setMatrix(MatrixType::Model, mTransformMatrix.mModel);

    rbSrv->endRenderBatch();
    rbSrv->endPass();

    AppBase::onUpdate();
}

