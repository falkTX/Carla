#!/bin/bash

# NOTE: You need the following packages installed via MacPorts:
# automake, autoconf, bison, flex, libtool
# p5-libxml-perl, p5-xml-libxml, p7zip, pkgconfig

# ---------------------------------------------------------------------------------------------------------------------
# stop on error

set -e

# ---------------------------------------------------------------------------------------------------------------------
# cd to correct path

if [ -f Makefile ]; then
  cd data/macos
fi

# ---------------------------------------------------------------------------------------------------------------------
# set variables

source common.env

# ---------------------------------------------------------------------------------------------------------------------
# function to remove old stuff

cleanup()
{

rm -rf $TARGETDIR/carla/ $TARGETDIR/carla32/ $TARGETDIR/carla64/
rm -rf cx_Freeze-*
rm -rf Python-*
rm -rf PyQt-*
rm -rf PyQt5_*
rm -rf file-*
rm -rf flac-*
rm -rf fltk-*
rm -rf fluidsynth-*
rm -rf fftw-*
rm -rf gettext-*
rm -rf glib-*
rm -rf libffi-*
rm -rf libgig-*
rm -rf liblo-*
rm -rf libogg-*
rm -rf libsndfile-*
rm -rf libvorbis-*
rm -rf linuxsampler-*
rm -rf mxml-*
rm -rf pkg-config-*
rm -rf pyliblo-*
rm -rf qtbase-*
rm -rf qtmacextras-*
rm -rf qtsvg-*
rm -rf sip-*
rm -rf zlib-*
rm -rf PaxHeaders.*
exit 0

}

# ---------------------------------------------------------------------------------------------------------------------
# function to build base libs

build_base()
{

export CC=clang
export CXX=clang++

export PREFIX=${TARGETDIR}/carla${ARCH}
export PATH=${PREFIX}/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig

export CFLAGS="-O3 -mtune=generic -msse -msse2 -mfpmath=sse -fvisibility=hidden -fdata-sections -ffunction-sections"
export CFLAGS="${CFLAGS} -fPIC -DPIC -DNDEBUG -I${PREFIX}/include -m${ARCH}"
export CXXFLAGS="${CFLAGS} -fvisibility-inlines-hidden"

export LDFLAGS="-fdata-sections -ffunction-sections -Wl,-dead_strip -Wl,-dead_strip_dylibs"
export LDFLAGS="${LDFLAGS} -L${PREFIX}/lib -m${ARCH}"

# ---------------------------------------------------------------------------------------------------------------------
# pkgconfig

if [ ! -d pkg-config-${PKG_CONFIG_VERSION} ]; then
  curl -O https://pkg-config.freedesktop.org/releases/pkg-config-${PKG_CONFIG_VERSION}.tar.gz
  tar -xf pkg-config-${PKG_CONFIG_VERSION}.tar.gz
fi

if [ ! -f pkg-config-${PKG_CONFIG_VERSION}_$ARCH/build-done ]; then
  cp -r pkg-config-${PKG_CONFIG_VERSION} pkg-config-${PKG_CONFIG_VERSION}_$ARCH
  cd pkg-config-${PKG_CONFIG_VERSION}_$ARCH
  ./configure --enable-indirect-deps --with-internal-glib --with-pc-path=$PKG_CONFIG_PATH --prefix=${PREFIX}
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# liblo

if [ ! -d liblo-${LIBLO_VERSION} ]; then
  curl -L http://download.sourceforge.net/liblo/liblo-${LIBLO_VERSION}.tar.gz -o liblo-${LIBLO_VERSION}.tar.gz
  tar -xf liblo-${LIBLO_VERSION}.tar.gz
fi

if [ ! -f liblo-${LIBLO_VERSION}_$ARCH/build-done ]; then
  cp -r liblo-${LIBLO_VERSION} liblo-${LIBLO_VERSION}_$ARCH
  cd liblo-${LIBLO_VERSION}_$ARCH
  ./configure --enable-static --disable-shared --prefix=${PREFIX} \
    --enable-threads \
    --disable-examples --disable-tools
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------

if [ x"${ARCH}" = x"32" ]; then
  return
fi

# ---------------------------------------------------------------------------------------------------------------------
# zlib

if [ ! -d zlib-${ZLIB_VERSION} ]; then
  curl -L https://github.com/madler/zlib/archive/v${ZLIB_VERSION}.tar.gz -o zlib-${ZLIB_VERSION}.tar.gz
  tar -xf zlib-${ZLIB_VERSION}.tar.gz
fi

if [ ! -f zlib-${ZLIB_VERSION}/build-done ]; then
  cd zlib-${ZLIB_VERSION}
  ./configure --static --prefix=${PREFIX}
  make
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# file/magic

if [ ! -d file-${FILE_VERSION} ]; then
  curl -O ftp://ftp.astron.com/pub/file/file-${FILE_VERSION}.tar.gz
  tar -xf file-${FILE_VERSION}.tar.gz
fi

if [ ! -f file-${FILE_VERSION}/build-done ]; then
  cd file-${FILE_VERSION}
  ./configure --enable-static --disable-shared --prefix=${PREFIX}
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# libogg

if [ ! -d libogg-${LIBOGG_VERSION} ]; then
  curl -O https://ftp.osuosl.org/pub/xiph/releases/ogg/libogg-${LIBOGG_VERSION}.tar.gz
  tar -xf libogg-${LIBOGG_VERSION}.tar.gz
fi

if [ ! -f libogg-${LIBOGG_VERSION}/build-done ]; then
  cd libogg-${LIBOGG_VERSION}
  ./configure --enable-static --disable-shared --prefix=${PREFIX}
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# libvorbis

if [ ! -d libvorbis-${LIBVORBIS_VERSION} ]; then
  curl -O https://ftp.osuosl.org/pub/xiph/releases/vorbis/libvorbis-${LIBVORBIS_VERSION}.tar.gz
  tar -xf libvorbis-${LIBVORBIS_VERSION}.tar.gz
fi

if [ ! -f libvorbis-${LIBVORBIS_VERSION}/build-done ]; then
  cd libvorbis-${LIBVORBIS_VERSION}
  ./configure --enable-static --disable-shared --prefix=${PREFIX}
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# flac

if [ ! -d flac-${FLAC_VERSION} ]; then
  curl -O https://svn.xiph.org/releases/flac/flac-${FLAC_VERSION}.tar.xz
  /opt/local/bin/7z x flac-${FLAC_VERSION}.tar.xz
  /opt/local/bin/7z x flac-${FLAC_VERSION}.tar
fi

if [ ! -f flac-${FLAC_VERSION}/build-done ]; then
  cd flac-${FLAC_VERSION}
  chmod +x configure install-sh
  ./configure --enable-static --disable-shared --prefix=${PREFIX} \
    --disable-cpplibs
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# libsndfile

if [ ! -d libsndfile-${LIBSNDFILE_VERSION} ]; then
  curl -O http://www.mega-nerd.com/libsndfile/files/libsndfile-${LIBSNDFILE_VERSION}.tar.gz
  tar -xf libsndfile-${LIBSNDFILE_VERSION}.tar.gz
fi

if [ ! -f libsndfile-${LIBSNDFILE_VERSION}/build-done ]; then
  cd libsndfile-${LIBSNDFILE_VERSION}
  ./configure --enable-static --disable-shared --prefix=${PREFIX} \
    --disable-full-suite --disable-alsa --disable-sqlite
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# libgig

if [ ! -d libgig-${LIBGIG_VERSION} ]; then
  curl -O http://download.linuxsampler.org/packages/libgig-${LIBGIG_VERSION}.tar.bz2
  tar -xf libgig-${LIBGIG_VERSION}.tar.bz2
fi

if [ ! -f libgig-${LIBGIG_VERSION}/build-done ]; then
  cd libgig-${LIBGIG_VERSION}
  if [ ! -f patched ]; then
    patch -p1 -i ../patches/libgig_fix-build.patch
    touch patched
  fi
  ./configure --enable-static --disable-shared --prefix=${PREFIX}
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# linuxsampler

if [ ! -d linuxsampler-${LINUXSAMPLER_VERSION} ]; then
  curl -O http://download.linuxsampler.org/packages/linuxsampler-${LINUXSAMPLER_VERSION}.tar.bz2
  tar -xf linuxsampler-${LINUXSAMPLER_VERSION}.tar.bz2
fi

if [ ! -f linuxsampler-${LINUXSAMPLER_VERSION}/build-done ]; then
  cd linuxsampler-${LINUXSAMPLER_VERSION}
  if [ ! -f patched ]; then
    patch -p1 -i ../patches/linuxsampler_allow-no-drivers-build.patch
    patch -p1 -i ../patches/linuxsampler_disable-ladspa-fx.patch
    sed -i -e "s|HAVE_AU|HAVE_VST|" src/hostplugins/Makefile.am
    touch patched
  fi
  env PATH=/opt/local/bin:$PATH /opt/local/bin/aclocal -I /opt/local/share/aclocal
  env PATH=/opt/local/bin:$PATH /opt/local/bin/glibtoolize --force --copy
  env PATH=/opt/local/bin:$PATH /opt/local/bin/autoheader
  env PATH=/opt/local/bin:$PATH /opt/local/bin/automake --add-missing --copy
  env PATH=/opt/local/bin:$PATH /opt/local/bin/autoconf
  env PATH=/opt/local/bin:$PATH ./configure \
    --enable-static --disable-shared --prefix=${PREFIX} \
    --enable-signed-triang-algo=diharmonic --enable-unsigned-triang-algo=diharmonic --enable-subfragment-size=8 \
    --disable-alsa-driver --disable-arts-driver --disable-jack-driver \
    --disable-asio-driver --disable-midishare-driver --disable-mmemidi-driver \
    --disable-coreaudio-driver --disable-coremidi-driver \
    --disable-instruments-db --disable-sf2-engine
  env PATH=/opt/local/bin:$PATH ./scripts/generate_instrument_script_parser.sh
  sed -i -e "s/bison (GNU Bison) //" config.h
  env PATH=/opt/local/bin:$PATH make ${MAKE_ARGS}
  make install
  sed -i -e "s|-llinuxsampler|-llinuxsampler -L${PREFIX}/lib/libgig -lgig -lsndfile -lFLAC -lvorbisenc -lvorbis -logg -lpthread -lm|" ${PREFIX}/lib/pkgconfig/linuxsampler.pc
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# libffi

if [ ! -d libffi-${LIBFFI_VERSION} ]; then
  curl -O ftp://sourceware.org/pub/libffi/libffi-${LIBFFI_VERSION}.tar.gz
  tar -xf libffi-${LIBFFI_VERSION}.tar.gz
fi

if [ ! -f libffi-${LIBFFI_VERSION}/build-done ]; then
  cd libffi-${LIBFFI_VERSION}
  ./configure --enable-static --disable-shared --prefix=${PREFIX}
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# gettext

if [ ! -d gettext-${GETTEXT_VERSION} ]; then
  curl -O http://ftp.gnu.org/gnu/gettext/gettext-${GETTEXT_VERSION}.tar.gz
  tar -xf gettext-${GETTEXT_VERSION}.tar.gz
fi

if [ ! -f gettext-${GETTEXT_VERSION}/build-done ]; then
  cd gettext-${GETTEXT_VERSION}
  env PATH=/opt/local/bin:$PATH ./configure --enable-static --disable-shared --prefix=${PREFIX}
  env PATH=/opt/local/bin:$PATH make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# glib

if [ ! -d glib-${GLIB_VERSION} ]; then
  curl -O http://caesar.ftp.acc.umu.se/pub/GNOME/sources/glib/${GLIB_MVERSION}/glib-${GLIB_VERSION}.tar.xz
  /opt/local/bin/7z x glib-${GLIB_VERSION}.tar.xz
  /opt/local/bin/7z x glib-${GLIB_VERSION}.tar
fi

if [ ! -f glib-${GLIB_VERSION}/build-done ]; then
  cd glib-${GLIB_VERSION}
  chmod +x configure install-sh
  env PATH=/opt/local/bin:$PATH LDFLAGS="-L${PREFIX}/lib -m${ARCH}" \
    ./configure --enable-static --disable-shared --prefix=${PREFIX}
  env PATH=/opt/local/bin:$PATH make ${MAKE_ARGS} || true
  touch gio/gio-querymodules gio/glib-compile-resources gio/gsettings gio/gdbus gio/gresource gio/gapplication
  env PATH=/opt/local/bin:$PATH make ${MAKE_ARGS}
  touch ${PREFIX}/bin/gtester-report
  env PATH=/opt/local/bin:$PATH make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# fluidsynth

if [ ! -d fluidsynth-${FLUIDSYNTH_VERSION} ]; then
  curl -L https://download.sourceforge.net/fluidsynth/fluidsynth-${FLUIDSYNTH_VERSION}.tar.gz -o fluidsynth-${FLUIDSYNTH_VERSION}.tar.gz
  tar -xf fluidsynth-${FLUIDSYNTH_VERSION}.tar.gz
fi

if [ ! -f fluidsynth-${FLUIDSYNTH_VERSION}/build-done ]; then
  cd fluidsynth-${FLUIDSYNTH_VERSION}
  env LDFLAGS="${LDFLAGS} -framework Carbon -framework CoreFoundation" \
  ./configure --enable-static --disable-shared --prefix=${PREFIX} \
    --enable-libsndfile-support \
    --disable-dbus-support --disable-aufile-support \
    --disable-pulse-support --disable-alsa-support --disable-portaudio-support --disable-oss-support --disable-jack-support \
    --disable-coreaudio --disable-coremidi --disable-dart --disable-lash --disable-ladcca \
    --without-readline
  make ${MAKE_ARGS}
  make install
  sed -i -e "s|-lfluidsynth|-lfluidsynth -lglib-2.0 -lgthread-2.0 -lsndfile -lFLAC -lvorbisenc -lvorbis -logg -lpthread -lm -liconv -lintl|" ${PREFIX}/lib/pkgconfig/fluidsynth.pc
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# mxml

if [ ! -d mxml-${MXML_VERSION} ]; then
  curl -L https://github.com/michaelrsweet/mxml/releases/download/v${MXML_VERSION}/mxml-${MXML_VERSION}.tar.gz -o mxml-${MXML_VERSION}.tar.gz
  mkdir mxml-${MXML_VERSION}
  cd mxml-${MXML_VERSION}
  tar -xf ../mxml-${MXML_VERSION}.tar.gz
  cd ..
fi

if [ ! -f mxml-${MXML_VERSION}/build-done ]; then
  cd mxml-${MXML_VERSION}
  ./configure --disable-shared --prefix=${PREFIX}
  make libmxml.a
  cp *.a    ${PREFIX}/lib/
  cp *.pc   ${PREFIX}/lib/pkgconfig/
  cp mxml.h ${PREFIX}/include/
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# fftw3 (needs to be last as it modifies C[XX]FLAGS)

if [ ! -d fftw-${FFTW3_VERSION} ]; then
  curl -O http://www.fftw.org/fftw-${FFTW3_VERSION}.tar.gz
  tar -xf fftw-${FFTW3_VERSION}.tar.gz
fi

if [ ! -f fftw-${FFTW3_VERSION}/build-done ]; then
  export CFLAGS="${CFLAGS} -ffast-math"
  export CXXFLAGS="${CXXFLAGS} -ffast-math"
  cd fftw-${FFTW3_VERSION}
  ./configure --enable-static --enable-sse2 --disable-shared --disable-debug --prefix=${PREFIX}
  make
  make install
  make clean
  ./configure --enable-static --enable-sse --enable-sse2 --enable-single --disable-shared --disable-debug --prefix=${PREFIX}
  make
  make install
  make clean
  touch build-done
  cd ..
fi

}

# ---------------------------------------------------------------------------------------------------------------------
# build base libs

export ARCH=32
build_base

export ARCH=64
build_base

# ---------------------------------------------------------------------------------------------------------------------
# set flags for qt stuff

export PREFIX=${TARGETDIR}/carla
export PATH=${PREFIX}/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
export PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig
export PKG_CONFIG=${TARGETDIR}/carla64/bin/pkg-config

export CFLAGS="-O3 -mtune=generic -msse -msse2 -mfpmath=sse -fPIC -DPIC -DNDEBUG -I${PREFIX}/include -m64"
export CXXFLAGS=${CFLAGS}
export LDFLAGS="-L${PREFIX}/lib -m64"

export MAKE=/usr/bin/make

export CFG_ARCH=x86_64
export QMAKESPEC=macx-clang

# ---------------------------------------------------------------------------------------------------------------------
# qt5-base download

if [ ! -d qtbase-opensource-src-${QT5_VERSION} ]; then
  curl -L http://download.qt.io/archive/qt/${QT5_MVERSION}/${QT5_VERSION}/submodules/qtbase-opensource-src-${QT5_VERSION}.tar.xz -o qtbase-opensource-src-${QT5_VERSION}.tar.xz
  /opt/local/bin/7z x qtbase-opensource-src-${QT5_VERSION}.tar.xz
  /opt/local/bin/7z x qtbase-opensource-src-${QT5_VERSION}.tar
fi

# ---------------------------------------------------------------------------------------------------------------------
# qt5-base (64bit, shared, framework)

if [ ! -f qtbase-opensource-src-${QT5_VERSION}/build-done ]; then
  cd qtbase-opensource-src-${QT5_VERSION}
  if [ ! -f configured ]; then
    if [ ! -f carla-patched ]; then
      sed -i -e "s|AWK=.*|AWK=/opt/local/bin/gawk|" configure
      patch -p1 -i ../patches/qt55-newosx-fix.patch
      touch carla-patched
    fi
    chmod +x configure
    chmod -R 777 config.tests/unix/
    ./configure -release -shared -opensource -confirm-license -force-pkg-config -platform macx-clang -framework \
                -prefix ${PREFIX} -plugindir ${PREFIX}/lib/qt5/plugins -headerdir ${PREFIX}/include/qt5 \
                -qt-freetype -qt-libjpeg -qt-libpng -qt-pcre -opengl desktop -qpa cocoa \
                -no-directfb -no-eglfs -no-kms -no-linuxfb -no-mtdev -no-xcb -no-xcb-xlib \
                -no-sse3 -no-ssse3 -no-sse4.1 -no-sse4.2 -no-avx -no-avx2 -no-mips_dsp -no-mips_dspr2 \
                -no-cups -no-dbus -no-evdev -no-fontconfig -no-harfbuzz -no-gif -no-glib -no-nis -no-openssl -no-pch -no-sql-ibase -no-sql-odbc \
                -no-audio-backend -no-qml-debug -no-separate-debug-info -no-use-gold-linker \
                -no-compile-examples -nomake examples -nomake tests -make libs -make tools
    touch configured
  fi
  make ${MAKE_ARGS}
  make install
  ln -s ${PREFIX}/lib/QtCore.framework/Headers    ${PREFIX}/include/qt5/QtCore
  ln -s ${PREFIX}/lib/QtGui.framework/Headers     ${PREFIX}/include/qt5/QtGui
  ln -s ${PREFIX}/lib/QtWidgets.framework/Headers ${PREFIX}/include/qt5/QtWidgets
  sed -i -e "s/ -lqtpcre/ /" ${PREFIX}/lib/pkgconfig/Qt5Core.pc
  sed -i -e "s/ '/ /" ${PREFIX}/lib/pkgconfig/Qt5Core.pc
  sed -i -e "s/ '/ /" ${PREFIX}/lib/pkgconfig/Qt5Core.pc
  sed -i -e "s/ '/ /" ${PREFIX}/lib/pkgconfig/Qt5Gui.pc
  sed -i -e "s/ '/ /" ${PREFIX}/lib/pkgconfig/Qt5Gui.pc
  sed -i -e "s/ '/ /" ${PREFIX}/lib/pkgconfig/Qt5Widgets.pc
  sed -i -e "s/ '/ /" ${PREFIX}/lib/pkgconfig/Qt5Widgets.pc
  touch build-done
  cd ..
fi

QT56_ARGS="./configure -prefix ${PREFIX} -plugindir ${PREFIX}/lib/qt5/plugins -headerdir ${PREFIX}/include/qt5 \
                -release -optimized-tools -opensource -confirm-license -c++std c++98 -no-qml-debug -platform macx-clang \
                -no-sse3 -no-ssse3 -no-sse4.1 -no-sse4.2 -no-avx -no-avx2 -no-mips_dsp -no-mips_dspr2 \
                -pkg-config -force-pkg-config \
                -no-mtdev -no-gif -qt-libpng -qt-libjpeg -qt-freetype \
                -no-openssl -no-libproxy \
                -qt-pcre -no-xcb -no-xkbcommon-x11 -no-xkbcommon-evdev -no-xinput2 -no-xcb-xlib -no-glib \
                -no-pulseaudio -no-alsa -no-gtkstyle \
                -make libs -make tools \
                -nomake examples -nomake tests \
                -no-compile-examples \
                -no-cups -no-iconv -no-evdev -no-icu -no-fontconfig \
                -no-dbus -no-xcb -no-eglfs -no-kms -no-gbm -no-directfb -no-linuxfb \
                -qpa cocoa -opengl desktop -framework \
                -no-audio-backend -no-pch
"

QT59_ARGS="./configure -prefix ${PREFIX} -plugindir ${PREFIX}/lib/qt5/plugins -headerdir ${PREFIX}/include/qt5 \
                -opensource -confirm-license -release -strip -shared -framework -platform macx-clang \
                -sse2 -no-sse3 -no-ssse3 -no-sse4.1 -no-sse4.2 -no-avx -no-avx2 -no-avx512 \
                -no-mips_dsp -no-mips_dspr2 \
                -no-pch -pkg-config \
                -make libs -make tools \
                -nomake examples -nomake tests \
                -no-compile-examples \
                -gui -widgets \
                -no-dbus \
                -no-glib -qt-pcre \
                -no-journald -no-syslog -no-slog2 \
                -no-openssl -no-securetransport -no-sctp -no-libproxy \
                -no-cups -no-fontconfig -qt-freetype -no-harfbuzz -no-gtk -opengl desktop -qpa cocoa -no-xcb-xlib \
                -no-directfb -no-eglfs -no-xcb \
                -no-evdev -no-libinput -no-mtdev -no-xinput2 -no-xkbcommon-x11 -no-xkbcommon-evdev \
                -no-gif -no-ico -qt-libpng -qt-libjpeg \
                -qt-sqlite
"

# ---------------------------------------------------------------------------------------------------------------------
# qt5-mac-extras

if [ ! -d qtmacextras-opensource-src-${QT5_VERSION} ]; then
  curl -L http://download.qt.io/archive/qt/${QT5_MVERSION}/${QT5_VERSION}/submodules/qtmacextras-opensource-src-${QT5_VERSION}.tar.xz -o qtmacextras-opensource-src-${QT5_VERSION}.tar.xz
  /opt/local/bin/7z x qtmacextras-opensource-src-${QT5_VERSION}.tar.xz
  /opt/local/bin/7z x qtmacextras-opensource-src-${QT5_VERSION}.tar
fi

if [ ! -f qtmacextras-opensource-src-${QT5_VERSION}/build-done ]; then
  cd qtmacextras-opensource-src-${QT5_VERSION}
  qmake
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# qt5-svg

if [ ! -d qtsvg-opensource-src-${QT5_VERSION} ]; then
  curl -L http://download.qt.io/archive/qt/${QT5_MVERSION}/${QT5_VERSION}/submodules/qtsvg-opensource-src-${QT5_VERSION}.tar.xz -o qtsvg-opensource-src-${QT5_VERSION}.tar.xz
  /opt/local/bin/7z x qtsvg-opensource-src-${QT5_VERSION}.tar.xz
  /opt/local/bin/7z x qtsvg-opensource-src-${QT5_VERSION}.tar
fi

if [ ! -f qtsvg-opensource-src-${QT5_VERSION}/build-done ]; then
  cd qtsvg-opensource-src-${QT5_VERSION}
  qmake
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# python

if [ ! -d Python-${PYTHON_VERSION} ]; then
  curl -O https://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tgz
  tar -xf Python-${PYTHON_VERSION}.tgz
fi

if [ ! -f Python-${PYTHON_VERSION}/build-done ]; then
  cd Python-${PYTHON_VERSION}
  ./configure --prefix=${PREFIX}
  make
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# sip

if [ ! -d sip-${SIP_VERSION} ]; then
  curl -L http://sourceforge.net/projects/pyqt/files/sip/sip-${SIP_VERSION}/sip-${SIP_VERSION}.tar.gz -o sip-${SIP_VERSION}.tar.gz
  tar -xf sip-${SIP_VERSION}.tar.gz
fi

if [ ! -f sip-${SIP_VERSION}/build-done ]; then
  cd sip-${SIP_VERSION}
  python3 configure.py
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# pyqt5

if [ ! -d PyQt-gpl-${PYQT5_VERSION} ]; then
  curl -L http://sourceforge.net/projects/pyqt/files/PyQt5/PyQt-${PYQT5_VERSION}/PyQt-gpl-${PYQT5_VERSION}.tar.gz -o PyQt-gpl-${PYQT5_VERSION}.tar.gz
  tar -xf PyQt-gpl-${PYQT5_VERSION}.tar.gz
fi

if [ ! -f PyQt-gpl-${PYQT5_VERSION}/build-done ]; then
  cd PyQt-gpl-${PYQT5_VERSION}
  python3 configure.py --confirm-license -c
  make ${MAKE_ARGS}
  make install
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# pyliblo

if [ ! -d pyliblo-${PYLIBLO_VERSION} ]; then
  curl -O http://das.nasophon.de/download/pyliblo-${PYLIBLO_VERSION}.tar.gz
  tar -xf pyliblo-${PYLIBLO_VERSION}.tar.gz
fi

if [ ! -f pyliblo-${PYLIBLO_VERSION}/build-done ]; then
  cd pyliblo-${PYLIBLO_VERSION}
  env CFLAGS="${CFLAGS} -I${TARGETDIR}/carla64/include" LDFLAGS="${LDFLAGS} -L${TARGETDIR}/carla64/lib" \
  python3 setup.py build
  python3 setup.py install --prefix=${PREFIX}
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
# cxfreeze

if [ ! -d cx_Freeze-${CXFREEZE_VERSION} ]; then
  curl -L https://github.com/anthony-tuininga/cx_Freeze/archive/${CXFREEZE_VERSION}.tar.gz -o cx_Freeze-${CXFREEZE_VERSION}.tar.gz
  tar -xf cx_Freeze-${CXFREEZE_VERSION}.tar.gz
fi

if [ ! -f cx_Freeze-${CXFREEZE_VERSION}/build-done ]; then
  cd cx_Freeze-${CXFREEZE_VERSION}
  sed -i -e 's/"python%s.%s"/"python%s.%sm"/' setup.py
  python3 setup.py build
  python3 setup.py install --prefix=${PREFIX}
  touch build-done
  cd ..
fi

# ---------------------------------------------------------------------------------------------------------------------
