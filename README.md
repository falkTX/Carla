#![Carla Logo](https://raw.githubusercontent.com/falkTX/Carla/master/resources/128x128/carla.png) Carla Plugin Host
[![Build Status](https://travis-ci.org/falkTX/Carla.png)](https://travis-ci.org/falkTX/Carla)

# ---  README for Carla  ---

Carla is a fully-featured audio plugin host, with support for many audio drivers and plugin formats. <br/>
It has some nice features like automation of parameters via MIDI CC (and send output back as MIDI too) and full OSC control.

Carla currently supports LADSPA, DSSI, LV2, VST2/3 and AU plugin formats, plus GIG, SF2 and SFZ file support. <br/>
It uses JACK as the default and preferred audio driver but also supports native drivers like ALSA, ASIO, DirectSound or CoreAudio.

Carla-Control is an OSC Control UI for Carla (you get the OSC address from the Carla's about dialog, and connect to it). <br/>
It supports controlling main UI components (Dry/Wet, Volume and Balance), and all plugins parameters. <br/>
Peak values and control outputs are displayed as well.

<br/>
