#!/usr/bin/env zsh

FILE_DIR="$(dirname "$0")"

cd "$FILE_DIR/.."

mkdir -p build/cmake-build-debug
cd build/cmake-build-debug

mkdir -p iso/boot/grub
cp ../../configs/grub.cfg iso/boot/grub/grub.cfg

cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../utils/i686-elf-toolchain.cmake
cmake --build .

cd ../..