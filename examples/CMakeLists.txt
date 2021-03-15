cmake_minimum_required(VERSION 3.9)

project(hello)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../cmake/Modules/")

find_package(SimGrid REQUIRED)
find_package(S4BXI REQUIRED)

include_directories(${SimGrid_INCLUDE_DIR})
include_directories(${S4BXI_INCLUDE_DIR})

add_library(hello SHARED hello.cpp)
