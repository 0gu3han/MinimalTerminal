#!/bin/sh
# Build (C++ modules need Ninja + Homebrew LLVM on macOS) and run the presentation.
# Usage: ./run.sh [slides.html]
set -e
cd "$(dirname "$0")"

if [ "$(uname)" = "Darwin" ]; then
    L=$(brew --prefix llvm)
    cmake -S . -B build -G Ninja \
        -DCMAKE_CXX_COMPILER="$L/bin/clang++" -DCMAKE_C_COMPILER="$L/bin/clang" > /dev/null
else
    cmake -S . -B build -G Ninja > /dev/null
fi

cmake --build build > /dev/null
exec ./build/dynodeck "${1:-sample/slides.html}"
