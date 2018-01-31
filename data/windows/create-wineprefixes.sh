#!/bin/bash

rm -rf ~/.winepy3
rm -rf ~/.winepy3_x32
rm -rf ~/.winepy3_x86
rm -rf ~/.winepy3_x64

export WINEARCH=win32
export WINEPREFIX=~/.winepy3_x32
wineboot
winetricks winxp
winetricks vcrun2010
winetricks corefonts
winetricks fontsmooth=rgb

# cd data/windows/python
# msiexec /i python-3.4.4.msi /qn
# wine cx_Freeze-4.3.4.win32-py3.4.exe
# wine PyQt5-5.5.1-gpl-Py3.4-Qt5.5.1-x32.exe
# cd ../../..

export WINEARCH=win64
export WINEPREFIX=~/.winepy3_x64
wineboot
winetricks win7
winetricks vcrun2010
winetricks corefonts
winetricks fontsmooth=rgb

# cd data/windows/python
# msiexec /i python-3.4.4.amd64.msi /qn
# wine cx_Freeze-4.3.4.win-amd64-py3.4.exe
# wine PyQt5-5.5.1-gpl-Py3.4-Qt5.5.1-x64.exe
# cd ../../..
