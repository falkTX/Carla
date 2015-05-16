export CFLAGS=-I/opt/kxstudio/include
export CXXFLAGS=-I/opt/kxstudio/include
export LDFLAGS=-L/opt/kxstudio/lib
export PATH=/opt/kxstudio/bin:$PATH
export PKG_CONFIG_PATH=/opt/kxstudio/lib/pkgconfig:$PKG_CONFIG_PATH

make features
make EXPERIMENTAL_PLUGINS=true
