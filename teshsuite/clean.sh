#!/bin/bash

rm -rf build

for d in _*/ ; do
    rm -rf ${d}build || true
done
