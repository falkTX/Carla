#!/bin/bash

set -e

pylint3 \
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

pylint3 \
    --extension-pkg-whitelist=PyQt5 \
    -E \
    source/frontend/carla_{database,host,skin}.py
