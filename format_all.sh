#!/usr/bin/env sh

CLANG="clang-format -i"

$CLANG include/s4bxi/*.h*
$CLANG include/s4bxi/actors/*.h*

$CLANG src/*.cpp
$CLANG src/actors/*.cpp
$CLANG src/s4ptl/*.cpp
