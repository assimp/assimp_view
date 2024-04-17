#include "contrib/sokol/sokol_app.h"
#include "contrib/sokol/sokol_gfx.h"
#include "contrib/sokol/sokol_time.h"
#include "contrib/sokol/sokol_glue.h"


#include "AssimpViewerApp.h"
#include "MainRenderView.h"

#include <assimp/postprocess.h>

#include <SDL.h>

namespace AssimpViewer {

static constexpr char Tag[] = "OsreEdApp";

static void createTitleString(const std::string &assetName, std::string &titleString) {
    titleString.clear();
    titleString += "AssimpViewer!";

    titleString += " Asset: ";
    titleString += assetName;
}

AssimpViewerApp::AssimpViewerApp(int argc, char *argv[]) : 
        mTitle(),
        mWindow(nullptr),
        mSceneData() {
    // empty
}

AssimpViewerApp::~AssimpViewerApp() {
    // empty
}

const aiScene *AssimpViewerApp::importAssimp(const std::string &path) {
    return nullptr;
}

void AssimpViewerApp::renderFrame() {
}

} // namespace AssimpViewer
