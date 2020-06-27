#!/bin/bash

set -e

make -C source/frontend

ln -sf ../patchcanvas source/frontend/widgets/

if which pylint3 >/dev/null; then
  pylint='pylint3'
else
  pylint='pylint'
fi

# widget code, check all errors
${pylint} \
    --extension-pkg-whitelist=PyQt5 \
    --max-attributes=25 \
    --max-line-length=120 \
    --max-locals=25 \
    --max-statements=100 \
    --enable=\
bad-continuation,\
len-as-condition \
    --disable=\
bad-whitespace,\
broad-except,\
fixme,\
invalid-name,\
missing-docstring \
    source/frontend/widgets/canvaspreviewframe.py \
    source/frontend/widgets/racklistwidget.py

# main app code, ignore some errors
${pylint} \
    --extension-pkg-whitelist=PyQt5 \
    --max-line-length=120 \
    --disable=\
bad-whitespace,\
bare-except,\
blacklisted-name,\
duplicate-code,\
fixme,\
invalid-name,\
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

# code not updated yet
${pylint} \
    --extension-pkg-whitelist=PyQt5 \
    -E \
    source/frontend/carla_{database,host,skin}.py
