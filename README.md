# ---  README for Carla  ---

Carla is an audio plugin host, with support for many audio drivers and plugin formats.<br/>
It's being developed by falkTX, using C++, Python3 and Qt4.

It has some nice features like automation of parameters via MIDI CCs (and send control outputs back as MIDI too) and full OSC control.<br/>
Currently supports LADSPA (including LRDF), DSSI, LV2, and VST plugin formats, with additional GIG, SF2 and SFZ file support via FluidSynth and LinuxSampler.<br/>
It uses JACK as the default and preferred audio driver, but also supports native system drivers using RtAudio + RtMidi.<br/>
<br/>

Carla-Control is an OSC Control GUI for Carla (you get the OSC address from the Carla's about dialog, and connect to it).<br/>
It supports controlling main UI components (Dry/Wet, Volume and Balance), and all plugins parameters.<br/>
Peak values and control outputs are displayed as well.
