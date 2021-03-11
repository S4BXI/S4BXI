#!/usr/bin/env sh

cd teshsuite || exit 1
./run.sh
OUTPUT=$?
cd ..
if [ $OUTPUT != 0 ]
then
    exit $OUTPUT
fi