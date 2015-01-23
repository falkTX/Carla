export PATH=/opt/mingw64/bin:/opt/mingw64/x86_64-w64-mingw32/bin:$PATH
export AR=x86_64-w64-mingw32-ar
export CC=x86_64-w64-mingw32-gcc
export CXX=x86_64-w64-mingw32-g++
export MOC=x86_64-w64-mingw32-moc
export RCC=x86_64-w64-mingw32-rcc
export UIC=x86_64-w64-mingw32-uic
export STRIP=x86_64-w64-mingw32-strip
export WINDRES=x86_64-w64-mingw32-windres

export PKGCONFIG=x86_64-w64-mingw32-pkg-config
export PKG_CONFIG_PATH=/opt/mingw64/lib/pkgconfig

export CFLAGS="-DBUILDING_CARLA_FOR_WINDOWS -DPTW32_STATIC_LIB -I/opt/mingw64/include"
export CXXFLAGS="$CFLAGS -DFLUIDSYNTH_NOT_A_DLL"

export WIN32=true
export WIN64=true

make features
make -j4
