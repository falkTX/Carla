# INSTALL for Carla

To install Carla, simply run as usual:
```
$ make
$ [sudo] make install
```

You can run it without installing, by using instead:
```
$ make
$ ./source/carla
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
