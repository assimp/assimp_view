# assimp_view
The Asset-Importer-Lib Viewer repository. 

## Build status
[![CMake on multiple platforms](https://github.com/assimp/assimp_view/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/assimp/assimp_view/actions/workflows/cmake-multi-platform.yml)

## How to build the app

First, you have to check out the code

>  git clone --recursive https://github.com/assimp/assimp_view.git

Now change into the repo folder and start the build:

> cd assimp_view
> cmake CMakeLists.txt
> cmake --build .

To run the app go to the bin folder and run it

### Linux
> cd bin/
> ./assimp_view

### Windows
> cd bin\debug
> .\assimp_view.exe
