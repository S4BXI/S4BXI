#!/bin/bash

# Compile example
mkdir -p build
cd build || exit 1
cmake .. && make
cd ..

# Run simulation
s4bximain ./platform.xml ./deploy.xml ./build/libhello.so hello
