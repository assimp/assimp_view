message(STATUS "The CMake version used is ${CMAKE_VERSION}")
cmake_minimum_required(VERSION 3.5...${CMAKE_VERSION})

project(assimp_view)

find_package(SDL2 CONFIG REQUIRED)

set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/lib )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin )

SET(OSRE_BUILD_ED OFF CACHE BOOL "no osre ed" )
SET(CPPCORE_BUILD_UNITTESTS OFF CACHE BOOL "no cppcore tests" )
SET(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "no assimp tools" )

add_subdirectory(contrib/osre)

INCLUDE_DIRECTORIES( BEFORE
    ./
    src
	contrib/osre/src/Engine
    contrib/osre/contrib/cppcore/include
	${SDL2_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
)

link_directories(
    ${SDL2_LIBRARIES}
)

set(assimpview_main_src
    src/main.cpp
    src/ModelLoadingApp.h
    src/ModelLoading.cpp
    src/SceneData.h
)

SOURCE_GROUP(Main\\    FILES ${assimpview_main_src})

ADD_EXECUTABLE( assimp_view
    ${assimpview_main_src}
)

target_link_libraries(assimp_view
	PRIVATE SDL2::SDL2 assimp osre
)
