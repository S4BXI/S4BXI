cmake_minimum_required(VERSION 3.9)
project(S4BXI VERSION 4.2.0 LANGUAGES CXX)

set(LIBNAME "portals") 

set(DEFAULT_BUILD_TYPE "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall -Wextra")
# set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address -O0 -Wall -Wextra")
# set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

if(NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "/opt/s4bxi")
endif()

if(NOT CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
endif()

if(NOT CMAKE_INSTALL_BINDIR)
    set(CMAKE_INSTALL_BINDIR "${CMAKE_INSTALL_PREFIX}/bin")
endif()

if(NOT CMAKE_INSTALL_INCLUDEDIR)
    set(CMAKE_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include")
endif()

if (NOT CMAKE_VERSION VERSION_LESS 3.11)
    add_compile_definitions(COMPILING_SIMULATOR)
else ()
    add_definitions(-DCOMPILING_SIMULATOR)
endif()

if(BUILD_MPI_MIDDLEWARE)
    if (NOT CMAKE_VERSION VERSION_LESS 3.11)
        add_compile_definitions(BUILD_MPI_MIDDLEWARE)
    else ()
        add_definitions(-DBUILD_MPI_MIDDLEWARE)
    endif()
endif()

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_language(C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_library(DL_LIBRARY dl)

# Configure lib

FILE(GLOB ACTORS_CPP_SOURCES ${CMAKE_SOURCE_DIR}/src/actors/*.cpp)
FILE(GLOB S4PTL_CPP_SOURCES ${CMAKE_SOURCE_DIR}/src/s4ptl/*.cpp)

set(SOURCE_FILES
        ${ACTORS_CPP_SOURCES}
        ${S4PTL_CPP_SOURCES}
        src/BxiEngine.cpp
        src/BxiQueue.cpp
        src/BxiNode.cpp
        src/ptl_str.cpp
        src/s4bxi_c_util.cpp
        src/portals4.cpp
        src/s4bxi_util.cpp
        src/s4bxi_redefine.cpp
        src/s4bxi_mailbox_pool.cpp
        src/s4bxi_bench.cpp
        src/s4bxi_sample.cpp
        src/plugins/BxiActorExt.cpp)

if(BUILD_MPI_MIDDLEWARE)
    list(APPEND SOURCE_FILES 
            src/mpi_middleware/s4bxi_mpi_middleware.cpp 
            src/mpi_middleware/BxiMpiComm.cpp 
            src/s4bxi_shared.cpp)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
find_package(SimGrid 3.28 REQUIRED)

include_directories(${SimGrid_INCLUDE_DIR})

# Include directly from SimGrid's code
if(BUILD_MPI_MIDDLEWARE)
    include_directories(${SimGrid_SOURCE})
    include_directories(${SimGrid_SOURCE}/include)
    include_directories(${SimGrid_SOURCE}/src/include)
    include_directories(${SimGrid_SOURCE}/src/smpi/include)
    configure_file("${SimGrid_SOURCE}/src/internal_config.h.in"    "${SimGrid_SOURCE}/src/internal_config.h.generated"    @ONLY IMMEDIATE)
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SimGrid_SOURCE}/src/internal_config.h.generated ${SimGrid_SOURCE}/src/internal_config.h)
    file(REMOVE ${SimGrid_SOURCE}/src/internal_config.h.generated)
endif()

add_library(${LIBNAME} SHARED ${SOURCE_FILES})
target_link_libraries(${LIBNAME} ${SimGrid_LIBRARY} ${DL_LIBRARY})

target_include_directories(${LIBNAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE src)

set_target_properties(${LIBNAME} PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1)

# Configure s4bximain

add_executable(s4bximain src/privatization_main.cpp)
target_link_libraries(s4bximain ${LIBNAME})

# Configure scripts

file(READ ${CMAKE_HOME_DIRECTORY}/src/scripts/s4bxitools.sh S4BXITOOLS_SH) # Definitions shared amongst all S4BXI scripts, inlined in each of them

set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(includedir "${CMAKE_INSTALL_INCLUDEDIR}")
set(libdir "${CMAKE_INSTALL_LIBDIR}")
set(includeflag "-I${includedir} -I${includedir}/s4bxi")
set(CMAKE_S4BXI_COMMAND "export LD_LIBRARY_PATH=\"${libdir}")
if(NS3_LIBRARY_PATH)
    set(CMAKE_S4BXI_COMMAND "${CMAKE_S4BXI_COMMAND}:${NS3_LIBRARY_PATH}")
endif()
set(CMAKE_S4BXI_COMMAND "${CMAKE_S4BXI_COMMAND}\${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}\"")
set(S4BXIMAIN ${CMAKE_BINARY_DIR}/s4bximain)

foreach(script cc cxx)
    configure_file(${CMAKE_HOME_DIRECTORY}/src/scripts/s4bxi${script}.in ${CMAKE_BINARY_DIR}/s4bxi${script} @ONLY)
endforeach()

# Install lib

install(TARGETS ${LIBNAME} EXPORT S4BXIConfig
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT S4BXIConfig DESTINATION share/S4BXI/cmake)

# Install s4bximain

install(TARGETS s4bximain RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Install scripts

foreach(script cc cxx)
    file(COPY ${CMAKE_BINARY_DIR}/s4bxi${script}
            DESTINATION ${CMAKE_INSTALL_BINDIR})
endforeach()
