#!/bin/bash

function BuildApp() {
    echo "Build type: $BUILD_TYPE"
    mkdir -pv build
    conan profile detect --force
    conan install . --output-folder=build --build=missing --settings=build_type=$BUILD_TYPE
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
    cmake --build . -j $(nproc)
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
        BuildApp || exit $?
        cd build
        LB_CONFIG="${3:-lb2/configs/config.yaml}"
        export LB_CONFIG
        ctest --no-compress-output --rerun-failed --output-log-on-failure --output-on-failure
        exit $?
    ;;
    * )
    ;;
esac

