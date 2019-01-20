#!/bin/bash

set -e

if [ -d bin ]; then
  cd bin
else
  echo "Please run this script from the root folder"
  exit
fi

rm -rf *.vst/

PLUGINS=$(ls | grep "Carla" | grep "dylib")

for i in ${PLUGINS}; do
  FILE=$(echo $i | awk 'sub(".dylib","")')
  cp -r ../data/macos/plugin.vst ${FILE}.vst
  cp $i ${FILE}.vst/Contents/MacOS/${FILE}
  rm -f ${FILE}.vst/Contents/MacOS/deleteme
  sed -i -e "s/X-PROJECTNAME-X/${FILE}/" ${FILE}.vst/Contents/Info.plist
  rm -f ${FILE}.vst/Contents/Info.plist-e
done

cd ..
