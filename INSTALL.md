# Build Franca on Arch Linux

Use a C and C++ toolchain, Make, pkg-config, and Python with PyQt6 or PyQt5. The build prefers Qt 6 when both are available. On Plasma, Franca uses the Qt platform theme supplied by Plasma Integration. Run `make features` to check which drivers, plugin formats, and editors will be available. For full Linux hosting support, build with liblo, libmagic, FluidSynth, libsndfile, ALSA, PulseAudio, and the X11 and OpenGL libraries.

```sh
make -j"$(nproc)"
make check-franca
./source/frontend/franca
```

For an existing PipeWire setup, use the JACK driver and the same engine mode as your Carla setup. Run `pw-jack ./source/frontend/franca project.carxp` when you need to select PipeWire’s JACK implementation explicitly. Open the Patchbay to connect the rack to your existing virtual mic.

## Wine

To build the 64-bit Windows plugin scanner and bridge alongside the Linux build, use the MinGW-w64 toolchain and Wine with `winegcc` available.

```sh
make -j"$(nproc)" win64 CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++ AR=x86_64-w64-mingw32-ar
make wine64
```

Keep your existing Wine prefix and plugin paths in Preferences. For a fresh configuration, enable experimental features, plugin bridges, and Wine bridges in Preferences, then refresh the plugin list. The paired `win32` and `wine32` recipes require a Wine toolchain with 32-bit Unix development libraries.

## Install

After building, run `make install` with a writable `PREFIX`, or use the privileges required by your chosen prefix. Franca installs one launcher, a desktop entry, and private runtime files. `DESTDIR` is available for staging packages.

```sh
make install PREFIX="$HOME/.local"
"$HOME/.local/bin/franca" project.carxp
```
