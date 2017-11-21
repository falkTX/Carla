#!/bin/sh

export HAIKU=true
export CC=gcc-x86
export CXX=g++-x86
export MOC_QT4=moc-x86
export RCC_QT4=rcc-x86
export UIC_QT4=uic-x86
export PKG_CONFIG_PATH=/boot/home/config/develop/lib/x86/pkgconfig:/boot/system/non-packaged/lib/pkgconfig

exec "$@"
