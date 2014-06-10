#!/usr/bin/make -f
# Makefile for carla modules #
# -------------------------- #
# Created by falkTX
#

all:

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

clean:
	rm -f *.a *.def *.dll *.dylib *.so
	$(MAKE) clean -C audio_decoder
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
	$(MAKE) clean -C native-plugins
	$(MAKE) clean -C rtaudio
	$(MAKE) clean -C rtmempool
	$(MAKE) clean -C rtmidi
	$(MAKE) clean -C stk
	$(MAKE) clean -C theme

# --------------------------------------------------------------

.PHONY: \
	audio_decoder dgl jackbridge lilv native-plugins rtaudio rtmempool rtmidi stk theme \
	juce_audio_basics juce_audio_devices juce_audio_formats juce_audio_processors juce_core juce_data_structures juce_events juce_graphics juce_gui_basics juce_gui_extra

# --------------------------------------------------------------
