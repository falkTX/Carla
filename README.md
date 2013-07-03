# ---  README for Carla  ---

Carla is an audio plugin host, with support for many audio drivers and plugin formats. <br/>
It has some nice features like automation of parameters via MIDI CC (and send output back as MIDI too) and full OSC control.

Carla currently supports LADSPA (including LRDF), DSSI, LV2, and VST plugin formats, plus GIG, SF2 and SFZ file support. <br/>
It uses JACK as the default and preferred audio driver but also supports native drivers like ALSA, DirectSound or CoreAudio.

Carla-Control is an OSC Control GUI for Carla (you get the OSC address from the Carla's about dialog, and connect to it). <br/>
It supports controlling main UI components (Dry/Wet, Volume and Balance), and all plugins parameters. <br/>
Peak values and control outputs are displayed as well.

<b>NOTE:</b> You're currently using the development git branch of Carla. <br/>
For the stable version please use the "1.0.x" branch: <br/>
`git checkout 1.0.x`

<br/>
