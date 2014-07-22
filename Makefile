#!/usr/bin/make -f
# Makefile for Carla #
# ------------------ #
# Created by falkTX
#

include source/Makefile.mk

# --------------------------------------------------------------

PREFIX  = /usr/local
DESTDIR =

LINK = ln -sf

ifeq ($(DEFAULT_QT),4)
PYUIC ?= pyuic4 -w
PYRCC ?= pyrcc4 -py3
else
PYUIC ?= pyuic5
PYRCC ?= pyrcc5
endif

# --------------------------------------------------------------

all: BIN RES UI WIDGETS

# --------------------------------------------------------------
# Binaries (native)

BIN: libs backend bridges discovery plugin theme

# --------------------------------------------------------------

ALL_LIBS += source/backend/carla_engine.a
ALL_LIBS += source/backend/carla_engine_plugin.a
ALL_LIBS += source/backend/carla_plugin.a

ALL_LIBS += source/modules/jackbridge.a

ALL_LIBS += source/modules/juce_audio_basics.a
ALL_LIBS += source/modules/juce_audio_formats.a
ALL_LIBS += source/modules/juce_core.a

ifeq ($(MACOS_OR_WIN32),true)
ALL_LIBS += source/modules/juce_audio_devices.a
ALL_LIBS += source/modules/juce_audio_processors.a
ALL_LIBS += source/modules/juce_data_structures.a
ALL_LIBS += source/modules/juce_events.a
ALL_LIBS += source/modules/juce_graphics.a
ALL_LIBS += source/modules/juce_gui_basics.a
ALL_LIBS += source/modules/juce_gui_extra.a
else
ALL_LIBS += source/modules/rtaudio.a
ALL_LIBS += source/modules/rtmidi.a
endif

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
ALL_LIBS += source/modules/lilv.a
endif

ALL_LIBS += source/modules/native-plugins.a
ALL_LIBS += source/modules/rtmempool.a

ifeq ($(HAVE_QT4),true)
ALL_LIBS += source/modules/theme.qt4.a
endif

ifeq ($(HAVE_QT5),true)
ALL_LIBS += source/modules/theme.qt5.a
endif

libs: $(ALL_LIBS)

source/backend/carla_%_plugin.a: .FORCE
	$(MAKE) -C source/backend/$* ../carla_$*_plugin.a

source/backend/carla_%.a: .FORCE
	$(MAKE) -C source/backend/$* ../carla_$*.a

source/modules/%.qt4.a: .FORCE
	$(MAKE) -C source/modules/$* ../$*.qt4.a

source/modules/%.qt5.a: .FORCE
	$(MAKE) -C source/modules/$* ../$*.qt5.a

source/modules/%.a: .FORCE
	$(MAKE) -C source/modules/$* ../$*.a

# --------------------------------------------------------------

backend: bin/libcarla_standalone2$(LIB_EXT)

bin/libcarla_standalone2$(LIB_EXT): libs .FORCE
	$(MAKE) -C source/backend

# --------------------------------------------------------------

bridges: bin/carla-bridge-native$(APP_EXT)

bin/carla-bridge-native$(APP_EXT): libs .FORCE
	$(MAKE) -C source/bridges

# --------------------------------------------------------------

discovery: bin/carla-discovery-native$(APP_EXT)

bin/carla-discovery-native$(APP_EXT): libs .FORCE
	$(MAKE) -C source/discovery

# --------------------------------------------------------------

plugin: bin/carla-native.lv2/manifest.ttl

bin/carla-native.lv2/carla-native$(LIB_EXT): backend .FORCE
	$(MAKE) -C source/plugin ../../$@

bin/carla-native.lv2/manifest.ttl: bin/carla-native-lv2-export$(APP_EXT) bridges discovery .FORCE
	cd bin && ./carla-native-lv2-export$(APP_EXT); cd ..
	cd bin/carla-native.lv2 && $(LINK) ../*bridge-* ../carla-discovery-* .; cd ..

bin/carla-native-lv2-export$(APP_EXT): bin/carla-native.lv2/carla-native$(LIB_EXT) .FORCE
	$(MAKE) -C source/plugin ../../$@

# --------------------------------------------------------------

theme: bin/styles/carlastyle$(LIB_EXT)

bin/styles/carlastyle$(LIB_EXT): .FORCE
	$(MAKE) -C source/modules/theme

# --------------------------------------------------------------
# Binaries (variants)

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
	$(MAKE) -C source/modules jackbridge-wine32
	$(LINK) source/modules/jackbridge-win32.dll.so source/bridges/jackbridge-win32.dll

wine64:
	$(MAKE) -C source/modules jackbridge-wine64
	$(LINK) source/modules/jackbridge-win64.dll.so source/bridges/jackbridge-win64.dll

# --------------------------------------------------------------
# Resources

RES = \
	bin/resources/carla_backend.py \
	bin/resources/carla_config.py \
	bin/resources/carla_database.py \
	bin/resources/carla_host.py \
	bin/resources/carla_rack.py \
	bin/resources/carla_settings.py \
	bin/resources/carla_skin.py \
	bin/resources/carla_shared.py \
	bin/resources/carla_style.py \
	bin/resources/carla_widgets.py \
	bin/resources/canvaspreviewframe.py \
	bin/resources/digitalpeakmeter.py \
	bin/resources/externalui.py \
	bin/resources/ledbutton.py \
	bin/resources/paramspinbox.py \
	bin/resources/patchcanvas.py \
	bin/resources/patchcanvas_theme.py \
	bin/resources/pixmapbutton.py \
	bin/resources/pixmapdial.py \
	bin/resources/pixmapkeyboard.py \
	bin/resources/resources_rc.py \
	bin/resources/ui_carla_about.py \
	bin/resources/ui_carla_database.py \
	bin/resources/ui_carla_edit.py \
	bin/resources/ui_carla_host.py \
	bin/resources/ui_carla_parameter.py \
	bin/resources/ui_carla_plugin_basic_fx.py \
	bin/resources/ui_carla_plugin_calf.py \
	bin/resources/ui_carla_plugin_default.py \
	bin/resources/ui_carla_plugin_zita.py \
	bin/resources/ui_carla_plugin_zynfx.py \
	bin/resources/ui_carla_refresh.py \
	bin/resources/ui_carla_settings.py \
	bin/resources/ui_carla_settings_driver.py \
	bin/resources/ui_inputdialog_value.py \
	source/carla_config.py \
	source/resources_rc.py

RES: $(RES)

source/carla_config.py:
	@echo "#!/usr/bin/env python3\n# -*- coding: utf-8 -*-\n" > $@
ifeq ($(DEFAULT_QT),4)
	@echo "config_UseQt5 = False" >> $@
else
	@echo "config_UseQt5 = True" >> $@
endif

source/resources_rc.py: resources/resources.qrc resources/*/*.png resources/*/*.svg
	$(PYRCC) $< -o $@

bin/resources/%.py: source/%.py
	$(LINK) $(CURDIR)/source/$*.py bin/resources/

# --------------------------------------------------------------
# UI code

UIs = \
	source/ui_carla_control.py \
	source/ui_carla_about.py \
	source/ui_carla_database.py \
	source/ui_carla_edit.py \
	source/ui_carla_host.py \
	source/ui_carla_parameter.py \
	source/ui_carla_plugin_basic_fx.py \
	source/ui_carla_plugin_default.py \
	source/ui_carla_plugin_calf.py \
	source/ui_carla_plugin_zita.py \
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
	source/pixmapkeyboard.py \
	source/qgraphicsembedscene.py

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

debug:
	$(MAKE) DEBUG=true

doxygen:
	$(MAKE) doxygen -C source/backend

# --------------------------------------------------------------

install:
	# Create directories
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)$(PREFIX)/lib/carla/
	install -d $(DESTDIR)$(PREFIX)/lib/carla/styles/
	install -d $(DESTDIR)$(PREFIX)/lib/lv2/carla-native.lv2/
	install -d $(DESTDIR)$(PREFIX)/lib/lv2/carla-native.lv2/styles/
	install -d $(DESTDIR)$(PREFIX)/lib/pkgconfig/
	install -d $(DESTDIR)$(PREFIX)/include/carla/
	install -d $(DESTDIR)$(PREFIX)/include/carla/includes/
	install -d $(DESTDIR)$(PREFIX)/share/applications/
	install -d $(DESTDIR)$(PREFIX)/share/carla/
	install -d $(DESTDIR)$(PREFIX)/share/carla/resources/
	install -d $(DESTDIR)$(PREFIX)/share/carla/resources/nekofilter/
	install -d $(DESTDIR)$(PREFIX)/share/carla/resources/zynaddsubfx/
	install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/16x16/apps/
	install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/48x48/apps/
	install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/256x256/apps/
	install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -d $(DESTDIR)$(PREFIX)/share/mime/packages/

	# Install script files
	install -m 755 \
		data/carla \
		data/carla-database \
		data/carla-patchbay \
		data/carla-rack \
		data/carla-settings \
		$(DESTDIR)$(PREFIX)/bin/

# 		data/carla-control \
# 		data/carla-single \

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

	# Install pkg-config file
	install -m 644 data/carla-standalone.pc $(DESTDIR)$(PREFIX)/lib/pkgconfig/

	# Install backend
	install -m 644 \
		bin/libcarla_standalone2.* \
		$(DESTDIR)$(PREFIX)/lib/carla/

	# Install lv2 plugin
	install -m 644 \
		bin/carla-native.lv2/carla-native.* \
		bin/carla-native.lv2/*.ttl \
		$(DESTDIR)$(PREFIX)/lib/lv2/carla-native.lv2/

	# Install binaries (backend)
	install -m 755 \
		bin/*bridge-* \
		bin/carla-discovery-* \
		$(DESTDIR)$(PREFIX)/lib/carla/

	# Install binaries (lv2 plugin)
	install -m 755 \
		bin/*bridge-* \
		bin/carla-discovery-* \
		$(DESTDIR)$(PREFIX)/lib/lv2/carla-native.lv2/

	# Install theme
	install -m 644 \
		bin/styles/* \
		$(DESTDIR)$(PREFIX)/lib/carla/styles/

	install -m 644 \
		bin/styles/* \
		$(DESTDIR)$(PREFIX)/lib/lv2/carla-native.lv2/styles/

	# Install python code
	install -m 644 \
		source/carla \
		source/carla-patchbay \
		source/carla-rack \
		source/*.py \
		$(DESTDIR)$(PREFIX)/share/carla/

	# Install headers
	install -m 644 \
		source/backend/CarlaBackend.h \
		source/backend/CarlaHost.h \
		source/backend/CarlaEngine.hpp \
		source/backend/CarlaPlugin.hpp \
		source/modules/CarlaNative.h \
		$(DESTDIR)$(PREFIX)/include/carla/

	install -m 644 \
		source/includes/CarlaDefines.h \
		$(DESTDIR)$(PREFIX)/include/carla/includes/

	# Install resources (main)
	install -m 755 \
		bin/resources/carla-plugin \
		bin/resources/*-ui \
		$(DESTDIR)$(PREFIX)/share/carla/resources/

	install -m 644 \
		bin/resources/nekofilter/*.png \
		$(DESTDIR)$(PREFIX)/share/carla/resources/nekofilter/

	install -m 644 \
		bin/resources/zynaddsubfx/*.png \
		$(DESTDIR)$(PREFIX)/share/carla/resources/zynaddsubfx/

	# Install resources (re-use python files)
	cd $(DESTDIR)$(PREFIX)/share/carla/resources/ && \
		$(LINK) ../*.py .
	cd $(CURDIR)

	# Link resources for lv2 plugin
	rm -rf $(DESTDIR)$(PREFIX)/lib/lv2/carla-native.lv2/resources
	$(LINK) $(PREFIX)/share/carla/resources/ $(DESTDIR)$(PREFIX)/lib/lv2/carla-native.lv2/

	# Adjust PREFIX value in script files
	sed -i "s?X-PREFIX-X?$(PREFIX)?" \
		$(DESTDIR)$(PREFIX)/bin/carla \
		$(DESTDIR)$(PREFIX)/bin/carla-database \
		$(DESTDIR)$(PREFIX)/bin/carla-patchbay \
		$(DESTDIR)$(PREFIX)/bin/carla-rack \
		$(DESTDIR)$(PREFIX)/bin/carla-settings \
		$(DESTDIR)$(PREFIX)/lib/pkgconfig/carla-standalone.pc

# 		$(DESTDIR)$(PREFIX)/bin/carla-single \
# 		$(DESTDIR)$(PREFIX)/bin/carla-control \

# --------------------------------------------------------------

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/carla*
	rm -f $(DESTDIR)$(PREFIX)/lib/pkgconfig/carla-standalone.pc
	rm -f $(DESTDIR)$(PREFIX)/share/applications/carla.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/carla-control.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/*/apps/carla.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/*/apps/carla-control.png
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/carla.svg
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/carla-control.svg
	rm -f $(DESTDIR)$(PREFIX)/share/mime/packages/carla.xml
	rm -rf $(DESTDIR)$(PREFIX)/include/carla/
	rm -rf $(DESTDIR)$(PREFIX)/lib/carla/
	rm -rf $(DESTDIR)$(PREFIX)/lib/lv2/carla-native.lv2/
	rm -rf $(DESTDIR)$(PREFIX)/share/carla/

# --------------------------------------------------------------

USE_COLORS=true
USE_VST3=false

ifeq ($(HAIKU),true)
USE_COLORS=false
endif

ifeq ($(USE_COLORS),true)
ANS_NO=\033[31m NO \033[0m
ANS_YES=\033[32m YES \033[0m
mS=\033[33m[
mZ=\033[30;1m[
mE=]\033[0m
tS=\033[36m
tE=\033[0m
else
ANS_NO=" NO "
ANS_YES=" YES "
endif

features:
	@echo "$(tS)---> Engine driver $(tE)"
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

	@echo "$(tS)---> Plugin formats: $(tE)"
	@echo "Internal:$(ANS_YES)"
ifeq ($(CARLA_PLUGIN_SUPPORT),true)
	@echo "LADSPA:  $(ANS_YES)"
	@echo "DSSI:    $(ANS_YES)"
	@echo "LV2:     $(ANS_YES)"
	@echo "VST:     $(ANS_YES)"
ifeq ($(MACOS_OR_WIN32),true)
	@echo "VST3:    $(ANS_YES)"
else
	@echo "VST3:    $(ANS_NO)  $(mZ)Windows and MacOS only$(mE)"
endif
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
	@echo "VST3:    $(ANS_NO)  $(mS)Plugins disabled$(mE)"
	@echo "AU:      $(ANS_NO)  $(mS)Plugins disabled$(mE)"
endif
	@echo ""

ifeq ($(CARLA_PLUGIN_SUPPORT),true)
	@echo "$(tS)---> LV2 UI toolkit support: $(tE)"
# 	@echo "External:$(ANS_YES) (direct+bridge)"
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
ifeq ($(HAVE_X11),true)
	@echo "X11:     $(ANS_YES) (direct+bridge)"
else
	@echo "X11:     $(ANS_NO)  $(mS)X11 missing$(mE)"
endif
# ifeq ($(MACOS),true)
# 	@echo "Cocoa:   $(ANS_YES) (direct+bridge)"
# else
# 	@echo "Cocoa:   $(ANS_NO)  $(mZ)MacOS only$(mE)"
# endif
# ifeq ($(WIN32),true)
# 	@echo "Windows: $(ANS_YES) (direct+bridge)"
# else
# 	@echo "Windows: $(ANS_NO)  $(mZ)Windows only$(mE)"
# endif
	@echo ""
endif

	@echo "$(tS)---> File formats: $(tE)"
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

	@echo "$(tS)---> Internal plugins: $(tE)"
ifneq ($(WIN32),true)
	@echo "Carla-Rack: $(ANS_YES)"
else
	@echo "Carla-Rack: $(ANS_NO)  $(mS)Not available for Windows$(mE)"
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

.FORCE:
.PHONY: .FORCE

# --------------------------------------------------------------
