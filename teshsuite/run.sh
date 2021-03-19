#!/bin/bash

CORES=$(grep ^cpu\\scores /proc/cpuinfo | uniq |  awk '{print $4}')

BAD_OUTPUT=0

mkdir -p build
cd build || exit 1

cmake .. && make -j $CORES
OUTPUT=$?
if [ $BAD_OUTPUT == 0 ]
then
  BAD_OUTPUT=$OUTPUT
fi

cd ..

if [ $BAD_OUTPUT != 0 ]
then
  echo -e "| \e[1;31mCompilation failed :(\e[0m"
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
      echo -e "| \e[1;32mTest $d passed\e[0m"
    else
      echo -e "| \e[1;31mTest $d failed\e[0m"
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
  echo -e "| \e[1;31mSome tests failed :(\e[0m"
  echo "|"
  exit 142
fi

echo -e "| \e[1;32mAll Tests passed :)\e[0m"
echo "|"
