# INSTALL for Carla

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

There are no required build dependencies.

But if you want the frontend (which is likely), you will need PyQt4/5 (python3 version)

 - libmagic (for auto-detection of binary types, needed for plugin-bridges)
 - liblo    (for OSC support)

Optional for extra Linux-only engine features:

 - ALSA
 - PulseAudio
 - X11 (internal/LV2/VST X11 UI support)

Optional for extended LV2 UIs support: (Linux only)

 - Gtk2
 - Gtk3
 - Qt4
 - Qt5

Optional for extra samplers support:

 - FluidSynth (SF2/3)

Optional for extra LADSPA plugin information:

 - python3-rdflib


You can use:
```
$ make features
```
To find out which dependencies are missing.


Under debian based systems, you can use this command to install all dependencies:
```
sudo apt install python3-pyqt5.qtsvg python3-rdflib pyqt5-dev-tools \
  libmagic-dev liblo-dev libasound2-dev libpulse-dev libx11-dev \
  libgtk2.0-dev libgtk-3-dev libqt4-dev qtbase5-dev libfluidsynth-dev
```

## BUILD BRIDGES

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
