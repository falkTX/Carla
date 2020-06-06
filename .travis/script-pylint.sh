#!/bin/bash

set -e

make -C source/frontend

if which pylint3 >/dev/null; then
  pylint='pylint3'
else
  pylint='pylint'
fi

${pylint} \
    --extension-pkg-whitelist=PyQt5 \
    --disable=\
bad-whitespace,\
bare-except,\
blacklisted-name,\
duplicate-code,\
fixme,\
invalid-name,\
line-too-long,\
missing-docstring,\
too-few-public-methods,\
too-many-arguments,\
too-many-branches,\
too-many-instance-attributes,\
too-many-lines,\
too-many-locals,\
too-many-public-methods,\
too-many-return-statements,\
too-many-statements,\
unused-argument,\
wrong-import-position \
    source/frontend/carla_{app,backend,backend_qt,settings,shared,utils,widgets}.py

${pylint} \
    --extension-pkg-whitelist=PyQt5 \
    -E \
    source/frontend/carla_{database,host,skin}.py
