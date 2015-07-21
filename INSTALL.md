# ---  INSTALL for Carla  ---

To install Carla, simply run as usual: <br/>
`$ make` <br/>
`$ [sudo] make install`

You can run it without installing, by using instead: <br/>
`$ make` <br/>
`$ ./source/carla`

Packagers can make use of the 'PREFIX' and 'DESTDIR' variable during install, like this: <br/>
`$ make install PREFIX=/usr DESTDIR=./test-dir`

<br/>

===== BUILD DEPENDENCIES =====
--------------------------------
The required build dependencies are: <i>(devel packages of these)</i>

 - PyQt4/5 (python3 version)

Optional for extra engine features:

 - libmagic (for auto-detection of binary types, needed for plugin-bridges)
 - liblo    (for OSC support)

Optional for extra Linux-only engine features:

 - ALSA
 - PulseAudio
 - X11 (internal/LV2/VST2 X11 UI support)

Optional for extended LV2 UIs support: (Linux only)

 - Gtk2
 - Gtk3
 - Qt4
 - Qt5

Optional for extra samplers support:

 - FluidSynth   (SF2)
 - LinuxSampler (GIG and SFZ)

Optional for extra native plugins:
 - fftw3
 - mxml
 - zlib
 - NTK
 - OpenGL
 - ProjectM

Optional but recommended:

 - python3-rdflib (for LADSPA-RDF support)


You can use: <br/>
`$ make features` <br/>
To find out which dependencies are missing.
