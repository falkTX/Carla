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
mS=\033[33m[
mE=]\033[0m

config:
ifeq ($(MACOS),true)
# --- MacOS ---
	@echo "\033[36m||| Engine drivers: (MacOS)\033[0m"
	@echo "JACK:      $(ANS_YES)"
ifeq ($(CARLA_RTAUDIO_SUPPORT),true)
	@echo "CoreAudio: $(ANS_YES)"
else
	@echo "CoreAudio: $(ANS_NO) $(mS)RtAudio disabled$(mE)"
endif
	@echo ""
# --- MacOS ---
else
# --- Win32 ---
ifeq ($(WIN32),true)
	@echo "\033[36m||| Engine drivers: (Windows)\033[0m"
	@echo "JACK:        $(ANS_YES)"
ifeq ($(CARLA_RTAUDIO_SUPPORT),true)
	@echo "ASIO:        $(ANS_YES)"
	@echo "DirectSound: $(ANS_YES)"
else
	@echo "ASIO:        $(ANS_NO) $(mS)RtAudio disabled$(mE)"
	@echo "DirectSound: $(ANS_NO) $(mS)RtAudio disabled$(mE)"
endif
	@echo ""
# --- Win32 ---
else
# --- Others ---
	@echo "\033[36m||| Engine drivers: \033[0m"
	@echo "JACK:      $(ANS_YES)"
ifeq ($(CARLA_RTAUDIO_SUPPORT),true)
ifeq ($(HAVE_ALSA),true)
	@echo "ALSA:      $(ANS_YES)"
else
	@echo "ALSA:      $(ANS_NO) $(mS)Missing ALSA$(mE)"
endif
ifeq ($(HAVE_PULSEAUDIO),true)
	@echo "PulseAudio:$(ANS_YES)"
else
	@echo "PulseAudio:$(ANS_NO) $(mS)Missing PulseAudio$(mE)"
endif
else
	@echo "ALSA:      $(ANS_NO) $(mS)RtAudio disabled$(mE)"
	@echo "PulseAudio:$(ANS_NO) $(mS)RtAudio disabled$(mE)"
endif
	@echo ""
# --- Others ---
endif
endif

	@echo "\033[36m---> Plugin formats: \033[0m"
	@echo "Internal:$(ANS_YES)"
ifeq ($(CARLA_PLUGIN_SUPPORT),true)
	@echo "LADSPA:  $(ANS_YES)"
	@echo "DSSI:    $(ANS_YES)"
	@echo "LV2:     $(ANS_YES)"
	@echo "VST:     $(ANS_YES)"
else
	@echo "LADSPA:  $(ANS_NO) $(mS)Plugins disabled$(mE)"
	@echo "DSSI:    $(ANS_NO) $(mS)Plugins disabled$(mE)"
	@echo "LV2:     $(ANS_NO) $(mS)Plugins disabled$(mE)"
	@echo "VST:     $(ANS_NO) $(mS)Plugins disabled$(mE)"
endif
	@echo ""

	@echo "\033[36m---> Sample formats: \033[0m"
ifeq ($(CARLA_SAMPLERS_SUPPORT),true)
ifeq ($(HAVE_LINUXSAMPLER),true)
	@echo "GIG:$(ANS_YES)"
else
	@echo "GIG:$(ANS_NO) $(mS)LinuxSampler missing$(mE)"
endif
ifeq ($(HAVE_FLUIDSYNTH),true)
	@echo "SF2:$(ANS_YES)"
else
	@echo "SF2:$(ANS_NO) $(mS)FluidSynth missing$(mE)"
endif
ifeq ($(HAVE_LINUXSAMPLER),true)
	@echo "SFZ:$(ANS_YES)"
else
	@echo "SFZ:$(ANS_NO) $(mS)LinuxSampler missing$(mE)"
endif
else
	@echo "GIG:$(ANS_NO) $(mS)Samplers disabled$(mE)"
	@echo "SF2:$(ANS_NO) $(mS)Samplers disabled$(mE)"
	@echo "SFZ:$(ANS_NO) $(mS)Samplers disabled$(mE)"
endif
	@echo ""

	@echo "\033[36m---> Internal plugins: \033[0m"
ifeq ($(HAVE_AF_DEPS),true)
ifeq ($(HAVE_FFMPEG),true)
	@echo "AudioFile:  $(ANS_YES) (with ffmpeg)"
else
	@echo "AudioFIle:  $(ANS_YES) (without ffmpeg) $(mS)ffmpeg/libav missing$(mE)"
endif
else
	@echo "AudioFIle:  $(ANS_NO) $(mS)libsndfile missing$(mE)"
endif

ifeq ($(HAVE_MF_DEPS),true)
	@echo "MidiFile:   $(ANS_YES)"
else
	@echo "MidiFile:   $(ANS_NO) $(mS)libsmf missing$(mE)"
endif

ifeq ($(HAVE_OPENGL),true)
	@echo "DISTRHO:    $(ANS_YES)"
else
	@echo "DISTRHO:    $(ANS_NO) $(mS)OpenGL missing$(mE)"
endif

ifeq ($(HAVE_ZYN_DEPS),true)
ifeq ($(HAVE_ZYN_UI_DEPS),true)
	@echo "ZynAddSubFX:$(ANS_YES) (with UI)"
else
	@echo "ZynAddSubFX:$(ANS_YES) (without UI) $(mS)NTK missing$(mE)"
endif
else
	@echo "ZynAddSubFX:$(ANS_NO) $(mS)fftw-3, mxml or zlib missing$(mE)"
endif
