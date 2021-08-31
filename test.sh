#!/usr/bin/env bash

work_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

${work_dir}/teshsuite/run.sh
OUTPUT=$?

if [ $OUTPUT != 0 ]
then
    exit $OUTPUT
fi
