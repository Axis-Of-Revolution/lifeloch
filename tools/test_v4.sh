#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build-host
"${CXX:-g++}" -std=c++17 -O2 -Wall -Wextra -Werror -Iinclude source/dice.cpp source/model.cpp source/storage.cpp source/ui.cpp source/pixel.cpp source/layout.cpp source/theme.cpp source/audio.cpp tests/test_v4.cpp -o build-host/tests-v4
./build-host/tests-v4
