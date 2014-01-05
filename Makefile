#!/usr/bin/make -f
# Makefile for Carla #
# ------------------ #
# Created by falkTX
#

include source/Makefile.mk

# --------------------------------------------------------------

PREFIX  = /usr/local
DESTDIR =

SED_PREFIX = $(shell echo $(PREFIX) | sed "s/\//\\\\\\\\\//g")

LINK   = ln -sf
PYUIC ?= pyuic4 -w
PYRCC ?= pyrcc4 -py3

# --------------------------------------------------------------

all: CXX RES UI WIDGETS

# --------------------------------------------------------------
# C++ code (native)

CXX: backend bridges discovery plugin theme

backend:
	$(MAKE) -C source/backend

bridges:
	$(MAKE) -C source/bridges

discovery:
	$(MAKE) -C source/discovery

plugin:
	$(MAKE) -C source/plugin

theme:
	$(MAKE) -C source/modules/theme

# --------------------------------------------------------------
# C++ code (variants)

posix32:
	$(MAKE) -C source/bridges posix32
	$(MAKE) -C source/discovery posix32

posix64:
	$(MAKE) -C source/bridges posix64
	$(MAKE) -C source/discovery posix64

win32:
	$(MAKE) -C source/bridges win32
	$(MAKE) -C source/discovery win32

win64:
	$(MAKE) -C source/bridges win64
	$(MAKE) -C source/discovery win64

wine32:
	$(MAKE) -C source/libs jackbridge-wine32
	$(LINK) ../libs/jackbridge-win32.dll.so source/bridges/jackbridge-win32.dll

wine64:
	$(MAKE) -C source/libs jackbridge-wine64
	$(LINK) ../libs/jackbridge-win64.dll.so source/bridges/jackbridge-win64.dll

# --------------------------------------------------------------
# Resources

RES = source/resources_rc.py

RES: $(RES)

source/%_rc.py: resources/%.qrc resources/*/*.png resources/*/*.svg
	$(PYRCC) $< -o $@

# --------------------------------------------------------------
# UI code

UIs = \
	source/ui_carla_control.py \
	source/ui_carla_about.py \
	source/ui_carla_database.py \
	source/ui_carla_edit.py \
	source/ui_carla_host.py \
	source/ui_carla_parameter.py \
	source/ui_carla_plugin_default.py \
	source/ui_carla_plugin_zynfx.py \
	source/ui_carla_refresh.py \
	source/ui_carla_settings.py \
	source/ui_carla_settings_driver.py \
	source/ui_inputdialog_value.py

UI: $(UIs)

source/ui_%.py: resources/ui/%.ui
	$(PYUIC) $< -o $@

# --------------------------------------------------------------
# Widgets

WIDGETS = \
	source/canvaspreviewframe.py \
	source/digitalpeakmeter.py \
	source/ledbutton.py \
	source/paramspinbox.py \
	source/pixmapbutton.py \
	source/pixmapdial.py \
	source/pixmapkeyboard.py

WIDGETS: $(WIDGETS)

source/%.py: source/widgets/%.py
	$(LINK) widgets/$*.py $@

# --------------------------------------------------------------

clean:
	$(MAKE) clean -C source/backend
	$(MAKE) clean -C source/bridges
	$(MAKE) clean -C source/discovery
	$(MAKE) clean -C source/modules
	$(MAKE) clean -C source/plugin
	rm -f $(RES)
	rm -f $(UIs)
	rm -f $(WIDGETS)
	rm -f *~ source/*~ source/*.pyc source/*_rc.py source/ui_*.py

# --------------------------------------------------------------

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

doxygen:
	$(MAKE) doxygen -C source/backend

# --------------------------------------------------------------

install:
	# Create directories
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)$(PREFIX)/lib/carla/
	install -d $(DESTDIR)$(PREFIX)/lib/carla/resources/
	install -d $(DESTDIR)$(PREFIX)/lib/carla/resources/nekofilter/
	install -d $(DESTDIR)$(PREFIX)/lib/carla/resources/zynaddsubfx/
	install -d $(DESTDIR)$(PREFIX)/lib/lv2/carla-native.lv2/
	install -d $(DESTDIR)$(PREFIX)/lib/pkgconfig/
	install -d $(DESTDIR)$(PREFIX)/include/carla/
	install -d $(DESTDIR)$(PREFIX)/include/carla/includes/
	install -d $(DESTDIR)$(PREFIX)/share/applications/
	install -d $(DESTDIR)$(PREFIX)/share/carla/
	install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/
	install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -d $(DESTDIR)$(PREFIX)/share/mime/packages/

	# Install script files
	install -m 755 \
		data/carla \
		data/carla-control \
		data/carla-database \
		data/carla-patchbay \
		data/carla-rack \
		data/carla-settings \
		data/carla-single \
		$(DESTDIR)$(PREFIX)/bin/

	# Install desktop files
	install -m 644 data/*.desktop $(DESTDIR)$(PREFIX)/share/applications/

	# Install icons, 16x16
	install -m 644 resources/16x16/carla.png            $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/
	install -m 644 resources/16x16/carla-control.png    $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/

	# Install icons, 48x48
	install -m 644 resources/48x48/carla.png            $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -m 644 resources/48x48/carla-control.png    $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/

	# Install icons, 128x128
	install -m 644 resources/128x128/carla.png          $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 644 resources/128x128/carla-control.png  $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/

	# Install icons, 256x256
	install -m 644 resources/256x256/carla.png          $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -m 644 resources/256x256/carla-control.png  $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/

	# Install icons, scalable
	install -m 644 resources/scalable/carla.svg         $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 644 resources/scalable/carla-control.svg $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/

	# Install mime package
	install -m 644 data/carla.xml $(DESTDIR)$(PREFIX)/share/mime/packages/

	# Install pkgconfig file
	install -m 644 data/carla-standalone.pc $(DESTDIR)$(PREFIX)/lib/pkgconfig/

	# Install backend
	install -m 644 \
		source/backend/*.so \
		$(DESTDIR)$(PREFIX)/lib/carla/

	# Install binaries
	install -m 755 \
		source/bridges/carla-bridge-* \
		source/discovery/carla-discovery-* \
		$(DESTDIR)$(PREFIX)/lib/carla/

	# Install plugin
	install -m 644 \
		source/plugin/carla-native.lv2/*.so \
		source/plugin/carla-native.lv2/*.ttl \
		$(DESTDIR)$(PREFIX)/lib/lv2/carla-native.lv2/

	# Install python code
	install -m 644 source/*.py $(DESTDIR)$(PREFIX)/share/carla/

	# Install python "binaries"
	install -m 755 \
		source/carla \
		source/carla-patchbay \
		source/carla-rack \
		$(DESTDIR)$(PREFIX)/share/carla/

	# Install headers
	install -m 644 source/backend/CarlaBackend.h  $(DESTDIR)$(PREFIX)/include/carla/
	install -m 644 source/backend/CarlaHost.h     $(DESTDIR)$(PREFIX)/include/carla/
	install -m 644 source/includes/CarlaDefines.h $(DESTDIR)$(PREFIX)/include/carla/includes/

	# Install resources
	install -m 644 source/modules/carla_native/resources/*-ui              $(DESTDIR)$(PREFIX)/lib/carla/resources/
	install -m 644 source/modules/carla_native/resources/*.py              $(DESTDIR)$(PREFIX)/lib/carla/resources/
	install -m 644 source/modules/carla_native/resources/nekofilter/*.png  $(DESTDIR)$(PREFIX)/lib/carla/resources/nekofilter/
	install -m 644 source/modules/carla_native/resources/zynaddsubfx/*.png $(DESTDIR)$(PREFIX)/lib/carla/resources/zynaddsubfx/

	# Install theme
	$(MAKE) STYLES_DIR=$(DESTDIR)$(PREFIX)/lib/carla/styles install-main -C source/modules/theme

	# Adjust PREFIX value in script files
	sed -i "s/X-PREFIX-X/$(SED_PREFIX)/" \
		$(DESTDIR)$(PREFIX)/bin/carla \
		$(DESTDIR)$(PREFIX)/bin/carla-control \
		$(DESTDIR)$(PREFIX)/bin/carla-database \
		$(DESTDIR)$(PREFIX)/bin/carla-patchbay \
		$(DESTDIR)$(PREFIX)/bin/carla-rack \
		$(DESTDIR)$(PREFIX)/bin/carla-settings \
		$(DESTDIR)$(PREFIX)/bin/carla-single \
		$(DESTDIR)$(PREFIX)/lib/pkgconfig/carla-standalone.pc

	# Set plugin resources
	cd $(DESTDIR)$(PREFIX)/lib/lv2/carla-native.lv2/ && \
		$(LINK) $(PREFIX)/lib/carla/resources/

# --------------------------------------------------------------

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/carla*
	rm -f $(DESTDIR)$(PREFIX)/share/applications/carla.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/carla-control.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/*/apps/carla.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/*/apps/carla-control.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/carla.svg
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/carla-control.svg
	rm -f $(DESTDIR)$(PREFIX)/share/mime/packages/carla.xml
	rm -f $(DESTDIR)$(PREFIX)/lib/dssi/carla-dssi.so
	rm -f $(DESTDIR)$(PREFIX)/lib/vst/carla-vst.so
	rm -rf $(DESTDIR)$(PREFIX)/lib/carla/
	rm -rf $(DESTDIR)$(PREFIX)/lib/lv2/carla.lv2/
	rm -rf $(DESTDIR)$(PREFIX)/share/carla/

# --------------------------------------------------------------

ANS_NO=\033[31m NO \033[0m
ANS_YES=\033[32m YES \033[0m
mS=\033[33m[
mZ=\033[30;1m[
mE=]\033[0m

features:
	@echo "\033[36m---> Engine drivers: \033[0m"
	@echo "JACK:       $(ANS_YES)"
ifeq ($(LINUX),true)
ifeq ($(HAVE_ALSA),true)
	@echo "ALSA:       $(ANS_YES)"
else
	@echo "ALSA:       $(ANS_NO)  $(mS)Missing ALSA$(mE)"
endif
ifeq ($(HAVE_PULSEAUDIO),true)
	@echo "PulseAudio: $(ANS_YES)"
else
	@echo "PulseAudio: $(ANS_NO)  $(mS)Missing PulseAudio$(mE)"
endif
else
	@echo "ALSA:       $(ANS_NO)  $(mZ)Linux only$(mE)"
	@echo "PulseAudio: $(ANS_NO)  $(mZ)Linux only$(mE)"
endif
ifeq ($(MACOS),true)
	@echo "CoreAudio:  $(ANS_YES)"
else
	@echo "CoreAudio:  $(ANS_NO)  $(mZ)MacOS only$(mE)"
endif
ifeq ($(WIN32),true)
	@echo "ASIO:       $(ANS_YES)"
	@echo "DirectSound:$(ANS_YES)"
else
	@echo "ASIO:       $(ANS_NO)  $(mZ)Windows only$(mE)"
	@echo "DirectSound:$(ANS_NO)  $(mZ)Windows only$(mE)"
endif
	@echo ""

	@echo "\033[36m---> Plugin formats: \033[0m"
	@echo "Internal:$(ANS_YES)"
ifeq ($(CARLA_PLUGIN_SUPPORT),true)
	@echo "LADSPA:  $(ANS_YES)"
	@echo "DSSI:    $(ANS_YES)"
	@echo "LV2:     $(ANS_YES)"
	@echo "VST:     $(ANS_YES)"
ifeq ($(MACOS),true)
	@echo "AU:      $(ANS_YES)"
else
	@echo "AU:      $(ANS_NO)  $(mZ)MacOS only$(mE)"
endif
else
	@echo "LADSPA:  $(ANS_NO)  $(mS)Plugins disabled$(mE)"
	@echo "DSSI:    $(ANS_NO)  $(mS)Plugins disabled$(mE)"
	@echo "LV2:     $(ANS_NO)  $(mS)Plugins disabled$(mE)"
	@echo "VST:     $(ANS_NO)  $(mS)Plugins disabled$(mE)"
	@echo "AU:      $(ANS_NO)  $(mS)Plugins disabled$(mE)"
endif
	@echo ""

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
	@echo "\033[36m---> LV2 UI toolkit support: \033[0m"
	@echo "External:$(ANS_YES) (direct+bridge)"
ifeq ($(LINUX),true)
ifeq ($(HAVE_GTK2),true)
	@echo "Gtk2:    $(ANS_YES) (bridge)"
else
	@echo "Gtk2:    $(ANS_NO)  $(mS)Gtk2 missing$(mE)"
endif
ifeq ($(HAVE_GTK3),true)
	@echo "Gtk3:    $(ANS_YES) (bridge)"
else
	@echo "Gtk3:    $(ANS_NO)  $(mS)Gtk3 missing$(mE)"
endif
ifeq ($(HAVE_QT4),true)
	@echo "Qt4:     $(ANS_YES) (bridge)"
else
	@echo "Qt4:     $(ANS_NO)  $(mS)Qt4 missing$(mE)"
endif
ifeq ($(HAVE_QT5),true)
	@echo "Qt5:     $(ANS_YES) (bridge)"
else
	@echo "Qt5:     $(ANS_NO)  $(mS)Qt5 missing$(mE)"
endif
	@echo "X11:     $(ANS_YES) (direct+bridge)"
else
	@echo "Gtk2:    $(ANS_NO)  $(mZ)Linux only$(mE)"
	@echo "Gtk3:    $(ANS_NO)  $(mZ)Linux only$(mE)"
	@echo "Qt4:     $(ANS_NO)  $(mZ)Linux only$(mE)"
	@echo "Qt5:     $(ANS_NO)  $(mZ)Linux only$(mE)"
	@echo "X11:     $(ANS_NO)  $(mZ)Linux only$(mE)"
endif
ifeq ($(MACOS),true)
	@echo "Cocoa:   $(ANS_YES) (direct+bridge)"
else
	@echo "Cocoa:   $(ANS_NO)  $(mZ)MacOS only$(mE)"
endif
ifeq ($(WIN32),true)
	@echo "Windows: $(ANS_YES) (direct+bridge)"
else
	@echo "Windows: $(ANS_NO)  $(mZ)Windows only$(mE)"
endif
	@echo ""
endif

	@echo "\033[36m---> File formats: \033[0m"
ifeq ($(CARLA_CSOUND_SUPPORT),true)
	@echo "CSD:$(ANS_YES)"
else
	@echo "CSD:$(ANS_NO)  $(mS)CSound disabled$(mE)"
endif
ifeq ($(CARLA_SAMPLERS_SUPPORT),true)
ifeq ($(HAVE_LINUXSAMPLER),true)
	@echo "GIG:$(ANS_YES)"
else
	@echo "GIG:$(ANS_NO)  $(mS)LinuxSampler missing$(mE)"
endif
ifeq ($(HAVE_FLUIDSYNTH),true)
	@echo "SF2:$(ANS_YES)"
else
	@echo "SF2:$(ANS_NO)  $(mS)FluidSynth missing$(mE)"
endif
ifeq ($(HAVE_LINUXSAMPLER),true)
	@echo "SFZ:$(ANS_YES)"
else
	@echo "SFZ:$(ANS_NO)  $(mS)LinuxSampler missing$(mE)"
endif
else
	@echo "GIG:$(ANS_NO)  $(mS)Samplers disabled$(mE)"
	@echo "SF2:$(ANS_NO)  $(mS)Samplers disabled$(mE)"
	@echo "SFZ:$(ANS_NO)  $(mS)Samplers disabled$(mE)"
endif
	@echo ""

	@echo "\033[36m---> Internal plugins: \033[0m"
ifeq ($(HAVE_AF_DEPS),true)
ifeq ($(HAVE_FFMPEG),true)
	@echo "AudioFile:  $(ANS_YES) (with ffmpeg)"
else
	@echo "AudioFile:  $(ANS_YES) (without ffmpeg) $(mS)ffmpeg/libav missing or too new$(mE)"
endif
else
	@echo "AudioFile:  $(ANS_NO)  $(mS)libsndfile missing$(mE)"
endif
ifeq ($(HAVE_MF_DEPS),true)
	@echo "MidiFile:   $(ANS_YES)"
else
	@echo "MidiFile:   $(ANS_NO)  $(mS)LibSMF missing$(mE)"
endif
ifeq ($(HAVE_OPENGL),true)
	@echo "DISTRHO:    $(ANS_YES)"
else
	@echo "DISTRHO:    $(ANS_NO)  $(mS)OpenGL missing$(mE)"
endif
ifeq ($(HAVE_ZYN_DEPS),true)
ifeq ($(HAVE_ZYN_UI_DEPS),true)
	@echo "ZynAddSubFX:$(ANS_YES) (with UI)"
else
	@echo "ZynAddSubFX:$(ANS_YES) (without UI) $(mS)NTK missing$(mE)"
endif
else
	@echo "ZynAddSubFX:$(ANS_NO)  $(mS)fftw3, mxml or zlib missing$(mE)"
endif

# --------------------------------------------------------------
