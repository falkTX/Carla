#!/usr/bin/make -f
# Makefile for Carla #
# ------------------ #
# Created by falkTX
#

CWD=source
include source/Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

LINK := ln -sf

ifeq ($(DEFAULT_QT),4)
PYUIC ?= pyuic4 -w
PYRCC ?= pyrcc4 -py3
ifeq ($(HAVE_QT4),true)
HAVE_THEME = true
endif
else
PYUIC ?= pyuic5
PYRCC ?= pyrcc5
ifeq ($(HAVE_QT5),true)
HAVE_THEME = true
endif
endif

# ---------------------------------------------------------------------------------------------------------------------

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

# ---------------------------------------------------------------------------------------------------------------------

all: BIN RES UI WIDGETS

# ---------------------------------------------------------------------------------------------------------------------
# Binaries (native)

BIN: backend discovery bridges-plugin bridges-ui interposer libjack plugin theme

# ---------------------------------------------------------------------------------------------------------------------

ALL_LIBS += $(MODULEDIR)/carla_engine.a
ALL_LIBS += $(MODULEDIR)/carla_engine_plugin.a
ALL_LIBS += $(MODULEDIR)/carla_plugin.a
ALL_LIBS += $(MODULEDIR)/jackbridge.a
ALL_LIBS += $(MODULEDIR)/native-plugins.a
ALL_LIBS += $(MODULEDIR)/audio_decoder.a
ALL_LIBS += $(MODULEDIR)/lilv.a
ALL_LIBS += $(MODULEDIR)/rtmempool.a
ALL_LIBS += $(MODULEDIR)/water.a

ifeq ($(HAVE_HYLIA),true)
ALL_LIBS += $(MODULEDIR)/hylia.a
endif

ALL_LIBS += $(MODULEDIR)/rtaudio.a
ALL_LIBS += $(MODULEDIR)/rtmidi.a

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

# ---------------------------------------------------------------------------------------------------------------------

backend: libs
	@$(MAKE) -C source/backend

bridges-plugin: libs
	@$(MAKE) -C source/bridges-plugin

bridges-ui: libs
	@$(MAKE) -C source/bridges-ui

discovery: libs
	@$(MAKE) -C source/discovery

interposer:
ifeq ($(LINUX),true)
	@$(MAKE) -C source/interposer
endif

libjack: libs
	@$(MAKE) -C source/libjack

plugin: backend bridges-plugin bridges-ui discovery
	@$(MAKE) -C source/plugin

theme: libs
	@$(MAKE) -C source/theme

# ---------------------------------------------------------------------------------------------------------------------
# Binaries (posix32)

LIBS_POSIX32  = $(MODULEDIR)/jackbridge.posix32.a
LIBS_POSIX32 += $(MODULEDIR)/lilv.posix32.a
LIBS_POSIX32 += $(MODULEDIR)/rtmempool.posix32.a
LIBS_POSIX32 += $(MODULEDIR)/water.posix32.a

posix32: $(LIBS_POSIX32)
	$(MAKE) -C source/bridges-plugin posix32
	$(MAKE) -C source/discovery posix32

# ---------------------------------------------------------------------------------------------------------------------
# Binaries (posix64)

LIBS_POSIX64  = $(MODULEDIR)/jackbridge.posix64.a
LIBS_POSIX64 += $(MODULEDIR)/lilv.posix64.a
LIBS_POSIX64 += $(MODULEDIR)/rtmempool.posix64.a
LIBS_POSIX64 += $(MODULEDIR)/water.posix64.a

posix64: $(LIBS_POSIX64)
	$(MAKE) -C source/bridges-plugin posix64
	$(MAKE) -C source/discovery posix64

# ---------------------------------------------------------------------------------------------------------------------
# Binaries (win32)

ifeq ($(BUILDING_FOR_WINDOWS),true)
LIBS_WIN32  = $(MODULEDIR)/jackbridge.win32.a
else
LIBS_WIN32  = $(MODULEDIR)/jackbridge.win32e.a
endif
LIBS_WIN32 += $(MODULEDIR)/lilv.win32.a
LIBS_WIN32 += $(MODULEDIR)/rtmempool.win32.a
LIBS_WIN32 += $(MODULEDIR)/water.win32.a

win32: $(LIBS_WIN32)
	$(MAKE) -C source/bridges-plugin win32
	$(MAKE) -C source/discovery win32

# ---------------------------------------------------------------------------------------------------------------------
# Binaries (win64)

ifeq ($(BUILDING_FOR_WINDOWS),true)
LIBS_WIN64  = $(MODULEDIR)/jackbridge.win64.a
else
LIBS_WIN64  = $(MODULEDIR)/jackbridge.win64e.a
endif
LIBS_WIN64 += $(MODULEDIR)/lilv.win64.a
LIBS_WIN64 += $(MODULEDIR)/rtmempool.win64.a
LIBS_WIN64 += $(MODULEDIR)/water.win64.a

win64: $(LIBS_WIN64)
	$(MAKE) -C source/bridges-plugin win64
	$(MAKE) -C source/discovery win64

# ---------------------------------------------------------------------------------------------------------------------
# Binaries (wine)

wine32:
	$(MAKE) -C source/jackbridge wine32
	cp -f $(MODULEDIR)/jackbridge-wine32.dll.so $(CURDIR)/bin/jackbridge-wine32.dll

wine64:
	$(MAKE) -C source/jackbridge wine64
	cp -f $(MODULEDIR)/jackbridge-wine64.dll.so $(CURDIR)/bin/jackbridge-wine64.dll

# ---------------------------------------------------------------------------------------------------------------------
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
	bin/resources/carla_settings.py \
	bin/resources/carla_skin.py \
	bin/resources/carla_shared.py \
	bin/resources/carla_utils.py \
	bin/resources/carla_widgets.py \
	bin/resources/canvaspreviewframe.py \
	bin/resources/digitalpeakmeter.py \
	bin/resources/draggablegraphicsview.py \
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
	bin/resources/ui_carla_add_jack.py \
	bin/resources/ui_carla_database.py \
	bin/resources/ui_carla_edit.py \
	bin/resources/ui_carla_host.py \
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

# ---------------------------------------------------------------------------------------------------------------------
# UI code

ifeq ($(HAVE_PYQT),true)
UIs = \
	source/ui_carla_about.py \
	source/ui_carla_add_jack.py \
	source/ui_carla_database.py \
	source/ui_carla_edit.py \
	source/ui_carla_host.py \
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

# ---------------------------------------------------------------------------------------------------------------------
# Widgets

WIDGETS = \
	source/canvaspreviewframe.py \
	source/digitalpeakmeter.py \
	source/draggablegraphicsview.py \
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

# ---------------------------------------------------------------------------------------------------------------------

clean:
	$(MAKE) clean -C source/backend
	$(MAKE) clean -C source/bridges-plugin
	$(MAKE) clean -C source/bridges-ui
	$(MAKE) clean -C source/discovery
	$(MAKE) clean -C source/interposer
	$(MAKE) clean -C source/libjack
	$(MAKE) clean -C source/modules
	$(MAKE) clean -C source/native-plugins
	$(MAKE) clean -C source/plugin
	$(MAKE) clean -C source/theme
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

# ---------------------------------------------------------------------------------------------------------------------

install_main:
	# Create directories
	install -d $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(LIBDIR)/carla/jack
	install -d $(DESTDIR)$(LIBDIR)/pkgconfig
	install -d $(DESTDIR)$(LIBDIR)/python3/dist-packages
	install -d $(DESTDIR)$(DATADIR)/carla
	install -d $(DESTDIR)$(DATADIR)/carla/resources
	install -d $(DESTDIR)$(INCLUDEDIR)/carla
	install -d $(DESTDIR)$(INCLUDEDIR)/carla/includes
	install -d $(DESTDIR)$(INCLUDEDIR)/carla/utils

ifeq ($(HAVE_PYQT),true)
	# Create directories (gui)
	install -d $(DESTDIR)$(LIBDIR)/carla/styles
	install -d $(DESTDIR)$(DATADIR)/applications
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/16x16/apps
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/48x48/apps
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/128x128/apps
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/256x256/apps
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/scalable/apps
	install -d $(DESTDIR)$(DATADIR)/mime/packages
endif

	# -------------------------------------------------------------------------------------------------------------

	# Install script files (non-gui)
	install -m 755 \
		data/carla-single \
		$(DESTDIR)$(BINDIR)

	# Adjust PREFIX value in script files (non-gui)
	sed -i 's?X-PREFIX-X?$(PREFIX)?' \
		$(DESTDIR)$(BINDIR)/carla-single

	# Install backend libs
	install -m 644 \
		bin/libcarla_*.* \
		$(DESTDIR)$(LIBDIR)/carla

	# Install other binaries
	install -m 755 \
		bin/*bridge-* \
		bin/carla-discovery-* \
		$(DESTDIR)$(LIBDIR)/carla

ifeq ($(LINUX),true)
	# Install libjack
	install -m 755 \
		bin/jack/libjack.so.0 \
		$(DESTDIR)$(LIBDIR)/carla/jack
endif

	# Install pkg-config files
	install -m 644 \
		data/*.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig

	# Adjust PREFIX, LIBDIR and INCLUDEDIR in pkg-config files
	sed -i 's?X-PREFIX-X?$(PREFIX)?' \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-standalone.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-utils.pc

	sed -i 's?X-LIBDIR-X?$(LIBDIR)?' \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-standalone.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-utils.pc

	sed -i 's?X-INCLUDEDIR-X?$(INCLUDEDIR)?' \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-standalone.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-utils.pc

	# Install python code (dist-packages)
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

	# -------------------------------------------------------------------------------------------------------------

ifeq ($(HAVE_PYQT),true)
	# Install script files (gui)
	install -m 755 \
		data/carla \
		data/carla-database \
		data/carla-jack-multi \
		data/carla-jack-single \
		data/carla-patchbay \
		data/carla-rack \
		data/carla-settings \
		$(DESTDIR)$(BINDIR)

	# Adjust PREFIX value in script files (gui)
	sed -i 's?X-PREFIX-X?$(PREFIX)?' \
		$(DESTDIR)$(BINDIR)/carla \
		$(DESTDIR)$(BINDIR)/carla-database \
		$(DESTDIR)$(BINDIR)/carla-jack-multi \
		$(DESTDIR)$(BINDIR)/carla-jack-single \
		$(DESTDIR)$(BINDIR)/carla-patchbay \
		$(DESTDIR)$(BINDIR)/carla-rack \
		$(DESTDIR)$(BINDIR)/carla-settings

ifeq ($(HAVE_LIBLO),true)
	install -m 755 \
		data/carla-control \
		$(DESTDIR)$(BINDIR)

	sed -i 's?X-PREFIX-X?$(PREFIX)?' \
		$(DESTDIR)$(BINDIR)/carla-control
endif

	# Install the real modgui bridge
	install -m 755 \
		data/carla-bridge-lv2-modgui \
		$(DESTDIR)$(LIBDIR)/carla

	# Adjust PREFIX value in modgui bridge
	sed -i 's?X-PREFIX-X?$(PREFIX)?' \
		$(DESTDIR)$(LIBDIR)/carla/carla-bridge-lv2-modgui

	# Install python code (gui)
	install -m 644 \
		source/carla \
		source/carla-control \
		source/carla-jack-multi \
		source/carla-jack-single \
		source/carla-patchbay \
		source/carla-rack \
		source/*.py \
		$(DESTDIR)$(DATADIR)/carla

	# Adjust LIBDIR and DATADIR value in python code
	sed -i 's?X_LIBDIR_X = None?X_LIBDIR_X = "$(LIBDIR)"?' \
		$(DESTDIR)$(DATADIR)/carla/carla_shared.py

	sed -i 's?X_DATADIR_X = None?X_DATADIR_X = "$(DATADIR)"?' \
		$(DESTDIR)$(DATADIR)/carla/carla_shared.py

	# Install resources (gui)
	install -m 755 \
		bin/resources/carla-plugin \
		bin/resources/carla-plugin-patchbay \
		bin/resources/*-ui \
		$(DESTDIR)$(DATADIR)/carla/resources

ifeq ($(HAVE_THEME),true)
	# Install theme
	install -m 644 \
		bin/styles/* \
		$(DESTDIR)$(LIBDIR)/carla/styles
endif

	# Install desktop files
	install -m 644 data/carla.desktop         $(DESTDIR)$(DATADIR)/applications
ifeq ($(HAVE_LIBLO),true)
	install -m 644 data/carla-control.desktop $(DESTDIR)$(DATADIR)/applications
endif

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

	# Install resources (re-use python files)
	$(LINK) $(DATADIR)/carla/carla_app.py                $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_backend.py            $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_backend_qt.py         $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_config.py             $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_control.py            $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_database.py           $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_host.py               $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_modgui.py             $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_settings.py           $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_skin.py               $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_shared.py             $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_utils.py              $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/carla_widgets.py            $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/canvaspreviewframe.py       $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/digitalpeakmeter.py         $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/draggablegraphicsview.py    $(DESTDIR)$(DATADIR)/carla/resources
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
	$(LINK) $(DATADIR)/carla/ui_carla_add_jack.py        $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ui_carla_database.py        $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ui_carla_edit.py            $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) $(DATADIR)/carla/ui_carla_host.py            $(DESTDIR)$(DATADIR)/carla/resources
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
endif

	# -------------------------------------------------------------------------------------------------------------

	# Install lv2 plugin
	install -d $(DESTDIR)$(LIBDIR)/lv2/carla.lv2

	install -m 644 \
		bin/carla.lv2/carla.* \
		bin/carla.lv2/*.ttl \
		$(DESTDIR)$(LIBDIR)/lv2/carla.lv2

	# Link binaries for lv2 plugin
	@for i in `find $(DESTDIR)$(LIBDIR)/carla/ -maxdepth 1 -type f -exec basename {} ';'`; do \
		$(LINK) $(LIBDIR)/carla/$$i $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/$$i; \
	done
	rm -f $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/libcarla_standalone2.*

	# Link resources for lv2 plugin
	rm -rf $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/resources
	$(LINK) $(DATADIR)/carla/resources $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/resources

ifeq ($(HAVE_PYQT),true)
	# Link styles for lv2 plugin
	rm -rf $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/styles
	$(LINK) $(LIBDIR)/carla/styles $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/styles
endif

	# -------------------------------------------------------------------------------------------------------------

ifeq ($(LINUX),true)
ifeq ($(HAVE_X11),true)
ifeq ($(HAVE_PYQT),true)
	# Install vst plugin
	install -d $(DESTDIR)$(LIBDIR)/vst/carla.vst

	install -m 644 \
		bin/CarlaRack*.* \
		bin/CarlaPatchbay*.* \
		$(DESTDIR)$(LIBDIR)/vst/carla.vst

	# Link binaries for vst plugin
	@for i in `find $(DESTDIR)$(LIBDIR)/carla/ -maxdepth 1 -type f -exec basename {} ';'`; do \
		$(LINK) $(LIBDIR)/carla/$$i $(DESTDIR)$(LIBDIR)/vst/carla.vst/$$i; \
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

	# -------------------------------------------------------------------------------------------------------------

ifneq ($(HAVE_PYQT),true)
	# Remove gui files for non-gui build
	rm $(DESTDIR)$(LIBDIR)/carla/carla-bridge-lv2-modgui
	rm $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/carla-bridge-lv2-modgui
endif

ifneq ($(EXTERNAL_PLUGINS),true)
install_external_plugins:
endif

install: install_main install_external_plugins

# ---------------------------------------------------------------------------------------------------------------------

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

ifeq ($(DEFAULT_QT),4)
FEV="Qt4"
else
FEV="Qt5"
endif

features_print_main:
	@printf -- "$(tS)---> Main features $(tE)\n"
ifeq ($(HAVE_PYQT),true)
	@printf -- "Front-End:    $(ANS_YES) (Using $(FEV))\n"
ifneq ($(WIN32),true)
	@printf -- "LV2 plugin:   $(ANS_YES)\n"
else
	@printf -- "LV2 plugin:   $(ANS_NO)  $(mZ)Not available for Windows$(mE)\n"
endif
ifeq ($(LINUX),true)
ifeq ($(HAVE_X11),true)
	@printf -- "VST plugin:   $(ANS_YES)\n"
else # HAVE_X11
	@printf -- "VST plugin:   $(ANS_NO)  $(mS)X11 missing$(mE)\n"
endif
else # LINUX
	@printf -- "VST plugin:   $(ANS_NO)  $(mZ)Linux only$(mE)\n"
endif
else
	@printf -- "Front-End:    $(ANS_NO)  $(mS)Missing PyQt$(mE)\n"
	@printf -- "LV2 plugin:   $(ANS_NO)  $(mS)No front-end$(mE)\n"
	@printf -- "VST plugin:   $(ANS_NO)  $(mS)No front-end$(mE)\n"
endif
ifeq ($(HAVE_HYLIA),true)
	@printf -- "Link support: $(ANS_YES)\n"
else
	@printf -- "Link support: $(ANS_NO)  $(mZ)Linux, MacOS and Windows only$(mE)\n"
endif
ifeq ($(HAVE_LIBLO),true)
	@printf -- "OSC support:  $(ANS_YES)\n"
else
	@printf -- "OSC support:  $(ANS_NO)  $(mS)Missing liblo$(mE)\n"
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
ifneq ($(MACOS_OR_WIN32),true)
ifeq ($(HAVE_PULSEAUDIO),true)
	@printf -- "PulseAudio:  $(ANS_YES)\n"
else
	@printf -- "PulseAudio:  $(ANS_NO)  $(mS)Missing PulseAudio$(mE)\n"
endif
else
	@printf -- "PulseAudio:  $(ANS_NO)  $(mZ)Not available for Windows or MacOS$(mE)\n"
endif
ifeq ($(MACOS),true)
	@printf -- "CoreAudio:   $(ANS_YES)\n"
else
	@printf -- "CoreAudio:   $(ANS_NO)  $(mZ)MacOS only$(mE)\n"
endif
ifeq ($(WIN32),true)
	@printf -- "ASIO:        $(ANS_YES)\n"
	@printf -- "DirectSound: $(ANS_YES)\n"
else
	@printf -- "ASIO:        $(ANS_NO)  $(mZ)Windows only$(mE)\n"
	@printf -- "DirectSound: $(ANS_NO)  $(mZ)Windows only$(mE)\n"
endif
	@printf -- "\n"

	@printf -- "$(tS)---> Plugin formats: $(tE)\n"
	@printf -- "Internal: $(ANS_YES)\n"
	@printf -- "LADSPA:   $(ANS_YES)\n"
	@printf -- "DSSI:     $(ANS_YES)\n"
	@printf -- "LV2:      $(ANS_YES)\n"
ifeq ($(MACOS_OR_WIN32),true)
	@printf -- "VST:      $(ANS_YES) (with UI)\n"
else
ifeq ($(HAIKU),true)
	@printf -- "VST:      $(ANS_YES) (without UI)\n"
else
ifeq ($(HAVE_X11),true)
	@printf -- "VST:      $(ANS_YES) (with UI)\n"
else
	@printf -- "VST:      $(ANS_YES) (without UI) $(mS)Missing X11$(mE)\n"
endif
endif
endif
	@printf -- "\n"

	@printf -- "$(tS)---> LV2 UI toolkit support: $(tE)\n"
	@printf -- "External: $(ANS_YES) (direct)\n"
ifneq ($(MACOS_OR_WIN32),true)
ifeq ($(HAVE_GTK2),true)
	@printf -- "Gtk2:     $(ANS_YES) (bridge)\n"
else
	@printf -- "Gtk2:     $(ANS_NO)  $(mS)Gtk2 missing$(mE)\n"
endif
ifeq ($(HAVE_GTK3),true)
	@printf -- "Gtk3:     $(ANS_YES) (bridge)\n"
else
	@printf -- "Gtk3:     $(ANS_NO)  $(mS)Gtk3 missing$(mE)\n"
endif
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
else # LINUX
	@printf -- "Gtk2:     $(ANS_NO)  $(mZ)Not available for Windows or MacOS$(mE)\n"
	@printf -- "Gtk3:     $(ANS_NO)  $(mZ)Not available for Windows or MacOS$(mE)\n"
	@printf -- "Qt4:      $(ANS_NO)  $(mZ)Not available for Windows or MacOS$(mE)\n"
	@printf -- "Qt5:      $(ANS_NO)  $(mZ)Not available for Windows or MacOS$(mE)\n"
	@printf -- "X11:      $(ANS_NO)  $(mZ)Not available for Windows or MacOS$(mE)\n"
endif # LINUX
ifeq ($(MACOS),true)
	@printf -- "Cocoa:    $(ANS_YES) (direct+bridge)\n"
else
	@printf -- "Cocoa:    $(ANS_NO)  $(mZ)MacOS only$(mE)\n"
endif
ifeq ($(WIN32),true)
	@printf -- "Windows:  $(ANS_YES) (direct+bridge)\n"
else
	@printf -- "Windows:  $(ANS_NO)  $(mZ)Windows only$(mE)\n"
endif
	@printf -- "\n"

	@printf -- "$(tS)---> File formats: $(tE)\n"
ifeq ($(HAVE_LINUXSAMPLER),true)
	@printf -- "GIG: $(ANS_YES)\n"
else
	@printf -- "GIG: $(ANS_NO)    $(mS)LinuxSampler missing or too old$(mE)\n"
endif
ifeq ($(HAVE_FLUIDSYNTH),true)
	@printf -- "SF2: $(ANS_YES)\n"
else
	@printf -- "SF2: $(ANS_NO)    $(mS)FluidSynth missing$(mE)\n"
endif
ifeq ($(HAVE_LINUXSAMPLER),true)
	@printf -- "SFZ: $(ANS_YES)\n"
else
	@printf -- "SFZ: $(ANS_NO)    $(mS)LinuxSampler missing or too old$(mE)\n"
endif
	@printf -- "\n"

	@printf -- "$(tS)---> Internal plugins: $(tE)\n"
	@printf -- "Basic Plugins:    $(ANS_YES)\n"
ifneq ($(WIN32),true)
	@printf -- "Carla-Patchbay:   $(ANS_YES)\n"
	@printf -- "Carla-Rack:       $(ANS_YES)\n"
else
	@printf -- "Carla-Patchbay:   $(ANS_NO)   $(mS)Not available for Windows$(mE)\n"
	@printf -- "Carla-Rack:       $(ANS_NO)   $(mS)Not available for Windows$(mE)\n"
endif
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

.FORCE:
.PHONY: .FORCE

# ---------------------------------------------------------------------------------------------------------------------
