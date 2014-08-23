# ---  INSTALL for Carla  ---

To install Carla, simply run as usual: <br/>
`$ make` <br/>
`$ [sudo] make install`

You can run it without installing, by using instead: <br/>
`$ make` <br/>
`$ python3 source/carla`

Packagers can make use of the 'PREFIX' and 'DESTDIR' variable during install, like this: <br/>
`$ make install PREFIX=/usr DESTDIR=./test-dir`

<br/>

===== BUILD DEPENDENCIES =====
--------------------------------
The required build dependencies are: <i>(devel packages of these)</i>

 - liblo
 - Qt4/5
 - PyQt4/5 (python3 version)
 - X11 (Linux only)

Optional for extended LV2 UIs support:

 - Gtk2
 - Gtk3
 - Qt4
 - Qt5

Optional for extra samplers support:

 - FluidSynth
 - LinuxSampler

Optional for extra native plugins:
 - NTK
 - fftw3
 - mxml
 - zlib

Optional but recommended:

 - python3-rdflib (for LADSPA-RDF support)


You can use: <br/>
`$ make features` <br/>
To find out which dependencies are missing.


<br/>
