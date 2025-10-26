#!/usr/bin/env zsh

FILE_DIR="$(dirname "$0")"

cd "$FILE_DIR/.."

scripts/build.sh

FIRST_PROGRAM_ARG="$1"

if [[ "$FIRST_PROGRAM_ARG" == "-d" ]]; then
    qemu-system-i386 -m 1G -s -S -cdrom build/cmake-build-debug/out/kernel.iso -boot d
else
    qemu-system-i386 -m 1G -cdrom build/cmake-build-debug/out/kernel.iso -boot d
fi