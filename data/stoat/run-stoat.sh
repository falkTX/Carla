#!/bin/bash

cd $(dirname $0)/../..

# make distclean

export CC=stoat-compile
export CXX=stoat-compile++
export CFLAGS="-DJACKBRIDGE_DIRECT=1 -DSTOAT_TEST_BUILD=1"
export CXXFLAGS=${CFLAGS}
export LDFLAGS="-ljack"

make -j 8 EXTERNAL_PLUGINS=false backend
stoat --recursive build/ -G stoat-output.png -b data/stoat/blacklist.txt -w  data/stoat/whitelist.txt
