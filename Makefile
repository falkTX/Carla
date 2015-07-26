#!/usr/bin/make -f
# Makefile for Carla #
# ------------------ #
# Created by falkTX
#

include source/Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

LINK := ln -sf

ifeq ($(DEFAULT_QT),4)
PYUIC ?= pyuic4 -w
PYRCC ?= pyrcc4 -py3
else
PYUIC ?= pyuic5
PYRCC ?= pyrcc5
endif

# ----------------------------------------------------------------------------------------------------------------------------

PREFIX     := /usr/local
BINDIR     := $(PREFIX)/bin
LIBDIR     := $(PREFIX)/lib
DATADIR    := $(PREFIX)/share
INCLUDEDIR := $(PREFIX)/include
DESTDIR    :=

ifeq ($(DEBUG),true)
MODULEDIR := $(CURDIR)/build/modules/Debug
else
MODULEDIR := $(CURDIR)/build/modules/Release
endif

# ----------------------------------------------------------------------------------------------------------------------------

all: BIN RES UI WIDGETS

# ----------------------------------------------------------------------------------------------------------------------------
# Binaries (native)

BIN: backend discovery bridges-plugin bridges-ui interposer plugin theme

# ----------------------------------------------------------------------------------------------------------------------------

ALL_LIBS  = $(MODULEDIR)/carla_engine.a
ALL_LIBS += $(MODULEDIR)/carla_engine_plugin.a
ALL_LIBS += $(MODULEDIR)/carla_plugin.a
ALL_LIBS += $(MODULEDIR)/jackbridge.a
ALL_LIBS += $(MODULEDIR)/native-plugins.a
ALL_LIBS += $(MODULEDIR)/juce_audio_basics.a
ALL_LIBS += $(MODULEDIR)/juce_audio_formats.a
ALL_LIBS += $(MODULEDIR)/juce_core.a
ALL_LIBS += $(MODULEDIR)/lilv.a
ALL_LIBS += $(MODULEDIR)/rtmempool.a

ifeq ($(HAVE_DGL),true)
ALL_LIBS += $(MODULEDIR)/dgl.a
endif

ifeq ($(MACOS_OR_WIN32),true)
ALL_LIBS += $(MODULEDIR)/juce_audio_devices.a
ALL_LIBS += $(MODULEDIR)/juce_audio_processors.a
ALL_LIBS += $(MODULEDIR)/juce_data_structures.a
ALL_LIBS += $(MODULEDIR)/juce_events.a
ALL_LIBS += $(MODULEDIR)/juce_graphics.a
ALL_LIBS += $(MODULEDIR)/juce_gui_basics.a
ifeq ($(MACOS),true)
ALL_LIBS += $(MODULEDIR)/juce_gui_extra.a
endif
else
ALL_LIBS += $(MODULEDIR)/rtaudio.a
ALL_LIBS += $(MODULEDIR)/rtmidi.a
endif

ifeq ($(HAVE_QT4),true)
ALL_LIBS += $(MODULEDIR)/theme.qt4.a
endif

ifeq ($(HAVE_QT5),true)
ALL_LIBS += $(MODULEDIR)/theme.qt5.a
endif

libs: $(ALL_LIBS)

$(MODULEDIR)/carla_engine.a: .FORCE
	@$(MAKE) -C source/backend/engine

$(MODULEDIR)/carla_engine_plugin.a: $(MODULEDIR)/carla_engine.a
	@$(MAKE) -C source/backend/engine

$(MODULEDIR)/carla_plugin.a: .FORCE
	@$(MAKE) -C source/backend/plugin

$(MODULEDIR)/jackbridge.a: .FORCE
	@$(MAKE) -C source/jackbridge

$(MODULEDIR)/jackbridge.%.a: .FORCE
	@$(MAKE) -C source/jackbridge $*

$(MODULEDIR)/native-plugins.a: .FORCE
	@$(MAKE) -C source/native-plugins

$(MODULEDIR)/theme.a: .FORCE
	@$(MAKE) -C source/theme

$(MODULEDIR)/theme.qt4.a: .FORCE
	@$(MAKE) -C source/theme qt4

$(MODULEDIR)/theme.qt5.a: .FORCE
	@$(MAKE) -C source/theme qt5

$(MODULEDIR)/%.posix32.a: .FORCE
	@$(MAKE) -C source/modules/$* posix32

$(MODULEDIR)/%.posix64.a: .FORCE
	@$(MAKE) -C source/modules/$* posix64

$(MODULEDIR)/%.win32.a: .FORCE
	@$(MAKE) -C source/modules/$* win32

$(MODULEDIR)/%.win64.a: .FORCE
	@$(MAKE) -C source/modules/$* win64

$(MODULEDIR)/%.a: .FORCE
	@$(MAKE) -C source/modules/$*

# ----------------------------------------------------------------------------------------------------------------------------

backend: libs
	@$(MAKE) -C source/backend

bridges-plugin: libs
	@$(MAKE) -C source/bridges-plugin

bridges-ui: libs
	@$(MAKE) -C source/bridges-ui

discovery: libs
	@$(MAKE) -C source/discovery

interposer:
	@$(MAKE) -C source/interposer

plugin: backend bridges-plugin bridges-ui discovery
	@$(MAKE) -C source/plugin

# FIXME
ifeq ($(HAVE_QT),true)
theme:
	@$(MAKE) -C source/theme
else
theme:
endif

# ----------------------------------------------------------------------------------------------------------------------------
# Binaries (posix32)

LIBS_POSIX32  = $(MODULEDIR)/jackbridge.posix32.a
LIBS_POSIX32 += $(MODULEDIR)/juce_audio_basics.posix32.a
LIBS_POSIX32 += $(MODULEDIR)/juce_core.posix32.a
LIBS_POSIX32 += $(MODULEDIR)/lilv.posix32.a
LIBS_POSIX32 += $(MODULEDIR)/rtmempool.posix32.a

ifeq ($(MACOS),true)
LIBS_POSIX32 += $(MODULEDIR)/juce_audio_processors.posix32.a
LIBS_POSIX32 += $(MODULEDIR)/juce_data_structures.posix32.a
LIBS_POSIX32 += $(MODULEDIR)/juce_events.posix32.a
LIBS_POSIX32 += $(MODULEDIR)/juce_graphics.posix32.a
LIBS_POSIX32 += $(MODULEDIR)/juce_gui_basics.posix32.a
LIBS_POSIX32 += $(MODULEDIR)/juce_gui_extra.posix32.a
endif

posix32: $(LIBS_POSIX32)
	$(MAKE) -C source/bridges-plugin posix32
	$(MAKE) -C source/discovery posix32

# ----------------------------------------------------------------------------------------------------------------------------
# Binaries (posix64)

LIBS_POSIX64  = $(MODULEDIR)/jackbridge.posix64.a
LIBS_POSIX64 += $(MODULEDIR)/juce_audio_basics.posix64.a
LIBS_POSIX64 += $(MODULEDIR)/juce_core.posix64.a
LIBS_POSIX64 += $(MODULEDIR)/lilv.posix64.a
LIBS_POSIX64 += $(MODULEDIR)/rtmempool.posix64.a

ifeq ($(MACOS),true)
LIBS_POSIX64 += $(MODULEDIR)/juce_audio_processors.posix64.a
LIBS_POSIX64 += $(MODULEDIR)/juce_data_structures.posix64.a
LIBS_POSIX64 += $(MODULEDIR)/juce_events.posix64.a
LIBS_POSIX64 += $(MODULEDIR)/juce_graphics.posix64.a
LIBS_POSIX64 += $(MODULEDIR)/juce_gui_basics.posix64.a
LIBS_POSIX64 += $(MODULEDIR)/juce_gui_extra.posix64.a
endif

posix64: $(LIBS_POSIX64)
	$(MAKE) -C source/bridges-plugin posix64
	$(MAKE) -C source/discovery posix64

# ----------------------------------------------------------------------------------------------------------------------------
# Binaries (win32)

LIBS_WIN32  = $(MODULEDIR)/jackbridge.win32e.a
LIBS_WIN32 += $(MODULEDIR)/juce_audio_basics.win32.a
LIBS_WIN32 += $(MODULEDIR)/juce_audio_processors.win32.a
LIBS_WIN32 += $(MODULEDIR)/juce_core.win32.a
LIBS_WIN32 += $(MODULEDIR)/juce_data_structures.win32.a
LIBS_WIN32 += $(MODULEDIR)/juce_events.win32.a
LIBS_WIN32 += $(MODULEDIR)/juce_graphics.win32.a
LIBS_WIN32 += $(MODULEDIR)/juce_gui_basics.win32.a
LIBS_WIN32 += $(MODULEDIR)/lilv.win32.a
LIBS_WIN32 += $(MODULEDIR)/rtmempool.win32.a

win32: $(LIBS_WIN32)
	$(MAKE) -C source/bridges-plugin win32
	$(MAKE) -C source/discovery win32

# ----------------------------------------------------------------------------------------------------------------------------
# Binaries (win64)

LIBS_WIN64  = $(MODULEDIR)/jackbridge.win64e.a
LIBS_WIN64 += $(MODULEDIR)/juce_audio_basics.win64.a
LIBS_WIN64 += $(MODULEDIR)/juce_audio_processors.win64.a
LIBS_WIN64 += $(MODULEDIR)/juce_core.win64.a
LIBS_WIN64 += $(MODULEDIR)/juce_data_structures.win64.a
LIBS_WIN64 += $(MODULEDIR)/juce_events.win64.a
LIBS_WIN64 += $(MODULEDIR)/juce_graphics.win64.a
LIBS_WIN64 += $(MODULEDIR)/juce_gui_basics.win64.a
LIBS_WIN64 += $(MODULEDIR)/lilv.win64.a
LIBS_WIN64 += $(MODULEDIR)/rtmempool.win64.a

win64: $(LIBS_WIN64)
	$(MAKE) -C source/bridges-plugin win64
	$(MAKE) -C source/discovery win64

# ----------------------------------------------------------------------------------------------------------------------------
# Binaries (wine)

wine32:
	$(MAKE) -C source/jackbridge wine32
	cp -f $(MODULEDIR)/jackbridge-wine32.dll.so $(CURDIR)/bin/jackbridge-wine32.dll

wine64:
	$(MAKE) -C source/jackbridge wine64
	cp -f $(MODULEDIR)/jackbridge-wine64.dll.so $(CURDIR)/bin/jackbridge-wine64.dll

# ----------------------------------------------------------------------------------------------------------------------------
# Resources

ifeq ($(HAVE_PYQT),true)
RES = \
	bin/resources/carla_app.py \
	bin/resources/carla_backend.py \
	bin/resources/carla_backend_qt.py \
	bin/resources/carla_config.py \
	bin/resources/carla_control.py \
	bin/resources/carla_database.py \
	bin/resources/carla_host.py \
	bin/resources/carla_panels.py \
	bin/resources/carla_settings.py \
	bin/resources/carla_skin.py \
	bin/resources/carla_shared.py \
	bin/resources/carla_utils.py \
	bin/resources/carla_widgets.py \
	bin/resources/canvaspreviewframe.py \
	bin/resources/digitalpeakmeter.py \
	bin/resources/externalui.py \
	bin/resources/ledbutton.py \
	bin/resources/paramspinbox.py \
	bin/resources/patchcanvas.py \
	bin/resources/patchcanvas_theme.py \
	bin/resources/pianoroll.py \
	bin/resources/pixmapbutton.py \
	bin/resources/pixmapdial.py \
	bin/resources/pixmapkeyboard.py \
	bin/resources/racklistwidget.py \
	bin/resources/resources_rc.py \
	bin/resources/ui_carla_about.py \
	bin/resources/ui_carla_about_juce.py \
	bin/resources/ui_carla_database.py \
	bin/resources/ui_carla_edit.py \
	bin/resources/ui_carla_host.py \
	bin/resources/ui_carla_panel_time.py \
	bin/resources/ui_carla_parameter.py \
	bin/resources/ui_carla_plugin_calf.py \
	bin/resources/ui_carla_plugin_classic.py \
	bin/resources/ui_carla_plugin_compact.py \
	bin/resources/ui_carla_plugin_default.py \
	bin/resources/ui_carla_plugin_presets.py \
	bin/resources/ui_carla_refresh.py \
	bin/resources/ui_carla_settings.py \
	bin/resources/ui_carla_settings_driver.py \
	bin/resources/ui_inputdialog_value.py \
	bin/resources/ui_midipattern.py \
	source/carla_config.py \
	source/resources_rc.py

RES: $(RES)

source/carla_config.py:
	@echo "#!/usr/bin/env python3" > $@
	@echo "# -*- coding: utf-8 -*-" >> $@
ifeq ($(DEFAULT_QT),4)
	@echo "config_UseQt5 = False" >> $@
else
	@echo "config_UseQt5 = True" >> $@
endif

source/resources_rc.py: resources/resources.qrc resources/*/*.png resources/*/*.svg
	$(PYRCC) $< -o $@

bin/resources/%.py: source/%.py
	$(LINK) $(CURDIR)/source/$*.py bin/resources/
else
RES:
endif

# ----------------------------------------------------------------------------------------------------------------------------
# UI code

ifeq ($(HAVE_PYQT),true)
UIs = \
	source/ui_carla_about.py \
	source/ui_carla_about_juce.py \
	source/ui_carla_database.py \
	source/ui_carla_edit.py \
	source/ui_carla_host.py \
	source/ui_carla_panel_time.py \
	source/ui_carla_parameter.py \
	source/ui_carla_plugin_calf.py \
	source/ui_carla_plugin_classic.py \
	source/ui_carla_plugin_compact.py \
	source/ui_carla_plugin_default.py \
	source/ui_carla_plugin_presets.py \
	source/ui_carla_refresh.py \
	source/ui_carla_settings.py \
	source/ui_carla_settings_driver.py \
	source/ui_inputdialog_value.py \
	source/ui_midipattern.py

UI: $(UIs)

source/ui_%.py: resources/ui/%.ui
	$(PYUIC) $< -o $@
else
UI:
endif

# ----------------------------------------------------------------------------------------------------------------------------
# Widgets

WIDGETS = \
	source/canvaspreviewframe.py \
	source/digitalpeakmeter.py \
	source/ledbutton.py \
	source/paramspinbox.py \
	source/pianoroll.py \
	source/pixmapbutton.py \
	source/pixmapdial.py \
	source/pixmapkeyboard.py \
	source/racklistwidget.py

WIDGETS: $(WIDGETS)

source/%.py: source/widgets/%.py
	$(LINK) widgets/$*.py $@

# ----------------------------------------------------------------------------------------------------------------------------

clean:
	$(MAKE) clean -C source/backend
	$(MAKE) clean -C source/bridges-plugin
	$(MAKE) clean -C source/bridges-ui
	$(MAKE) clean -C source/discovery
	$(MAKE) clean -C source/modules
	$(MAKE) clean -C source/plugin
	rm -f $(RES)
	rm -f $(UIs)
	rm -f $(WIDGETS)
	rm -f *~ source/*~ source/*.pyc source/*_rc.py source/ui_*.py

distclean: clean
	rm -f bin/*.exe bin/*.dll bin/*.dylib bin/*.so
	rm -rf build build-lv2

debug:
	$(MAKE) DEBUG=true

doxygen:
	$(MAKE) doxygen -C source/backend

stoat:
	stoat --recursive ./build/ --suppression ./data/stoat-supression.txt --whitelist ./data/stoat-whitelist.txt --graph-view ./data/stoat-callgraph.png

# 	stoat --recursive ./build/ \
# 	--suppression ./data/stoat-supression.txt \
# 	--whitelist   ./data/stoat-whitelist.txt  \
# 	--graph-view  ./data/stoat-callgraph.png

# ----------------------------------------------------------------------------------------------------------------------------

install:
	# Create directories
	install -d $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(LIBDIR)
	install -d $(DESTDIR)$(LIBDIR)/carla
	install -d $(DESTDIR)$(LIBDIR)/carla/styles
	install -d $(DESTDIR)$(LIBDIR)/pkgconfig
	install -d $(DESTDIR)$(LIBDIR)/python3/dist-packages
	install -d $(DESTDIR)$(DATADIR)
	install -d $(DESTDIR)$(DATADIR)/applications
	install -d $(DESTDIR)$(DATADIR)/carla
	install -d $(DESTDIR)$(DATADIR)/carla/resources
ifeq ($(EXPERIMENTAL_PLUGINS),true)
	install -d $(DESTDIR)$(DATADIR)/carla/resources/at1
	install -d $(DESTDIR)$(DATADIR)/carla/resources/bls1
	install -d $(DESTDIR)$(DATADIR)/carla/resources/rev1
endif
ifeq ($(HAVE_ZYN_DEPS),true)
ifeq ($(HAVE_ZYN_UI_DEPS),true)
	install -d $(DESTDIR)$(DATADIR)/carla/resources/zynaddsubfx
endif
endif
	install -d $(DESTDIR)$(DATADIR)/icons
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/16x16
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/16x16/apps
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/48x48
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/48x48/apps
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/128x128
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/128x128/apps
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/256x256
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/256x256/apps
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/scalable
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/scalable/apps
	install -d $(DESTDIR)$(DATADIR)/mime
	install -d $(DESTDIR)$(DATADIR)/mime/packages
	install -d $(DESTDIR)$(INCLUDEDIR)
	install -d $(DESTDIR)$(INCLUDEDIR)/carla
	install -d $(DESTDIR)$(INCLUDEDIR)/carla/includes
	install -d $(DESTDIR)$(INCLUDEDIR)/carla/utils

	# --------------------------------------------------------------------------------------------------------------------

	# Install script files
	install -m 755 \
		data/carla \
		data/carla-control \
		data/carla-database \
		data/carla-patchbay \
		data/carla-rack \
		data/carla-single \
		data/carla-settings \
		$(DESTDIR)$(BINDIR)

	# Install pkg-config files
	install -m 644 data/*.pc      $(DESTDIR)$(LIBDIR)/pkgconfig

	# Install desktop files
	install -m 644 data/*.desktop $(DESTDIR)$(DATADIR)/applications

	# Install mime package
	install -m 644 data/carla.xml $(DESTDIR)$(DATADIR)/mime/packages

	# Install icons, 16x16
	install -m 644 resources/16x16/carla.png            $(DESTDIR)$(DATADIR)/icons/hicolor/16x16/apps
	install -m 644 resources/16x16/carla-control.png    $(DESTDIR)$(DATADIR)/icons/hicolor/16x16/apps

	# Install icons, 48x48
	install -m 644 resources/48x48/carla.png            $(DESTDIR)$(DATADIR)/icons/hicolor/48x48/apps
	install -m 644 resources/48x48/carla-control.png    $(DESTDIR)$(DATADIR)/icons/hicolor/48x48/apps

	# Install icons, 128x128
	install -m 644 resources/128x128/carla.png          $(DESTDIR)$(DATADIR)/icons/hicolor/128x128/apps
	install -m 644 resources/128x128/carla-control.png  $(DESTDIR)$(DATADIR)/icons/hicolor/128x128/apps

	# Install icons, 256x256
	install -m 644 resources/256x256/carla.png          $(DESTDIR)$(DATADIR)/icons/hicolor/256x256/apps
	install -m 644 resources/256x256/carla-control.png  $(DESTDIR)$(DATADIR)/icons/hicolor/256x256/apps

	# Install icons, scalable
	install -m 644 resources/scalable/carla.svg         $(DESTDIR)$(DATADIR)/icons/hicolor/scalable/apps
	install -m 644 resources/scalable/carla-control.svg $(DESTDIR)$(DATADIR)/icons/hicolor/scalable/apps

	# Install backend libs
	install -m 644 \
		bin/libcarla_*.* \
		$(DESTDIR)$(LIBDIR)/carla

	# Install other binaries
	install -m 755 \
		bin/*bridge-* \
		bin/carla-discovery-* \
		$(DESTDIR)$(LIBDIR)/carla

	# Install the real modgui bridge
	install -m 755 \
		data/carla-bridge-lv2-modgui \
		$(DESTDIR)$(LIBDIR)/carla

ifeq ($(HAVE_QT),true)
	# Install theme
	install -m 644 \
		bin/styles/* \
		$(DESTDIR)$(LIBDIR)/carla/styles
endif

	# Install python code
	install -m 644 \
		source/carla \
		source/carla-control \
		source/carla-patchbay \
		source/carla-rack \
		source/*.py \
		$(DESTDIR)$(DATADIR)/carla

	install -m 644 \
		source/carla_backend.py \
		source/carla_utils.py \
		$(DESTDIR)$(LIBDIR)/python3/dist-packages

	# Install headers
	install -m 644 \
		source/backend/CarlaBackend.h \
		source/backend/CarlaHost.h \
		source/backend/CarlaUtils.h \
		source/backend/CarlaEngine.hpp \
		source/backend/CarlaPlugin.hpp \
		source/includes/CarlaNative.h \
		$(DESTDIR)$(INCLUDEDIR)/carla

	install -m 644 \
		source/includes/CarlaDefines.h \
		source/includes/CarlaMIDI.h \
		$(DESTDIR)$(INCLUDEDIR)/carla/includes

	install -m 644 \
		source/utils/CarlaUtils.hpp \
		source/utils/CarlaJuceUtils.hpp \
		source/utils/CarlaMathUtils.hpp \
		source/utils/CarlaPipeUtils.hpp \
		source/utils/CarlaPipeUtils.cpp \
		source/utils/CarlaExternalUI.hpp \
		source/utils/CarlaMutex.hpp \
		source/utils/CarlaString.hpp \
		$(DESTDIR)$(INCLUDEDIR)/carla/utils

	# Install resources
	install -m 755 \
		bin/resources/carla-plugin \
		bin/resources/carla-plugin-patchbay \
		bin/resources/*-ui \
		$(DESTDIR)$(DATADIR)/carla/resources

ifeq ($(EXPERIMENTAL_PLUGINS),true)
	install -m 644 \
		bin/resources/at1/*.png \
		$(DESTDIR)$(DATADIR)/carla/resources/at1

	install -m 644 \
		bin/resources/bls1/*.png \
		$(DESTDIR)$(DATADIR)/carla/resources/bls1

	install -m 644 \
		bin/resources/rev1/*.png \
		$(DESTDIR)$(DATADIR)/carla/resources/rev1

	install -m 755 \
		bin/resources/at1-ui \
		bin/resources/bls1-ui \
		bin/resources/rev1-ui \
		$(DESTDIR)$(DATADIR)/carla/resources
endif

ifeq ($(HAVE_ZYN_DEPS),true)
ifeq ($(HAVE_ZYN_UI_DEPS),true)
	install -m 644 \
		bin/resources/zynaddsubfx/*.png \
		$(DESTDIR)$(DATADIR)/carla/resources/zynaddsubfx

	install -m 755 \
		bin/resources/zynaddsubfx-ui \
		$(DESTDIR)$(DATADIR)/carla/resources
endif
endif

	# Install resources (re-use python files)
	$(LINK) $(DATADIR)/carla/carla_app.py                $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_backend.py            $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_backend_qt.py         $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_config.py             $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_control.py            $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_database.py           $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_host.py               $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_modgui.py             $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_panels.py             $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_settings.py           $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_skin.py               $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_shared.py             $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_utils.py              $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_widgets.py            $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/canvaspreviewframe.py       $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/digitalpeakmeter.py         $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/externalui.py               $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ledbutton.py                $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/paramspinbox.py             $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/patchcanvas.py              $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/patchcanvas_theme.py        $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/pianoroll.py                $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/pixmapbutton.py             $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/pixmapdial.py               $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/pixmapkeyboard.py           $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/racklistwidget.py           $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/resources_rc.py             $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ui_carla_about.py           $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ui_carla_about_juce.py      $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ui_carla_database.py        $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ui_carla_edit.py            $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ui_carla_host.py            $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ui_carla_panel_time.py      $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ui_carla_parameter.py       $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ui_carla_plugin_calf.py     $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ui_carla_plugin_classic.py  $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ui_carla_plugin_compact.py  $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ui_carla_plugin_default.py  $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ui_carla_plugin_presets.py  $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ui_carla_refresh.py         $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ui_carla_settings.py        $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ui_carla_settings_driver.py $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ui_inputdialog_value.py     $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ui_midipattern.py           $(DESTDIR)$(DATADIR)/carla/resources

	# Adjust PREFIX value in script files
	sed -i 's?X-PREFIX-X?$(PREFIX)?' \
		$(DESTDIR)$(BINDIR)/carla \
		$(DESTDIR)$(BINDIR)/carla-control \
		$(DESTDIR)$(BINDIR)/carla-database \
		$(DESTDIR)$(BINDIR)/carla-patchbay \
		$(DESTDIR)$(BINDIR)/carla-rack \
		$(DESTDIR)$(BINDIR)/carla-single \
		$(DESTDIR)$(BINDIR)/carla-settings \
		$(DESTDIR)$(LIBDIR)/carla/carla-bridge-lv2-modgui \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-standalone.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-utils.pc

	# Adjust LIBDIR and INCLUDEDIR in pkg-config files
	sed -i 's?X-LIBDIR-X?$(LIBDIR)?' \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-standalone.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-utils.pc

	sed -i 's?X-INCLUDEDIR-X?$(INCLUDEDIR)?' \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-standalone.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-utils.pc

	# Adjust LIBDIR and DATADIR value in code
	sed -i 's?X_LIBDIR_X = None?X_LIBDIR_X = "$(LIBDIR)"?' \
		$(DESTDIR)$(DATADIR)/carla/carla_shared.py

	sed -i 's?X_DATADIR_X = None?X_DATADIR_X = "$(DATADIR)"?' \
		$(DESTDIR)$(DATADIR)/carla/carla_shared.py

	# --------------------------------------------------------------------------------------------------------------------

	# Install lv2 plugin
	install -d $(DESTDIR)$(LIBDIR)/lv2/carla.lv2

	install -m 644 \
		bin/carla.lv2/carla.* \
		bin/carla.lv2/*.ttl \
		$(DESTDIR)$(LIBDIR)/lv2/carla.lv2

	# Link binaries for lv2 plugin
	@for i in $(shell find $(DESTDIR)$(LIBDIR)/carla/ -maxdepth 1 -type f -exec basename {} ';'); do \
		$(LINK) $(LIBDIR)/carla/$$i $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/$$i; \
		$(LINK) $(LIBDIR)/carla/$$i $(DESTDIR)$(LIBDIR)/vst/carla.vst/$$i; \
	done
	rm -f $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/libcarla_standalone2.*
	rm -f $(DESTDIR)$(LIBDIR)/vst/carla.vst/libcarla_standalone2.*

	# Link resources for lv2 plugin
	rm -rf $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/resources
	$(LINK) $(DATADIR)/carla/resources $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/resources

	# Link styles for lv2 plugin
	rm -rf $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/styles
	$(LINK) $(LIBDIR)/carla/styles $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/styles

	# --------------------------------------------------------------------------------------------------------------------

ifeq ($(LINUX),true)
ifeq ($(HAVE_X11),true)
ifeq ($(DEFAULT_QT),4)
	# Install vst plugin
	install -d $(DESTDIR)$(LIBDIR)/vst/carla.vst

	install -m 644 \
		bin/CarlaRack*.* \
		bin/CarlaPatchbay*.* \
		$(DESTDIR)$(LIBDIR)/vst/carla.vst

	# Link binaries for vst plugin
	@for i in $(shell find $(DESTDIR)$(LIBDIR)/carla/ -maxdepth 1 -type f -exec basename {} ';'); do \
		$(LINK) $(LIBDIR)/carla/$$i $(DESTDIR)$(LIBDIR)/vst/carla.vst/; \
	done
	rm -f $(DESTDIR)$(LIBDIR)/vst/carla.vst/libcarla_standalone2.*

	# Link resources for vst plugin
	rm -rf $(DESTDIR)$(LIBDIR)/vst/carla.vst/resources
	$(LINK) $(DATADIR)/carla/resources $(DESTDIR)$(LIBDIR)/vst/carla.vst/resources

	# Link styles for vst plugin
	rm -rf $(DESTDIR)$(LIBDIR)/vst/carla.vst/styles
	$(LINK) $(LIBDIR)/carla/styles $(DESTDIR)$(LIBDIR)/vst/carla.vst/styles
endif
endif
endif

# ----------------------------------------------------------------------------------------------------------------------------

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/carla*
	rm -f $(DESTDIR)$(LIBDIR)/pkgconfig/carla-standalone.pc
	rm -f $(DESTDIR)$(DATADIR)/applications/carla.desktop
	rm -f $(DESTDIR)$(DATADIR)/applications/carla-control.desktop
	rm -f $(DESTDIR)$(DATADIR)/icons/hicolor/*/apps/carla.png
	rm -f $(DESTDIR)$(DATADIR)/icons/hicolor/*/apps/carla-control.png
	rm -f $(DESTDIR)$(DATADIR)/icons/hicolor/scalable/apps/carla.svg
	rm -f $(DESTDIR)$(DATADIR)/icons/hicolor/scalable/apps/carla-control.svg
	rm -f $(DESTDIR)$(DATADIR)/mime/packages/carla.xml
	rm -rf $(DESTDIR)$(LIBDIR)/carla
	rm -rf $(DESTDIR)$(LIBDIR)/lv2/carla.lv2
	rm -rf $(DESTDIR)$(LIBDIR)/lv2/carla-native.lv2
	rm -rf $(DESTDIR)$(LIBDIR)/vst/carla.vst
	rm -rf $(DESTDIR)$(DATADIR)/carla
	rm -rf $(DESTDIR)$(INCLUDEDIR)/carla

# ----------------------------------------------------------------------------------------------------------------------------

USE_COLORS=true

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

ifeq ($(DEFAULT_QT),4)
FEV="Qt4"
else
FEV="Qt5"
endif

features:
	@echo "$(tS)---> Main features $(tE)"
ifeq ($(HAVE_PYQT),true)
	@echo "Front-End:  $(ANS_YES)(Using $(FEV))"
ifneq ($(WIN32),true)
	@echo "LV2 plugin: $(ANS_YES)"
else
	@echo "LV2 plugin: $(ANS_NO) $(mZ)Not available for Windows$(mE)"
endif
ifeq ($(LINUX),true)
ifeq ($(DEFAULT_QT),4)
ifeq ($(HAVE_X11),true)
	@echo "VST plugin: $(ANS_YES)"
else # HAVE_X11
	@echo "VST plugin: $(ANS_NO) $(mS)X11 missing$(mE)"
endif
else # DEFAULT_QT
	@echo "VST plugin: $(ANS_NO) $(mZ)Qt4 only$(mE)"
endif
else # LINUX
	@echo "VST plugin: $(ANS_NO) $(mZ)Linux only$(mE)"
endif
else
	@echo "Front-End:  $(ANS_NO) $(mS)Missing PyQt$(mE)"
	@echo "LV2 plugin: $(ANS_NO) $(mS)No front-end$(mE)"
	@echo "VST plugin: $(ANS_NO) $(mS)No front-end$(mE)"
endif
ifeq ($(HAVE_LIBLO),true)
	@echo "OSC support:$(ANS_YES)"
else
	@echo "OSC support:$(ANS_NO) $(mS)Missing liblo$(mE)"
endif
	@echo ""

	@echo "$(tS)---> Engine drivers $(tE)"
	@echo "JACK:       $(ANS_YES)"
ifeq ($(LINUX),true)
ifeq ($(HAVE_ALSA),true)
	@echo "ALSA:       $(ANS_YES)"
else
	@echo "ALSA:       $(ANS_NO) $(mS)Missing ALSA$(mE)"
endif
ifeq ($(HAVE_PULSEAUDIO),true)
	@echo "PulseAudio: $(ANS_YES)"
else
	@echo "PulseAudio: $(ANS_NO) $(mS)Missing PulseAudio$(mE)"
endif
else
	@echo "ALSA:       $(ANS_NO) $(mZ)Linux only$(mE)"
	@echo "PulseAudio: $(ANS_NO) $(mZ)Linux only$(mE)"
endif
ifeq ($(MACOS),true)
	@echo "CoreAudio:  $(ANS_YES)"
else
	@echo "CoreAudio:  $(ANS_NO) $(mZ)MacOS only$(mE)"
endif
ifeq ($(WIN32),true)
	@echo "ASIO:       $(ANS_YES)"
	@echo "DirectSound:$(ANS_YES)"
else
	@echo "ASIO:       $(ANS_NO) $(mZ)Windows only$(mE)"
	@echo "DirectSound:$(ANS_NO) $(mZ)Windows only$(mE)"
endif
	@echo ""

	@echo "$(tS)---> Plugin formats: $(tE)"
	@echo "Internal:$(ANS_YES)"
	@echo "LADSPA:  $(ANS_YES)"
	@echo "DSSI:    $(ANS_YES)"
	@echo "LV2:     $(ANS_YES)"
ifeq ($(MACOS_OR_WIN32),true)
	@echo "VST:     $(ANS_YES)(with UI)"
else
ifeq ($(LINUX),true)
ifeq ($(HAVE_X11),true)
	@echo "VST:     $(ANS_YES)(with UI)"
else
	@echo "VST:     $(ANS_YES)(without UI) $(mS)Missing X11$(mE)"
endif
else # LINUX
	@echo "VST:     $(ANS_YES)(without UI) $(mZ)Linux, Mac and Windows only$(mE)"
endif
endif
ifeq ($(MACOS_OR_WIN32),true)
	@echo "VST3:    $(ANS_YES)"
else
	@echo "VST3:    $(ANS_NO) $(mZ)Windows and MacOS only$(mE)"
endif
ifeq ($(MACOS),true)
	@echo "AU:      $(ANS_YES)"
else
	@echo "AU:      $(ANS_NO) $(mZ)MacOS only$(mE)"
endif
	@echo ""

	@echo "$(tS)---> LV2 UI toolkit support: $(tE)"
	@echo "External:$(ANS_YES)(direct)"
ifeq ($(LINUX),true)
ifeq ($(HAVE_GTK2),true)
	@echo "Gtk2:    $(ANS_YES)(bridge)"
else
	@echo "Gtk2:    $(ANS_NO) $(mS)Gtk2 missing$(mE)"
endif
ifeq ($(HAVE_GTK3),true)
	@echo "Gtk3:    $(ANS_YES)(bridge)"
else
	@echo "Gtk3:    $(ANS_NO) $(mS)Gtk3 missing$(mE)"
endif
ifeq ($(HAVE_QT4),true)
	@echo "Qt4:     $(ANS_YES)(bridge)"
else
	@echo "Qt4:     $(ANS_NO) $(mS)Qt4 missing$(mE)"
endif
ifeq ($(HAVE_QT5),true)
	@echo "Qt5:     $(ANS_YES)(bridge)"
else
	@echo "Qt5:     $(ANS_NO) $(mS)Qt5 missing$(mE)"
endif
ifeq ($(HAVE_X11),true)
	@echo "X11:     $(ANS_YES)(direct+bridge)"
else
	@echo "X11:     $(ANS_NO) $(mS)X11 missing$(mE)"
endif
else # LINUX
	@echo "Gtk2:    $(ANS_NO) $(mZ)Linux only$(mE)"
	@echo "Gtk3:    $(ANS_NO) $(mZ)Linux only$(mE)"
	@echo "Qt4:     $(ANS_NO) $(mZ)Linux only$(mE)"
	@echo "Qt5:     $(ANS_NO) $(mZ)Linux only$(mE)"
	@echo "X11:     $(ANS_NO) $(mZ)Linux only$(mE)"
endif # LINUX
ifeq ($(MACOS),true)
	@echo "Cocoa:   $(ANS_YES)(direct+bridge)"
else
	@echo "Cocoa:   $(ANS_NO) $(mZ)MacOS only$(mE)"
endif
ifeq ($(WIN32),true)
	@echo "Windows: $(ANS_YES)(direct+bridge)"
else
	@echo "Windows: $(ANS_NO) $(mZ)Windows only$(mE)"
endif
	@echo ""

	@echo "$(tS)---> File formats: $(tE)"
ifeq ($(HAVE_LINUXSAMPLER),true)
	@echo "GIG:$(ANS_YES)"
else
	@echo "GIG:$(ANS_NO)  $(mS)LinuxSampler missing or too old$(mE)"
endif
ifeq ($(HAVE_FLUIDSYNTH),true)
	@echo "SF2:$(ANS_YES)"
else
	@echo "SF2:$(ANS_NO)  $(mS)FluidSynth missing$(mE)"
endif
ifeq ($(HAVE_LINUXSAMPLER),true)
	@echo "SFZ:$(ANS_YES)"
else
	@echo "SFZ:$(ANS_NO)  $(mS)LinuxSampler missing or too old$(mE)"
endif
	@echo ""

	@echo "$(tS)---> Internal plugins: $(tE)"
ifneq ($(WIN32),true)
	@echo "Carla-Patchbay: $(ANS_YES)"
	@echo "Carla-Rack:     $(ANS_YES)"
else
	@echo "Carla-Patchbay: $(ANS_NO) $(mS)Not available for Windows$(mE)"
	@echo "Carla-Rack:     $(ANS_NO) $(mS)Not available for Windows$(mE)"
endif
ifeq ($(HAVE_DGL),true)
	@echo "DISTRHO Plugins:$(ANS_YES)(with UI)"
else
	@echo "DISTRHO Plugins:$(ANS_YES)(without UI)"
endif
ifeq ($(HAVE_PROJECTM),true)
	@echo "DISTRHO ProM:   $(ANS_YES)"
else
	@echo "DISTRHO ProM:   $(ANS_NO) (missing libprojectM)"
endif
ifeq ($(HAVE_ZYN_DEPS),true)
ifeq ($(HAVE_ZYN_UI_DEPS),true)
ifeq ($(HAVE_NTK),true)
	@echo "ZynAddSubFX:    $(ANS_YES)(with NTK UI)"
else
	@echo "ZynAddSubFX:    $(ANS_YES)(with FLTK UI)"
endif
else
	@echo "ZynAddSubFX:    $(ANS_YES)(without UI) $(mS)FLTK or NTK missing$(mE)"
endif
else
	@echo "ZynAddSubFX:    $(ANS_NO) $(mS)fftw3, mxml or zlib missing$(mE)"
endif

# ----------------------------------------------------------------------------------------------------------------------------

.FORCE:
.PHONY: .FORCE

# ----------------------------------------------------------------------------------------------------------------------------
