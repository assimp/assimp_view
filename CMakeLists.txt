message(STATUS "The CMake version used is ${CMAKE_VERSION}")
cmake_minimum_required(VERSION 3.5...${CMAKE_VERSION})

project(assimp_view)

find_package(SDL2 CONFIG REQUIRED)


set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin )

add_subdirectory(contrib/assimp)

INCLUDE_DIRECTORIES( BEFORE
    ${PROJECT_SOURCE_DIR}/contrib/imgui
    ${PROJECT_SOURCE_DIR}/contrib/assimp/include
    ./
    ./src
    ${SDL2_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
)

link_directories(
    ${SDL2_LIBRARIES}
)

set(assimpview_platform_backend 
    contrib/imgui/backends/imgui_impl_sdl2.cpp
    contrib/imgui/backends/imgui_impl_sdl2.h
    contrib/imgui/backends/imgui_impl_opengl3.cpp
    contrib/imgui/backends/imgui_impl_opengl3.h
)

set(assimpview_imgui_src
    contrib/imgui/imconfig.h
    contrib/imgui/imgui.cpp
    contrib/imgui/imgui.h
    contrib/imgui/imgui_demo.cpp
    contrib/imgui/imgui_draw.cpp
    contrib/imgui/imgui_internal.h
    contrib/imgui/imgui_tables.cpp
    contrib/imgui/imgui_widgets.cpp
    contrib/imgui/imstb_rectpack.h
    contrib/imgui/imstb_textedit.h
    contrib/imgui/imstb_truetype.h
    ${assimpview_platform_backend}
)

set(assimpview_main_src
    src/MainRenderView.cpp
    src/MainRenderView.h
    src/AssimpViewerApp.cpp
    src/AssimpViewerApp.h
    src/SceneData.h
    src/main.cpp
)

SOURCE_GROUP(ImGui\\   FILES ${assimpview_imgui_src})
SOURCE_GROUP(Main\\    FILES ${assimpview_main_src})

ADD_EXECUTABLE( assimp_view
    ${assimpview_imgui_src}    
    ${assimpview_main_src}
)

target_link_libraries(assimp_view
	PRIVATE SDL2::SDL2 assimp GL
)
