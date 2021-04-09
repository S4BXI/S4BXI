cmake_minimum_required(VERSION 3.9)

project(teshsuite)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../cmake/Modules/")

find_package(SimGrid 3.26 REQUIRED)
find_package(S4BXI REQUIRED)

include_directories(${SimGrid_INCLUDE_DIR})
include_directories(${S4BXI_INCLUDE_DIR})

foreach(x pt2pt_unexpected_headers 
          pt2pt_put_non_matching 
          pt2pt_put_matching 
          pt2pt_offsets 
          pt2pt_get_non_matching 
          pt2pt_get_matching 
          pt2pt_fetch_atomic 
          pt2pt_atomic 
          pt2pt_auto_unlink 
          pt2pt_counters
          pt2pt_l2p)
  add_library          (${x} SHARED ${CMAKE_SOURCE_DIR}/_${x}/${x}.cpp)
  # We don't even need to link with S4BXI because of dlopen magic
  # target_link_libraries(${x} ${S4BXI_LIBRARY})
  set_target_properties(${x} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/_${x}/build)
endforeach()