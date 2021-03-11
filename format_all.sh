#!/usr/bin/env sh

CLANG="clang-format-9 -i"

# DON'T format .h files, keep them as close to the original BXI ones as possible
$CLANG include/*.hpp
$CLANG include/actors/*.hpp

$CLANG src/*.cpp
$CLANG src/actors/*.cpp
$CLANG src/s4ptl/*.cpp
