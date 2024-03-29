#!/bin/bash

go_back_to=$( pwd )
work_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd "${work_dir}" || exit 1

source ../s4bxi_env.sh

CORES=$(grep ^cpu\\scores /proc/cpuinfo | uniq |  awk '{print $4}')

BAD_OUTPUT=0

mkdir -p build
cd build || exit 1

cmake .. -DSimGrid_PATH="${S4BXI_INSTALL_ROOT}/simgrid" -DS4BXI_PATH="${S4BXI_INSTALL_ROOT}/s4bxi" && make -j $CORES
OUTPUT=$?
if [ $BAD_OUTPUT == 0 ]
then
  BAD_OUTPUT=$OUTPUT
fi

cd ..

if [ $BAD_OUTPUT != 0 ]
then
  echo -e "| \033[1;31mCompilation failed :(\033[0m"
  exit $BAD_OUTPUT
fi

for d in _*/ ; do # Run all
    echo "|"
    echo "| ~~~~~~~~~~~~~~~~~~"
    echo "|"
    cd $d || exit 1
    tesh *.tesh
    OUTPUT=$?
    echo "|"
    if [ $OUTPUT == 0 ]
    then
      echo -e "| \033[1;32mTest $d passed\033[0m"
    else
      echo -e "| \033[1;31mTest $d failed\033[0m"
    fi

    if [ $BAD_OUTPUT == 0 ]
    then
      BAD_OUTPUT=$OUTPUT
    fi
    cd ..
done

echo "|"
echo "| ~~~~~~~~~~~~~~~~~~"
echo "|"

if [ $BAD_OUTPUT != 0 ]
then
  echo -e "| \033[1;31mSome tests failed :(\033[0m"
  echo "|"
  exit 142
fi

echo -e "| \033[1;32mAll Tests passed :)\033[0m"
echo "|"

cd "$go_back_to"
