# ![Carla Logo](https://raw.githubusercontent.com/falkTX/Carla/master/resources/48x48/carla.png) Carla Plugin Host
[![Build Status](https://travis-ci.org/falkTX/Carla.png)](https://travis-ci.org/falkTX/Carla)

What is Carla?
---------------

Carla is a fully-featured audio plugin host, with support for many audio drivers and plugin formats.<br>
It's open source and licensed under the GNU General Public License, version 2 or later.

Features
---------

* LADSPA, DSSI, LV2, VST2, VST3 and AU plugin formats
* GIG, SF2 and SFZ sound banks
* Internal audio and midi file player
* Automation of plugin parameters via MIDI CC
* Full OSC control
* Rack and Patchbay engine modes, plus Single and Multi-Client if using JACK
* Native audio drivers (ALSA, DirectSound, CoreAudio) and low-latency (ASIO and JACK)
* Transport sync with JACK or Ableton Link

In experimental phase / work in progress:
* Export any Carla loadable plugin or sound bank as an LV2 plugin
* Plugin bridge support (such as running 32bit plugins on a 64bit Carla, or Windows plugins on Linux)
* Run JACK applications as audio plugins

Carla is also available as an LV2 plugin for MacOS and Linux, and VST plugin for Linux.

Screenshot
----------

![Screenshot](http://kxstudio.linuxaudio.org/screenshots/carla.png)
