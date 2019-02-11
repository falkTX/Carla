#!/bin/bash

set -e

if [ "${TARGET}" = "linux" ]; then
    sudo apt-get install -y \
        g++-multilib \
        pkg-config \
        pyqt5-dev-tools \
        python3-pyqt5.qtsvg \
        python3-rdflib \
        libgtk2.0-dev \
        libgtk-3-dev \
        libqt4-dev \
        qtbase5-dev \
        libasound2-dev \
        libpulse-dev \
        libmagic-dev \
        libgl1-mesa-dev \
        libx11-dev \
        libx11-6:i386 \
        liblo-static \
        fluidsynth-static \
        mingw32-x-gcc \
        mingw32-x-pkgconfig \
        mingw64-x-gcc \
        mingw64-x-pkgconfig \
        wine-rt-dev

    # Fix for 32bit bridge link
    sudo ln -s /usr/lib/i386-linux-gnu/libX11.so.6 /usr/lib/i386-linux-gnu/libX11.so

elif [ "${TARGET}" = "linux-strict" ]; then
    sudo apt-get install -y \
        g++-8 \
        pkg-config \
        pyqt5-dev-tools \
        python3-pyqt5.qtsvg \
        python3-rdflib \
        libgtk2.0-dev \
        libgtk-3-dev \
        libqt4-dev \
        qtbase5-dev \
        libasound2-dev \
        libpulse-dev \
        libmagic-dev \
        libgl1-mesa-dev \
        libx11-dev \
        libx11-6:i386 \
        liblo-static \
        fluidsynth-static

    # Fix for 32bit bridge link
    sudo ln -s /usr/lib/i386-linux-gnu/libX11.so.6 /usr/lib/i386-linux-gnu/libX11.so

elif [ "${TARGET}" = "linux-juce-strict" ]; then
    sudo apt-get install -y \
        g++-8 \
        pkg-config \
        pyqt5-dev-tools \
        python3-pyqt5.qtsvg \
        python3-rdflib \
        libgtk2.0-dev \
        libgtk-3-dev \
        libqt4-dev \
        qtbase5-dev \
        libasound2-dev \
        libfreetype6-dev \
        libmagic-dev \
        libgl1-mesa-dev \
        libx11-dev \
        libxext-dev \
        liblo-static \
        fluidsynth-static

elif [ "${TARGET}" = "macos" ]; then
    sudo apt-get install -y \
        pkg-config \
        apple-x86-setup

# mkdir /tmp/osx-macports-pkgs
# cd /tmp/osx-macports-pkgs
# wget https://launchpad.net/~kxstudio-team/+archive/ubuntu/builds/+files/apple-macports-flac_1.2.1-1_all.deb
# wget https://launchpad.net/~kxstudio-team/+archive/ubuntu/builds/+files/apple-macports-libiconv_1.14-0_all.deb
# wget https://launchpad.net/~kxstudio-team/+archive/ubuntu/builds/+files/apple-macports-liblo_0.26-1_all.deb
# wget https://launchpad.net/~kxstudio-team/+archive/ubuntu/builds/+files/apple-macports-libogg_1.3.0-1_all.deb
# wget https://launchpad.net/~kxstudio-team/+archive/ubuntu/builds/+files/apple-macports-libsndfile_1.0.25-0_all.deb
# wget https://launchpad.net/~kxstudio-team/+archive/ubuntu/builds/+files/apple-macports-libvorbis_1.3.3-0_all.deb
# cd ~
# rm -r /tmp/osx-macports-pkgs

elif [ "${TARGET}" = "win32" ]; then
    sudo apt-get install -y \
        mingw32-x-gcc \
        mingw32-x-pkgconfig

elif [ "${TARGET}" = "win64" ]; then
    sudo apt-get install -y \
        mingw32-x-gcc \
        mingw32-x-pkgconfig \
        mingw64-x-gcc \
        mingw64-x-pkgconfig

fi

#     mingw32-x-fluidsynth
#     mingw32-x-fftw
#     mingw32-x-liblo
#     mingw32-x-mxml
#     mingw32-x-zlib
#     mingw64-x-fluidsynth
#     mingw64-x-fftw
#     mingw64-x-liblo
#     mingw64-x-mxml
#     mingw64-x-zlib
