#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
: "${DEVKITPRO:?Set DEVKITPRO to the SDK root (for the real libctru keyboard API headers)}"
mkdir -p build-host
"${CXX:-g++}" -std=c++17 -O2 -Wall -Wextra -Werror -D__3DS__ \
  -Iinclude -I"$DEVKITPRO/libctru/include" \
  source/dice.cpp source/model.cpp source/storage.cpp source/ui.cpp source/pixel.cpp source/layout.cpp source/theme.cpp source/audio.cpp source/native_keyboard.cpp \
  tests/test_native_keyboard.cpp -o build-host/native-keyboard-tests
./build-host/native-keyboard-tests
