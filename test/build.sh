#!/bin/bash
# input paramter:
# empty for release build
# "clean" for cleaning build folder
# "init" for initialising and updating submodules
# "update" for just updating submodules
# "master" for checking out master branch of submodules
# "pull" for pulling submodules

if [[ $1 == "clean" ]]; then
    echo "Cleaning build folder ..."
    rm -rf build
    exit
elif [[ "$1" == "" ]];then
    build_folder=./build/Release
else
    echo "Wrong build option! ./build.sh [clean]"
    exit
fi

# build
echo "Generating build system files ..."
cmake -G Ninja -Wno-dev -DCMAKE_VERBOSE_MAKEFILE=OFF -B$build_folder
echo "Building project ..."
cmake --build $build_folder
