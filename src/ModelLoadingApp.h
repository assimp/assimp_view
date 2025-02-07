#pragma once

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

#include "App/App.h"
#include "App/Scene.h"
#include "IO/Uri.h"
#include "Common/BaseMath.h"
#include "Platform/AbstractWindow.h"
#include "Platform/PlatformOperations.h"
#include "Properties/Settings.h"
#include "RenderBackend/RenderBackendService.h"
#include "RenderBackend/RenderCommon.h"
#include "RenderBackend/TransformMatrixBlock.h"

using namespace ::OSRE;
using namespace ::OSRE::App;
using namespace ::OSRE::Common;
using namespace ::OSRE::RenderBackend;

namespace OSRE {
    namespace App {
        class AssimpWrapper;
    } // Namespace App
} // Namespace OSRE


/// @brief The loader application, will create the renderer and loads a model.
class ModelLoadingApp final : public App::AppBase {
public:
    ModelLoadingApp(int argc, char *argv[]);
    ~ModelLoadingApp() override;
    bool hasModel() const;
    void pushIntention();
    void popIntention();
    void checkName(String &name);
    void dumpNode(aiNode &node);
    void showStatistics(const aiScene &scene);

protected:
    void importAsset(const IO::Uri &modelLoc);
    void exportAsset(const IO::Uri &modelLoc);
    void onUpdate() override;

private:
    String mAssetFolder;                    ///< The asset folder, here we will locate our assets.
    App::CameraComponent *mCamera;          ///< The camera component.
    TransformMatrixBlock mTransformMatrix;  ///< The tansform block.
    TransformComponent::NodePtr mModelNode; ///< The mode node.
    int mIntention;
    OSRE::App::AssimpWrapper *mAssimpWrapper;
};
