#!/bin/bash


if ! command -v conan &> /dev/null
then
    echo "Installing conan"
    pip install conan
fi

if ! command -v cmake &> /dev/null
then
    echo "Installing conan"
    pip install cmake
fi

mkdir -pv build
conan profile detect --force
conan install . --output-folder=build --build=missing
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
cmake --build . -j $(nproc)

cp -r ../configs ./lb2/
