#!/usr/bin/make -f
# Makefile for Carla #
# ------------------ #
# Created by falkTX
#

# NOTE to be imported from main Makefile

# ----------------------------------------------------------------------------------------------------------------------------

ifneq ($(MAKE_TERMOUT),)
ANS_NO=\033[31mNO\033[0m
ANS_YES=\033[32mYES\033[0m
mS=\033[33m[
mZ=\033[30;1m[
mE=]\033[0m
tS=\033[36m
tE=\033[0m
else
ANS_NO=NO
ANS_YES=YES
mS=[
mZ=[
mE=]
endif

features_print_main:
	@printf -- "$(tS)---> Main features $(tE)\n"
ifeq ($(HAVE_FRONTEND),true)
	@printf -- "Front-End:     $(ANS_YES) (Qt$(FRONTEND_TYPE))\n"
	@printf -- "LV2 plugin:    $(ANS_YES)\n"
ifneq ($(HAIKU),true)
	@printf -- "VST2 plugin:   $(ANS_YES)\n"
else
	@printf -- "VST2 plugin:   $(ANS_NO)  $(mZ)Not available for Haiku$(mE)\n"
endif
else
	@printf -- "Front-End:     $(ANS_NO)  $(mS)Missing Qt and/or PyQt$(mE)\n"
	@printf -- "LV2 plugin:    $(ANS_NO)  $(mS)No front-end$(mE)\n"
	@printf -- "VST2 plugin:   $(ANS_NO)  $(mS)No front-end$(mE)\n"
endif
ifeq ($(HAVE_HYLIA),true)
	@printf -- "Link support:  $(ANS_YES)\n"
else
	@printf -- "Link support:  $(ANS_NO)  $(mZ)Linux, MacOS and Windows only$(mE)\n"
endif
ifeq ($(HAVE_LIBLO),true)
	@printf -- "OSC support:   $(ANS_YES)\n"
else
	@printf -- "OSC support:   $(ANS_NO)  $(mS)Missing liblo$(mE)\n"
endif
ifeq ($(WINDOWS),true)
	@printf -- "Binary detect: $(ANS_YES)\n"
else ifeq ($(HAVE_LIBMAGIC),true)
	@printf -- "Binary detect: $(ANS_YES)\n"
else
	@printf -- "Binary detect: $(ANS_NO)  $(mS)Missing libmagic/file$(mE)\n"
endif
	@printf -- "\n"

	@printf -- "$(tS)---> Engine drivers $(tE)\n"
	@printf -- "JACK:        $(ANS_YES)\n"
ifeq ($(LINUX),true)
ifeq ($(HAVE_ALSA),true)
	@printf -- "ALSA:        $(ANS_YES)\n"
else
	@printf -- "ALSA:        $(ANS_NO)  $(mS)Missing ALSA$(mE)\n"
endif
else
	@printf -- "ALSA:        $(ANS_NO)  $(mZ)Linux only$(mE)\n"
endif
ifeq ($(UNIX),true)
ifneq ($(MACOS),true)
ifeq ($(HAVE_PULSEAUDIO),true)
	@printf -- "PulseAudio:  $(ANS_YES)\n"
else
	@printf -- "PulseAudio:  $(ANS_NO)  $(mS)Missing PulseAudio$(mE)\n"
endif
else
	@printf -- "PulseAudio:  $(ANS_NO)  $(mZ)Not available for MacOS$(mE)\n"
endif
else
	@printf -- "PulseAudio:  $(ANS_NO)  $(mZ)Only available for Unix systems$(mE)\n"
endif
ifeq ($(MACOS),true)
	@printf -- "CoreAudio:   $(ANS_YES)\n"
else
	@printf -- "CoreAudio:   $(ANS_NO)  $(mZ)MacOS only$(mE)\n"
endif
ifeq ($(WINDOWS),true)
	@printf -- "DirectSound: $(ANS_YES)\n"
	@printf -- "WASAPI:      $(ANS_YES)\n"
else
	@printf -- "DirectSound: $(ANS_NO)  $(mZ)Windows only$(mE)\n"
	@printf -- "WASAPI:      $(ANS_NO)  $(mZ)Windows only$(mE)\n"
endif
ifeq ($(HAVE_SDL),true)
	@printf -- "SDL:         $(ANS_YES)\n"
else
	@printf -- "SDL:         $(ANS_NO)  $(mS)Missing SDL$(mE)\n"
endif
	@printf -- "\n"

	@printf -- "$(tS)---> Plugin formats: $(tE)\n"
	@printf -- "Internal: $(ANS_YES)\n"
	@printf -- "LADSPA:   $(ANS_YES)\n"
	@printf -- "DSSI:     $(ANS_YES)\n"
	@printf -- "LV2:      $(ANS_YES)\n"
	@printf -- "CLAP:     $(ANS_YES)\n"
ifeq ($(MACOS_OR_WINDOWS),true)
	@printf -- "VST2:     $(ANS_YES) (with UI)\n"
	@printf -- "VST3:     $(ANS_YES) (with UI)\n"
else ifeq ($(HAIKU),true)
	@printf -- "VST2:     $(ANS_YES) (without UI)\n"
	@printf -- "VST3:     $(ANS_YES) (without UI)\n"
else ifeq ($(HAVE_X11),true)
	@printf -- "VST2:     $(ANS_YES) (with UI)\n"
	@printf -- "VST3:     $(ANS_YES) (with UI)\n"
else
	@printf -- "VST2:     $(ANS_YES) (without UI) $(mS)Missing X11$(mE)\n"
	@printf -- "VST3:     $(ANS_YES) (without UI) $(mS)Missing X11$(mE)\n"
endif
ifeq ($(MACOS),true)
	@printf -- "AU:       $(ANS_YES) (with UI)\n"
else  # MACOS
	@printf -- "AU:       $(ANS_NO)  $(mZ)MacOS only$(mE)\n"
endif # MACOS
	@printf -- "\n"

	@printf -- "$(tS)---> LV2 UI toolkit support: $(tE)\n"
	@printf -- "External: $(ANS_YES) (direct)\n"
	@printf -- "Gtk2:     $(ANS_YES) (bridge)\n"
	@printf -- "Gtk3:     $(ANS_YES) (bridge)\n"
ifneq ($(MACOS_OR_WINDOWS),true)
ifeq ($(HAVE_QT4),true)
	@printf -- "Qt4:      $(ANS_YES) (bridge)\n"
else
	@printf -- "Qt4:      $(ANS_NO)  $(mS)Qt4 missing$(mE)\n"
endif
ifeq ($(HAVE_QT5),true)
	@printf -- "Qt5:      $(ANS_YES) (bridge)\n"
else
	@printf -- "Qt5:      $(ANS_NO)  $(mS)Qt5 missing$(mE)\n"
endif
ifeq ($(HAVE_X11),true)
	@printf -- "X11:      $(ANS_YES) (direct+bridge)\n"
else
	@printf -- "X11:      $(ANS_NO)  $(mS)X11 missing$(mE)\n"
endif
else # !MACOS_OR_WINDOWS
	@printf -- "Qt4:      $(ANS_NO)  $(mZ)Not available for Windows or MacOS$(mE)\n"
	@printf -- "Qt5:      $(ANS_NO)  $(mZ)Not available for Windows or MacOS$(mE)\n"
	@printf -- "X11:      $(ANS_NO)  $(mZ)Not available for Windows or MacOS$(mE)\n"
endif # !MACOS_OR_WINDOWS
ifeq ($(MACOS),true)
	@printf -- "Cocoa:    $(ANS_YES) (direct+bridge)\n"
else
	@printf -- "Cocoa:    $(ANS_NO)  $(mZ)MacOS only$(mE)\n"
endif
ifeq ($(WINDOWS),true)
	@printf -- "Windows:  $(ANS_YES) (direct+bridge)\n"
else
	@printf -- "Windows:  $(ANS_NO)  $(mZ)Windows only$(mE)\n"
endif
	@printf -- "\n"

	@printf -- "$(tS)---> File formats: $(tE)\n"
ifeq ($(HAVE_SNDFILE),true)
	@printf -- "Basic: $(ANS_YES)\n"
else
	@printf -- "Basic: $(ANS_NO) $(mS)libsndfile missing$(mE)\n"
endif
	@printf -- "MP3:   $(ANS_YES)\n"
ifeq ($(HAVE_FFMPEG),true)
	@printf -- "Extra: $(ANS_YES)\n"
else
	@printf -- "Extra: $(ANS_NO) $(mS)FFmpeg missing or too new$(mE)\n"
endif
# ifeq ($(HAVE_FLUIDSYNTH_INSTPATCH),true)
# 	@printf -- "DLS:   $(ANS_YES)\n"
# else
# 	@printf -- "DLS:   $(ANS_NO) $(mS)FluidSynth/instpatch missing or too old$(mE)\n"
# endif
ifeq ($(HAVE_FLUIDSYNTH),true)
	@printf -- "SF2/3: $(ANS_YES)\n"
else
	@printf -- "SF2/3: $(ANS_NO) $(mS)FluidSynth missing or too old$(mE)\n"
endif
	@printf -- "SFZ:   $(ANS_YES)\n"
	@printf -- "\n"

	@printf -- "$(tS)---> Internal plugins: $(tE)\n"
	@printf -- "Basic Plugins:    $(ANS_YES)\n"
	@printf -- "Carla-Patchbay:   $(ANS_YES)\n"
	@printf -- "Carla-Rack:       $(ANS_YES)\n"
ifeq ($(EXTERNAL_PLUGINS),true)
	@printf -- "External Plugins: $(ANS_YES)\n"
else
	@printf -- "External Plugins: $(ANS_NO)\n"
endif

ifneq ($(EXTERNAL_PLUGINS),true)
features_print_external_plugins:
endif

features: features_print_main features_print_external_plugins

# ---------------------------------------------------------------------------------------------------------------------
