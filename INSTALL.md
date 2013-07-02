# ---  INSTALL for Carla  ---

To install Carla, simply run as usual: <br/>
`$ make` <br/>
`$ [sudo] make install`

You can run it without installing, by using instead: <br/>
`$ make` <br/>
`$ python3 source/carla.py`

Packagers can make use of the 'PREFIX' and 'DESTDIR' variable during install, like this: <br/>
`$ make install PREFIX=/usr DESTDIR=./test-dir`

<br/>

===== BUILD DEPENDENCIES =====
--------------------------------
The required build dependencies are: <i>(devel packages of these)</i>

 - liblo
 - Qt4
 - PyQt4

Optional for extended LV2 UIs support:

 - Gtk2
 - Gtk3
 - Qt5

Optional for extra samplers support:

 - FluidSynth
 - LinuxSampler

Optional for extra native plugins:
 - OpenGL
 - NTK
 - libsmf
 - libsndfile
 - ffmpeg/libav
 - fftw3
 - mxml
 - zlib

To run Carla-Control, you'll additionally need:

 - python3-liblo

Optional but recommended:

 - python3-rdflib (for LADSPA-RDF support)


You can use: <br/>
`$ make features` <br/>
To find out which dependencies are missing.


<br/>
