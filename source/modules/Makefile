#!/usr/bin/make -f
# Makefile for carla modules #
# -------------------------- #
# Created by falkTX
#

all:

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

juce_gui_extra:
	$(MAKE) -C juce_gui_extra

juce_gui_extra_%:
	$(MAKE) -C juce_gui_extra $*

# --------------------------------------------------------------

lilv:
	$(MAKE) -C lilv

lilv_%:
	$(MAKE) -C lilv $*

# --------------------------------------------------------------

native-plugins:
	$(MAKE) -C native-plugins

# --------------------------------------------------------------

rtaudio:
	$(MAKE) -C rtaudio

rtmidi:
	$(MAKE) -C rtmidi

# --------------------------------------------------------------

rtmempool:
	$(MAKE) -C rtmempool

rtmempool_%:
	$(MAKE) -C rtmempool $*

# --------------------------------------------------------------

stk:
	$(MAKE) -C stk

stk_%:
	$(MAKE) -C stk $*

# --------------------------------------------------------------

theme:
	$(MAKE) -C theme

theme.qt4:
	$(MAKE) -C theme qt4

theme.qt5:
	$(MAKE) -C theme qt5

# --------------------------------------------------------------

jackbridge:
	$(MAKE) -C jackbridge

# --------------------------------------------------------------

jackbridge-posix32:
	$(MAKE) -C jackbridge posix32

jackbridge-posix64:
	$(MAKE) -C jackbridge posix64

jackbridge-posix32e:
	$(MAKE) -C jackbridge posix32e

jackbridge-posix64e:
	$(MAKE) -C jackbridge posix64e

# --------------------------------------------------------------

jackbridge-win32:
	$(MAKE) -C jackbridge win32

jackbridge-win64:
	$(MAKE) -C jackbridge win64

jackbridge-win32e:
	$(MAKE) -C jackbridge win32e

jackbridge-win64e:
	$(MAKE) -C jackbridge win64e

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
	$(MAKE) clean -C audio_decoder
	$(MAKE) clean -C native-plugins
	$(MAKE) clean -C dgl
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
	$(MAKE) clean -C juce_gui_extra
	$(MAKE) clean -C lilv
	$(MAKE) clean -C rtaudio
	$(MAKE) clean -C rtmidi
	$(MAKE) clean -C rtmempool
	$(MAKE) clean -C stk
	$(MAKE) clean -C theme

# --------------------------------------------------------------

.PHONY: \
	audio_decoder dgl jackbridge lilv native-plugins rtaudio rtmidi rtmempool stk theme \
	juce_audio_basics juce_audio_devices juce_audio_formats juce_audio_processors juce_core juce_data_structures juce_events juce_graphics juce_gui_basics juce_gui_extra

# --------------------------------------------------------------
