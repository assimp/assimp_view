message(STATUS "The CMake version used is ${CMAKE_VERSION}")
cmake_minimum_required(VERSION 3.5...${CMAKE_VERSION})
include("cmake/HunterGate.cmake")

# Todo: we need to solve this more cleaner
IF (WIN32)
    HunterGate(
        URL "https://github.com/cpp-pm/hunter/archive/v0.24.0.tar.gz"
        SHA1 "a3d7f4372b1dcd52faa6ff4a3bd5358e1d0e5efd"
    )
ELSE()
    HunterGate(
        URL "https://github.com/ruslo/hunter/archive/v0.23.111.tar.gz"
        SHA1 "4189bbd869abe4eb69b5303d3f916c03d34dc154"
    )
ENDIF()

project(assimp_view)

hunter_add_package(SDL2)
find_package(SDL2 REQUIRED)

set(osre_folder ${CMAKE_HOME_DIRECTORY}/contrib/osre/)
set(osre_contrib_folder ${osre_folder}/contrib)

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

message({SDL2_LIBRARIES})
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
    ${platform_backend}
)

set(assimpview_main_src
    src/AssimpViewerApp.cpp
    src/AssimpViewerApp.h
    src/SceneData.h
    src/main.cpp

)

set(OSRE_BUILD_ED OFF CACHE BOOL "Disable osre ed")
add_subdirectory(${osre_folder})

SOURCE_GROUP(Main\\                   FILES ${main_src})

ADD_EXECUTABLE( assimp_view
    ${assimpview_imgui_src}    
    ${assimpview_main_src}
)

IF(WIN32)
  target_link_libraries( assimp_view osre Ws2_32 SDL2::SDL2 )
ELSE()
  target_link_libraries( assimp_view osre SDL2::SDL2 )
ENDIF(WIN32)
