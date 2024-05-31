#pragma once

#include "SceneData.h"

struct SDL_Window;

namespace AssimpViewer {

class AssimpViewerApp  {
public:
    AssimpViewerApp(int argc, char *argv[]);
    ~AssimpViewerApp();
    const aiScene *importAssimp(const std::string &path);
    const aiScene *getScene() const;

private:
    std::string mTitle;
    SDL_Window *mWindow;
    SceneData mSceneData;
};

inline const aiScene *AssimpViewerApp::getScene() const {
    return mSceneData.mScene;
}

} // namespace AssimpViewer
