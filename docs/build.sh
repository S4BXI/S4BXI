#!/bin/bash

WORKING_DIR="$( pwd )"
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

cd $SCRIPT_DIR || exit 1

./m.css/documentation/doxygen.py ./Doxyfile-mcss

cd $WORKING_DIR
