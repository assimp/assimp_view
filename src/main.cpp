/*-----------------------------------------------------------------------------------------------
The MIT License (MIT)

Copyright (c) 2015-2023 assimp_view by Kim Kulling

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
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>
#include <iostream>

#include "Engine/Platform/win32/Win32Window.h"

#ifdef main
#undef main
#endif

#define main main

#include <osre/App/App.h>
#include <osre/RenderBackend/RenderCommon.h>
#include <osre/RenderBackend/MeshBuilder.h>
#include <osre/Common/Logger.h>
#include <osre/RenderBackend/RenderBackendService.h>
#include <osre/RenderBackend/TransformMatrixBlock.h>
#include <osre/App/Entity.h>
#include <osre/Platform/AbstractWindow.h>
#include <osre/Common/glm_common.h>
#include <osre/Platform/PlatformOperations.h>
#include <osre/Platform/PlatformInterface.h>

#include "AssimpViewerApp.h"

#include <assimp/scene.h>

using namespace OSRE;
using namespace Assimp;
using namespace OSRE::RenderBackend;
using namespace OSRE::App;
using namespace OSRE::Platform;

using namespace AssimpViewer;

static constexpr c8 Tag[] = "AssimpViewerApp";

void addChildren(aiNode *node) {
    if (node == nullptr) {
        return;
    }

    bool open = false;
    if (ImGui::TreeNode("aiNode")) {
        open = true;
        ImGui::Text(node->mName.C_Str());
        for (size_t i = 0; i < node->mNumChildren; ++i) {
            aiNode *currentNode = node->mChildren[i];
            if (currentNode == nullptr) {
                continue;
            }

            addChildren(currentNode);
        }
    }

    if (open) {
        ImGui::TreePop();
    }
}

void setMenu(bool &newImporter, bool &importAsset, bool &done) {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("New", nullptr, &newImporter);
            ImGui::MenuItem("Import", nullptr, &importAsset);
            ImGui::MenuItem("Quit", nullptr, &done);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void setSceneTree(const aiScene *scene) {
    if (scene == nullptr) {
        return;
    }

    if (scene->mRootNode == nullptr) {
        return;
    }

    aiNode *node = scene->mRootNode;
    addChildren(node);
}

struct PlatformHandle {
#ifdef __APPLE__
#elif __linux__
#elif _WIN32
    HWND hwnd;
    HWND getHandle() const {
        return hwnd;
    }
#endif
};

struct SDLContext {
    SDL_Window *window;
    SDL_GLContext gl_context;
    SDL_SysWMinfo wmInfo;
    const char *glsl_version;
    PlatformHandle handle;
};

using errcode_t = int32_t;

struct Logger {
    static Logger sInstance;
    static Logger &getInstance() {
        return sInstance;
    }

    void logInfo(const std::string &msg) {
        std::cout << "*Inf* : "
                  << msg << "\n";
    }

    void logWarn(const std::string &msg) {
        std::cerr << "*Warn*: "
                  << msg << "\n";
    }

    void logError(const std::string &msg) {
        std::cerr << "*Err* : "
                  << msg << "\n";
    }
};

Logger Logger::sInstance;

errcode_t initSDL(SDLContext &ctx) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
    // GL 3.0 + GLSL 130
    ctx.glsl_version = "#version 130";

#ifdef __APPLE__
    // GL 3.2 Core + GLSL 150
    ctx.glsl_version = "#version 150";
    SDL_GL_SetAttribute( // required on Mac OS
            SDL_GL_CONTEXT_FLAGS,
            SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#elif __linux__
    // GL 3.2 Core + GLSL 150
    ctx.glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#elif _WIN32
    // GL 3.0 + GLSL 130
    ctx.glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    ctx.window = SDL_CreateWindow("Assimp Viewer", 20, 20, 1280, 1024, window_flags);

    ctx.gl_context = SDL_GL_CreateContext(ctx.window);
    SDL_GL_MakeCurrent(ctx.window, ctx.gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    
    SDL_VERSION(&ctx.wmInfo.version);
    SDL_GetWindowWMInfo(ctx.window, &ctx.wmInfo);
    ctx.handle.hwnd = ctx.wmInfo.info.win.window;

    return 0;
}


errcode_t releaseSDL(SDLContext &ctx) {
    SDL_GL_DeleteContext(ctx.gl_context);
    SDL_DestroyWindow(ctx.window);
    SDL_Quit();

    return 0;
}

int main(int argc, char *argv[]) {
    std::cout << "Editor version 0.1\n";
    
    // Setup SDL
    SDLContext ctx;
    if (initSDL(ctx) == -1) {
        Logger::getInstance().logError("Cannot initialize SDL.");
        return -1;
    }

    AssimpViewerApp assimpViewerApp(argc, argv);
    if (!assimpViewerApp.initWindow(400, 25, 800, 600, "test", false, true, App::RenderBackendType::OpenGLRenderBackend)) {
        Logger::getInstance().logError("Cannot initialize window.");
        return -1;
    }
    if (ctx.window == nullptr) {
        Logger::getInstance().logError("Window is nullptr.");
        return -1;
    }
    
    assimpViewerApp.setWindow(ctx.window);

    AbstractWindow *w = assimpViewerApp.getRootWindow();
    if (w == nullptr) {
        Logger::getInstance().logError("OSRE root window is nullptr.");
        return -1;
    }

    Win32Window *win32Win = (Win32Window *)w;
    win32Win->setParent(ctx.handle.hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer back end's
    ImGui_ImplSDL2_InitForOpenGL(ctx.window, ctx.gl_context);
    ImGui_ImplOpenGL3_Init(ctx.glsl_version);

    // Our state
    const ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                done = true;
            }

            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(ctx.window)) {
                    done = true;
                }
                WindowsProperties *p = w->getProperties();
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_RESIZED:
                        if (p != nullptr) {
                            const uint32_t width = event.window.data1;
                            const uint32_t height = event.window.data2;
                            w->resize(p->m_x, p->m_y, width, height);
                        }
                        break;

                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        if (p != nullptr) {
                            const uint32_t width = event.window.data1;
                            const uint32_t height = event.window.data2;
                            w->resize(p->m_x, p->m_y, width, height);
                        }
                        break;

                    default:
                        break;
                }
            } 
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_MenuBar;
        {
            static int counter = 0;
            bool importAsset = false;
            bool newImporter = false;
            bool p_open = true;
            ImGui::Begin("OSRE-Viewer", &p_open, window_flags); 
            setMenu(newImporter, importAsset, done);

            if (importAsset) {
                IO::Uri modelLoc;
                PlatformOperations::getFileOpenDialog("Select asset for import", "*", modelLoc);
                if (modelLoc.isValid()) {
                    assimpViewerApp.loadAsset(modelLoc);
                }
            }

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            if (ImGui::TreeNode("Scene")) {
                App::Stage *stage = assimpViewerApp.getStage();
                if (stage != nullptr) {
                    World *world = stage->getActiveWorld(0);
                }

                const aiScene *scene = assimpViewerApp.getScene();
                setSceneTree(scene);
                ImGui::TreePop();
            }
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(ctx.window);

        assimpViewerApp.handleEvents();
        assimpViewerApp.update();
        assimpViewerApp.requestNextFrame();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (releaseSDL(ctx) == -1) {
        std::cerr << "*Err*: "
                  << "Cannot release SDL.";
        return -1;
    }

    return 0;
}
