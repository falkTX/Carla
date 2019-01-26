# ---  README for Carla - Windows build ---

What is Carla?
---------------

Carla is a fully-featured audio plugin host, with support for many audio drivers and plugin formats.<br>
It's open source and licensed under the GNU General Public License, version 2 or later.

Features
---------

* LADSPA, DSSI, LV2 and VST2 and VST3 plugin formats
* SF2/3 and SFZ sound banks
* Internal audio and midi file player
* Automation of plugin parameters via MIDI CC
* Remote control over OSC
* Rack and Patchbay processing modes, plus Single and Multi-Client if using JACK
* Native audio drivers (ALSA, DirectSound, CoreAudio, etc) and JACK
* Transport controls, sync with JACK Transport or Ableton Link

In experimental phase / work in progress:
* Export any Carla loadable plugin or sound bank as an LV2 plugin
* Plugin bridge support (such as running 32bit plugins on a 64bit Carla, or Windows plugins on Linux)

Carla is also available as an LV2 and VST2 plugin for Windows.
For the LV2 plugin, create the "C:\Program Files\Common Files\LV2" folder (if it does not exist yet), then copy Carla.lv2 into it and restart your LV2 host.
For the VST2 plugin, create the "C:\Program Files\Common Files\VST2" folder (if it does not exist yet), then copy Carla.vst into it and restart your VST2 host.

For a complete and updated description of Carla, please check:
http://kxstudio.linuxaudio.org/carla
