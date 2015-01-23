export PATH=/opt/mingw32/bin:/opt/mingw32/i686-w64-mingw32/bin:$PATH
export AR=i686-w64-mingw32-ar
export CC=i686-w64-mingw32-gcc
export CXX=i686-w64-mingw32-g++
export MOC=i686-w64-mingw32-moc
export RCC=i686-w64-mingw32-rcc
export UIC=i686-w64-mingw32-uic
export STRIP=i686-w64-mingw32-strip
export WINDRES=i686-w64-mingw32-windres

export PKGCONFIG=i686-w64-mingw32-pkg-config
export PKG_CONFIG_PATH=/opt/mingw32/lib/pkgconfig

export CFLAGS="-DBUILDING_CARLA_FOR_WINDOWS -DPTW32_STATIC_LIB -I/opt/mingw32/include"
export CXXFLAGS="$CFLAGS -DFLUIDSYNTH_NOT_A_DLL"

export WIN32=true

make features
make -j4
