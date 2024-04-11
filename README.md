# ![Carla Logo](resources/48x48/carla.png) Carla Plugin Host
[![build](https://github.com/falkTX/Carla/actions/workflows/build.yml/badge.svg)](https://github.com/falkTX/Carla/actions/workflows/build.yml)

What is Carla?
---------------

Carla is a fully-featured audio plugin host, with support for many audio drivers and plugin formats.<br>
It's open source and licensed under the GNU General Public License, version 2 or later.

Features
---------

* LADSPA, DSSI, LV2 and VST2, VST3 and AU plugin formats
* SF2/3 and SFZ sound banks
* Internal audio and midi file player
* Automation of plugin parameters via MIDI CC
* Remote control over [OSC](https://opensoundcontrol.stanford.edu/)
* Rack and Patchbay processing modes, plus Single and Multi-Client if using JACK
* Native audio drivers (ALSA, DirectSound, CoreAudio, etc) and JACK

In experimental phase / work in progress:
* Export any Carla loadable plugin or sound bank as an LV2 plugin
* Plugin bridge support (such as running 32bit plugins on a 64bit Carla, or Windows plugins on Linux)
* Run JACK applications as audio plugins
* Transport controls, sync with JACK Transport or Ableton Link

Carla is also available as an LV2 plugin and VST2 plugin.

Screenshot
----------

![Screenshot](https://kx.studio/screenshots/carla.png)


See the [official webpage](https://kx.studio/Applications:Carla) also.
