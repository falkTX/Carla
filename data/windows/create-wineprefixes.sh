#!/bin/bash

rm -rf ~/.winepy3
rm -rf ~/.winepy3_x86
rm -rf ~/.winepy3_x64

export WINEARCH=win32
export WINEPREFIX=~/.winepy3_x86
wineboot
regsvr32 wineasio.dll
winetricks corefonts
winetricks fontsmooth=rgb

# cd data/windows/python
# msiexec /i python-3.3.5.msi /qn
# wine PyQt4-4.10.4-gpl-Py3.3-Qt4.8.5-x32.exe
# msiexec /i cx_Freeze-4.3.2.win32-py3.3.msi /qn
# cd ../../..

export WINEARCH=win64
export WINEPREFIX=~/.winepy3_x64
wineboot
regsvr32 wineasio.dll
wine64 regsvr32 wineasio.dll
winetricks corefonts
winetricks fontsmooth=rgb

# cd data/windows/python
# msiexec /i python-3.3.5.amd64.msi /qn
# wine PyQt4-4.10.4-gpl-Py3.3-Qt4.8.5-x64.exe
# msiexec /i cx_Freeze-4.3.2.win-amd64-py3.3.msi /qn
# cd ../../..
