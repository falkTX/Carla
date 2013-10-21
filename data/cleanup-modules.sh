#!/bin/bash

if [ ! -f Makefile ]; then
  echo "Needs to be ran from the source root folder"
  exit 1
fi

set -e

cd source/modules

mv lilv/config lilv_config

rm -f */*.in
rm -f */configure
rm -f */configure.ac
rm -f */juce_module_info
rm -f */install
rm -f */readme
rm -rf */config
rm -rf */contrib
rm -rf */doc
rm -rf */lib
rm -rf */msw
rm -rf */tests

rm -f */*/INSTALL
rm -f */*/NEWS
rm -f */*/PACKAGING
rm -f */*/README
rm -f */*/*.in
rm -f */*/*.ttl
rm -f */*/waf
rm -f */*/wscript
rm -rf */*/bindings
rm -rf */*/doc
rm -rf */*/test
rm -rf */*/tests
rm -rf */*/utils

mv lilv_config lilv/config

cd ../..
