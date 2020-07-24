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
        fluidsynth2-static \
        mingw-w64 \
        binutils-mingw-w64-i686 \
        binutils-mingw-w64-x86-64 \
        g++-mingw-w64-i686 \
        g++-mingw-w64-x86-64 \
        libwine-development-dev \
        wine64-development-tools

    # Fix for 32bit bridge link
    sudo ln -s /usr/lib/i386-linux-gnu/libX11.so.6 /usr/lib/i386-linux-gnu/libX11.so

elif [ "${TARGET}" = "linux-strict" ]; then
    sudo apt-get install -y \
        g++-10 \
        g++-10-multilib \
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
        fluidsynth2-static

    # Fix for 32bit bridge link
    sudo ln -s /usr/lib/i386-linux-gnu/libX11.so.6 /usr/lib/i386-linux-gnu/libX11.so

elif [ "${TARGET}" = "linux-juce-strict" ]; then
    sudo apt-get install -y \
        g++-10 \
        g++-10-multilib \
        pkg-config \
        pyqt5-dev-tools \
        python3-pyqt5.qtsvg \
        python3-rdflib \
        libgtk2.0-dev \
        libgtk-3-dev \
        libqt4-dev \
        qtbase5-dev \
        libasound2-dev \
        libjack-jackd2-dev \
        libfreetype6-dev \
        libmagic-dev \
        libgl1-mesa-dev \
        libx11-dev \
        libxext-dev \
        libx11-6:i386 \
        libxext6:i386 \
        libfreetype6:i386 \
        liblo-static \
        fluidsynth2-static

    # Fix for 32bit bridge links
    sudo ln -s /usr/lib/i386-linux-gnu/libX11.so.6 /usr/lib/i386-linux-gnu/libX11.so
    sudo ln -s /usr/lib/i386-linux-gnu/libXext.so.6 /usr/lib/i386-linux-gnu/libXext.so
    sudo ln -s /usr/lib/i386-linux-gnu/libfreetype.so.6 /usr/lib/i386-linux-gnu/libfreetype.so

elif [ "${TARGET}" = "macos" ]; then
    sudo apt-get install -y \
        pkg-config \
        apple-x86-setup

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

elif [ "${TARGET}" = "pylint" ]; then
    sudo apt-get install -y \
        pylint3 \
        python3-liblo \
        python3-pyqt5 python3-pyqt5.qtsvg python3-pyqt5.qtopengl python3-rdflib \
        pyqt5-dev-tools \
        qtbase5-dev

fi
