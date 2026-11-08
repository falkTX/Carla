# INSTALL for Carla

NOTE: when using MSYS2 on Windows, an additional step is necessary in order
to solve an issue with symbolic links to some dependency folders before build:
```
$ make msys2fix
```

To install Carla, simply run as usual:
```
$ make
$ [sudo] make install
```

You can run it without installing, by using instead:
```
$ make
$ ./source/frontend/carla
```

Packagers can make use of the `PREFIX` and `DESTDIR` variable during install, like this:
```
$ make install PREFIX=/usr DESTDIR=./test-dir
```

## BUILD DEPENDENCIES

There are no required build dependencies. The default build is probably not what you want though.

If you want the frontend (which is likely), you will need PyQt5 or PyQt6 (python3 version).

You likely will also want:

 - libmagic/file (for auto-detection of binary types, needed for plugin-bridges)
 - liblo         (for OSC support, also a requirement for DSSI UIs)

Optional for extra Linux-only engine features:

 - ALSA
 - PulseAudio
 - X11 (CLAP/LV2/VST2/VST3 X11 UI support)

Optional for extended LV2 UIs support: (Linux only)

 - Qt4
 - Qt5

NOTE: Gtk2 and Gtk3 support is always on, as Carla uses dlopen + dlsym to support them

Optional for extra samplers support:

 - FluidSynth (SF2/3)

Optional for extra LADSPA plugin information:

 - python3-rdflib


You can use:
```
$ make features
```
To find out which dependencies are missing.


Under Debian based systems, you can use this command to install everything with Qt5 packages:
```
sudo apt install python3-pyqt5.qtsvg python3-rdflib pyqt5-dev-tools \
  libmagic-dev liblo-dev libasound2-dev libpulse-dev libx11-dev libxcursor-dev libxext-dev \
  qtbase5-dev libfluidsynth-dev
```

Under Debian based systems, you can alternatively install everything with Qt6 packages:
```
sudo apt install python3-pyqt6.qtsvg python3-rdflib pyqt6-dev-tools \
  libmagic-dev liblo-dev libasound2-dev libpulse-dev libx11-dev libxcursor-dev libxext-dev \
  qt6-base-dev libfluidsynth-dev
```

Under Fedora, you can use the following command instead:
```
sudo dnf install python3-qt5-devel python3-rdflib \
  file-devel liblo-devel alsa-lib-devel pulseaudio-libs-devel libX11-devel
  qt5-devel fluidsynth-devel libsndfile-devel
```

## BUILD BRIDGES (Experimental)

Carla can make use of plugin bridges to load additional plugin types.

### 32bit plugins on 64bit systems

Simply run `make posix32` after a regular Carla build, and install or run Carla locally.<br/>
This feature requires a compiler capable of building 32bit binaries.

### JACK Applications inside Carla

This is built by default on Linux systems.<br/>
Requires LD_PRELOAD support by the OS and the GCC compiler.<br/>
Does not work with clang. (if you discover why, please let me know!)

### Windows plugins (via Wine)

Requires a mingw compiler, and winegcc.

First, we build the Windows bridges using mingw, like this: (adjust as needed)
```
make win32 CC=i686-w64-mingw32-gcc CXX=i686-w64-mingw32-g++
make win64 CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++
```

To finalize, we build the wine<->native bridges using winegcc:
```
make wine32
make wine64
```

Then install or run Carla locally as usual.<br/>
Don't forget to enable experimental options and plugin bridges in Carla settings to actually be able to use them.
