export PATH=/opt/kxstudio/bin:$PATH
export PKG_CONFIG_PATH=/opt/kxstudio/lib/pkgconfig:$PKG_CONFIG_PATH

make features
make EXPERIMENTAL_PLUGINS=true
