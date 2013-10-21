#!/usr/bin/make -f
# Makefile for carla modules #
# -------------------------- #
# Created by falkTX
#

all:

# --------------------------------------------------------------

carla_native:
	$(MAKE) -C carla_native

# --------------------------------------------------------------

dgl:
	$(MAKE) -C distrho/dgl

dgl_%:
	$(MAKE) -C distrho/dgl $*

# --------------------------------------------------------------

juce_audio_basics:
	$(MAKE) -C juce_audio_basics

juce_audio_basics_%:
	$(MAKE) -C juce_audio_basics $*

# --------------------------------------------------------------

juce_audio_devices:
	$(MAKE) -C juce_audio_devices

juce_audio_devices_%:
	$(MAKE) -C juce_audio_devices $*

# --------------------------------------------------------------

juce_audio_formats:
	$(MAKE) -C juce_audio_formats

juce_audio_formats_%:
	$(MAKE) -C juce_audio_formats $*

# --------------------------------------------------------------

juce_audio_processors:
	$(MAKE) -C juce_audio_processors

juce_audio_processors_%:
	$(MAKE) -C juce_audio_processors $*

# --------------------------------------------------------------

juce_core:
	$(MAKE) -C juce_core

juce_core_%:
	$(MAKE) -C juce_core $*

# --------------------------------------------------------------

juce_data_structures:
	$(MAKE) -C juce_data_structures

juce_data_structures_%:
	$(MAKE) -C juce_data_structures $*

# --------------------------------------------------------------

juce_events:
	$(MAKE) -C juce_events

juce_events_%:
	$(MAKE) -C juce_events $*

# --------------------------------------------------------------

juce_graphics:
	$(MAKE) -C juce_graphics

juce_graphics_%:
	$(MAKE) -C juce_graphics $*

# --------------------------------------------------------------

juce_gui_basics:
	$(MAKE) -C juce_gui_basics

juce_gui_basics_%:
	$(MAKE) -C juce_gui_basics $*

# --------------------------------------------------------------

lilv:
	$(MAKE) -C lilv

lilv_%:
	$(MAKE) -C lilv $*

# --------------------------------------------------------------

rtaudio:
	$(MAKE) -C rtaudio

# --------------------------------------------------------------

rtmempool:
	$(MAKE) -C rtmempool

rtmempool_%:
	$(MAKE) -C rtmempool $*

# --------------------------------------------------------------

theme:
	$(MAKE) -C theme

theme_%:
	$(MAKE) -C theme $*

# --------------------------------------------------------------

jackbridge:
	$(MAKE) -C jackbridge

jackbridge-win32:
	$(MAKE) -C jackbridge win32

jackbridge-win64:
	$(MAKE) -C jackbridge win64

jackbridge-wine32:
	$(MAKE) -C jackbridge wine32

jackbridge-wine64:
	$(MAKE) -C jackbridge wine64

# --------------------------------------------------------------

lib%.dll:
	$(MAKE) -C $* ../lib$*.dll

lib%.dylib:
	$(MAKE) -C $* ../lib$*.dylib

lib%.so:
	$(MAKE) -C $* ../lib$*.so

# --------------------------------------------------------------

clean:
	rm -f *.a *.def *.dll *.dylib *.so
	$(MAKE) clean -C carla_native
	$(MAKE) clean -C distrho/dgl
	$(MAKE) clean -C jackbridge
	$(MAKE) clean -C juce_audio_basics
	$(MAKE) clean -C juce_audio_devices
	$(MAKE) clean -C juce_audio_formats
	$(MAKE) clean -C juce_audio_processors
	$(MAKE) clean -C juce_core
	$(MAKE) clean -C juce_data_structures
	$(MAKE) clean -C juce_events
	$(MAKE) clean -C juce_graphics
	$(MAKE) clean -C juce_gui_basics
	$(MAKE) clean -C lilv
	$(MAKE) clean -C rtaudio
	$(MAKE) clean -C rtmempool
	$(MAKE) clean -C theme
	$(MAKE) clean -C widgets

# --------------------------------------------------------------

.PHONY: \
	carla_native lilv rtaudio rtmidi rtmempool theme \
	juce_audio_basics juce_audio_devices juce_audio_formats juce_audio_processors juce_core juce_data_structures juce_events juce_graphics juce_gui_basics

# --------------------------------------------------------------
