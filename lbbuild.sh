#!/bin/bash

set -e

function BuildApp() {
    echo "Build type: $BUILD_TYPE"
    mkdir -pv build
    conan profile detect --force
    conan install . --output-folder=build --build=missing --settings=build_type=$BUILD_TYPE
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
    cmake --build . -j 4
}

if ! command -v conan &> /dev/null
then
    echo "Installing conan"
    pip install conan
fi

if ! command -v cmake &> /dev/null
then
    echo "Installing cmake"
    pip install cmake
fi

CMD=$1

case $CMD in
    build )
        BUILD_TYPE="${2:-Debug}"
        BuildApp
        exit $?
    ;;
    test )
        BUILD_TYPE="${2:-Debug}"
        BuildApp
        ctest --output-on-failure
    ;;
    * )
        echo "Unknown command: $CMD"
        exit 1
    ;;
esac

