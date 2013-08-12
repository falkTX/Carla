#!/usr/bin/make -f
# Makefile for carla modules #
# -------------------------- #
# Created by falkTX
#

all:

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

juce_core:
	$(MAKE) -C juce_core

juce_core_%:
	$(MAKE) -C juce_core $*

# --------------------------------------------------------------

lilv:
	$(MAKE) -C lilv

lilv_%:
	$(MAKE) -C lilv $*

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

widgets:
	$(MAKE) -C widgets

# --------------------------------------------------------------

jackbridge-win32:
	$(MAKE) -C jackbridge win32

jackbridge-win64:
	$(MAKE) -C jackbridge win64

jackbridge-wine32:
	$(MAKE) -C jackbridge wine32

jackbridge-wine64:
	$(MAKE) -C jackbridge wine64

# --------------------------------------------------------------

clean:
	rm -f *.a *.def *.dll *.dylib *.so
	$(MAKE) clean -C distrho/dgl
	$(MAKE) clean -C lilv
	$(MAKE) clean -C jackbridge
	$(MAKE) clean -C juce_core
	$(MAKE) clean -C rtmempool
	$(MAKE) clean -C theme
	$(MAKE) clean -C widgets

# --------------------------------------------------------------

.PHONY: dgl juce_audio_basics juce_core lilv rtmempool theme widgets
