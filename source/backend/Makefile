#!/usr/bin/make -f
# Makefile for carla backend and standalone #
# ----------------------------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

all:
	$(MAKE) -C engine
	$(MAKE) -C plugin
	$(MAKE) -C native
	$(MAKE) -C standalone

# --------------------------------------------------------------

clean:
	$(MAKE) clean -C engine
	$(MAKE) clean -C plugin
	$(MAKE) clean -C native
	$(MAKE) clean -C standalone

# --------------------------------------------------------------

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

doc: CarlaBackend.doxygen
	doxygen $^

# --------------------------------------------------------------

ANS_NO=\033[31m NO \033[0m
ANS_YES=\033[32m YES \033[0m

config:
	@echo "\033[33m||| Engine drivers: \033[0m"
ifeq ($(HAVE_ALSA),true)
	@echo "ALSA:      $(ANS_YES)"
else
	@echo "ALSA:      $(ANS_NO)"
endif
	@echo "JACK:      $(ANS_YES)"
ifeq ($(HAVE_PULSEAUDIO),true)
	@echo "PulseAudio:$(ANS_YES)"
else
	@echo "PulseAudio:$(ANS_NO)"
endif
	@echo ""

	@echo "\033[33m||| Plugin formats: \033[0m"
	@echo "Internal:$(ANS_YES)"
ifeq ($(CARLA_PLUGIN_SUPPORT),true)
	@echo "LADSPA:  $(ANS_YES)"
	@echo "DSSI:    $(ANS_YES)"
	@echo "LV2:     $(ANS_YES)"
	@echo "VST:     $(ANS_YES)"
else
	@echo "LADSPA:  $(ANS_NO)"
	@echo "DSSI:    $(ANS_NO)"
	@echo "LV2:     $(ANS_NO)"
	@echo "VST:     $(ANS_NO)"
endif
	@echo ""

	@echo "\033[33m||| Sample formats: \033[0m"
ifeq ($(HAVE_LINUXSAMPLER),true)
	@echo "GIG:$(ANS_YES)"
else
	@echo "GIG:$(ANS_NO)"
endif
ifeq ($(HAVE_FLUIDSYNTH),true)
	@echo "SF2:$(ANS_YES)"
else
	@echo "SF2:$(ANS_NO)"
endif
ifeq ($(HAVE_LINUXSAMPLER),true)
	@echo "SFZ:$(ANS_YES)"
else
	@echo "SFZ:$(ANS_NO)"
endif
	@echo ""

	@echo "\033[33m||| Internal plugins: \033[0m"
ifeq ($(HAVE_AF_DEPS),true)
	@echo "AudioFile:  $(ANS_YES)"
else
	@echo "AudioFIle:  $(ANS_NO)"
endif

ifeq ($(HAVE_MF_DEPS),true)
	@echo "MidiFile:   $(ANS_YES)"
else
	@echo "MidiFile:   $(ANS_NO)"
endif

ifeq ($(HAVE_OPENGL),true)
	@echo "DISTRHO:    $(ANS_YES)"
else
	@echo "DISTRHO:    $(ANS_NO)"
endif

ifeq ($(HAVE_ZYN_DEPS),true)
ifeq ($(HAVE_ZYN_UI_DEPS),true)
	@echo "ZynAddSubFX:$(ANS_YES) (with UI)"
else
	@echo "ZynAddSubFX:$(ANS_YES) (without UI)"
endif
else
	@echo "ZynAddSubFX:$(ANS_NO)"
endif
