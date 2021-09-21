#!/usr/bin/env bash

source ./s4bxi_env.sh

build_type=${1:-RELEASE}
install_prefix=${2:-${S4BXI_INSTALL_ROOT}/s4bxi}
CORES=$(grep ^cpu\\scores /proc/cpuinfo | uniq |  awk '{print $4}')

# Create build dir
mkdir -p build
cd build || exit 1
rm CMakeCache.txt

# Compile and install
cmake -DBUILD_MPI_MIDDLEWARE=$BUILD_MPI_MIDDLEWARE \
      -DCMAKE_BUILD_TYPE=$build_type \
      -DCMAKE_INSTALL_PREFIX=$install_prefix \
      -DSimGrid_SOURCE="$SIMGRID_SOURCE" \
      -DSimGrid_PATH="${S4BXI_INSTALL_ROOT}/simgrid" .. && \
make -j $CORES && \
make install
OUTPUT=$?

cd ..

# In case of compilation error, return an appropriate error code
if [ $OUTPUT != 0 ]
then
  echo -e "\e[1;31mCompilation failed :(\e[0m"
  exit $OUTPUT
fi
