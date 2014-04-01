#!/bin/bash

rm -rf ~/.winepy3
rm -rf ~/.winepy3_x86
rm -rf ~/.winepy3_x64

export WINEARCH=win32
export WINEPREFIX=~/.winepy3_x86
wineboot
regsv32 wineasio.dll
winetricks corefonts
winetricks fontsmooth=rgb

export WINEARCH=win64
export WINEPREFIX=~/.winepy3_x64
wineboot
regsv32 wineasio.dll
wine64 regsv32 wineasio.dll
winetricks corefonts
winetricks fontsmooth=rgb
