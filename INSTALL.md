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

 - JACK
 - liblo
 - Qt4
 - PyQt4
 - OpenGL

Optional but recommended:

 - FluidSynth
 - LinuxSampler

Optional for extended LV2 UIs support:

 - Gtk2
 - Gtk3
 - Suil

Optional for native zynaddsubfx plugin:
 - fftw3
 - mxml

On Debian and Ubuntu, use these commands to install all dependencies: <br/>
`$ sudo apt-get install libjack-dev liblo-dev libqt4-dev libfluidsynth-dev qt4-dev-tools` <br/>
`$ sudo apt-get install libgtk2.0-dev libgtk-3-dev libsuil-dev` <br/>
`$ sudo apt-get install libfftw3-dev libmxml-dev` <br/>
`$ sudo apt-get install python3-pyqt4 pyqt4-dev-tools`

NOTE: linuxsampler is not packaged in either Debian or Ubuntu, but it's available in KXStudio. <br/>
<br/>

To run Carla-Control, you'll additionally need:

 - python3-liblo

Optional but recommended:

 - python3-rdflib (for LADSPA-RDF support)

The python version used and tested is python3.2. Older versions won't work! <br/>
After install, Carla will still work on distros with python2 as default, without any additional work.

<br/>
