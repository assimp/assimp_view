message(STATUS "The CMake version used is ${CMAKE_VERSION}")
cmake_minimum_required(VERSION 3.5...${CMAKE_VERSION})


project(assimp_view)

find_package(SDL2 CONFIG REQUIRED)

set(osre_folder ${CMAKE_HOME_DIRECTORY}/contrib/osre/)
set(osre_contrib_folder ${osre_folder}/contrib)

set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin )


INCLUDE_DIRECTORIES( BEFORE
    ${osre_folder}/src
    ${osre_contrib_folder}/src
    ${osre_contrib_folder}/imgui
    ${osre_contrib_folder}/cppcore/include
    ${osre_contrib_folder}/glm
    ${osre_contrib_folder}/stb
    ${osre_contrib_folder}/glew/include
    ${osre_contrib_folder}/assimp/include
    ${osre_contrib_folder}/assimp/contrib/zlib
    ${CMAKE_HOME_DIRECTORY}/contrib/osre/include
    ./
    ./src
    ${SDL2_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
)

link_directories(
    ${osre_folder}/lib
    ${osre_folder}/bin
    ${SDL2_LIBRARIES}
)

set(assimpview_platform_backend 
    ${osre_contrib_folder}/imgui/backends/imgui_impl_sdl2.cpp
    ${osre_contrib_folder}/imgui/backends/imgui_impl_sdl2.h
    ${osre_contrib_folder}/imgui/backends/imgui_impl_opengl3.cpp
    ${osre_contrib_folder}/imgui/backends/imgui_impl_opengl3.h
)

set(assimpview_imgui_src
    ${osre_contrib_folder}/imgui/imconfig.h
    ${osre_contrib_folder}/imgui/imgui.cpp
    ${osre_contrib_folder}/imgui/imgui.h
    ${osre_contrib_folder}/imgui/imgui_demo.cpp
    ${osre_contrib_folder}/imgui/imgui_draw.cpp
    ${osre_contrib_folder}/imgui/imgui_internal.h
    ${osre_contrib_folder}/imgui/imgui_tables.cpp
    ${osre_contrib_folder}/imgui/imgui_widgets.cpp
    ${osre_contrib_folder}/imgui/imstb_rectpack.h
    ${osre_contrib_folder}/imgui/imstb_textedit.h
    ${osre_contrib_folder}/imgui/imstb_truetype.h
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

set(OSRE_BUILD_PLAYER OFF CACHE BOOL "Disable osre player")
set(OSRE_BUILD_ED OFF CACHE BOOL "Disable osre ed")
set(OSRE_BUILD_TESTS OFF CACHE BOOL "Disable osre unittests")
set(OSRE_BUILD_SAMPLES OFF OFF CACHE BOOL "Disable osre samples")
add_subdirectory(${osre_folder})

SOURCE_GROUP(ImGui\\       FILES ${${assimpview_imgui_src}})
SOURCE_GROUP(Main\\        FILES ${assimpview_main_src})

ADD_EXECUTABLE( assimp_view
    ${assimpview_imgui_src}    
    ${assimpview_main_src}
)

target_link_libraries(assimp_view
	PRIVATE SDL2::SDL2 osre 
)

