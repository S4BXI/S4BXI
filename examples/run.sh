#!/bin/bash

# Compile example
mkdir -p build
cd build || exit 1
cmake .. && make
cd ..
# Or simply
# s4bxicxx ./hello.cpp -o ./build/libhello.so

# Run simulation
s4bximain ./platform.xml ./deploy.xml ./build/libhello.so hello
