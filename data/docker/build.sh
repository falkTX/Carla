#!/bin/bash

cd $(dirname $0)

rm -rf bin32 bin64 *.dll

set -e

mkdir bin32 bin64
docker build -t carla-bridge-win .
docker run -v $PWD:/mnt --rm --entrypoint \
    cp carla-bridge-win:latest \
        /Carla/bin32/CarlaVstShellBridged.dll.so \
        /Carla/bin32/CarlaVstFxShellBridged.dll.so \
        /mnt/bin32
docker run -v $PWD:/mnt --rm --entrypoint \
    cp carla-bridge-win:latest \
        /Carla/bin64/CarlaVstShellBridged.dll.so \
        /Carla/bin64/CarlaVstFxShellBridged.dll.so \
        /mnt/bin64
docker run -v $PWD:/mnt --rm --entrypoint \
    cp carla-bridge-win:latest \
        /Carla/bin/jackbridge-wine32.dll \
        /Carla/bin/jackbridge-wine64.dll \
        /mnt
