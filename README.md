#![Carla Logo](https://raw.githubusercontent.com/falkTX/Carla/master/resources/48x48/carla.png) Carla Plugin Host
[![Build Status](https://travis-ci.org/falkTX/Carla.png)](https://travis-ci.org/falkTX/Carla)

What is Carla?
---------------

Carla is a fully-featured audio plugin host, with support for many audio drivers and plugin formats.

Features
---------

* LADSPA, DSSI, LV2, VST2, VST3 and AU plugin formats
* GIG, SF2 and SFZ sound banks
* Internal audio and midi file player
* Automation of plugin parameters via MIDI CC
* Full OSC control
* Plugin bridge support (such as running 32bit plugins on a 64bit Carla)
* Rack and Patchbay engine modes, plus Single and Multi-Client if using JACK
* Native audio drivers (ALSA, DirectSound, CoreAudio) and low-latency (ASIO and JACK)

Development Build
---------------

### Build

* Install `pyqt5-dev-tools` and `python3-pyqt5.qtsvg`
* `make`

### Run

* `mkdir dist`
* `make install PREFIX=$(pwd)/dist`
* `export PYTHONPATH=$(pwd)/dist/share/carla:$PYTHONPATH`
* `dist/bin/carla`
