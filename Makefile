#!/usr/bin/make -f
# Makefile for Carla #
# ------------------ #
# Created by falkTX
#

CWD=source
include source/Makefile.mk

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

all: backend discovery bridges-plugin bridges-ui frontend interposer libjack plugin theme

# ---------------------------------------------------------------------------------------------------------------------
# Binaries (native)

ALL_LIBS += $(MODULEDIR)/carla_engine.a
ALL_LIBS += $(MODULEDIR)/carla_engine_plugin.a
ALL_LIBS += $(MODULEDIR)/carla_plugin.a
ALL_LIBS += $(MODULEDIR)/jackbridge.a
ALL_LIBS += $(MODULEDIR)/native-plugins.a
ALL_LIBS += $(MODULEDIR)/audio_decoder.a
ALL_LIBS += $(MODULEDIR)/lilv.a
ALL_LIBS += $(MODULEDIR)/rtmempool.a
ALL_LIBS += $(MODULEDIR)/sfzero.a
ALL_LIBS += $(MODULEDIR)/water.a

ifeq ($(HAVE_DGL),true)
ALL_LIBS += $(MODULEDIR)/dgl.a
endif

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
ifneq ($(WIN32),true)
	@$(MAKE) -C source/modules/$* posix32
else
	$(error Trying to build POSIX binaries with a Windows toolchain, this cannot work)
endif

$(MODULEDIR)/%.posix64.a: .FORCE
ifneq ($(WIN32),true)
	@$(MAKE) -C source/modules/$* posix64
else
	$(error Trying to build POSIX binaries with a Windows toolchain, this cannot work)
endif

$(MODULEDIR)/%.win32.a: .FORCE
ifeq ($(WIN32),true)
	@$(MAKE) -C source/modules/$* win32
else
	$(error Trying to build Windows binaries with a regular toolchain, this cannot work)
endif

$(MODULEDIR)/%.win64.a: .FORCE
ifeq ($(WIN32),true)
	@$(MAKE) -C source/modules/$* win64
else
	$(error Trying to build Windows binaries with a regular toolchain, this cannot work)
endif

$(MODULEDIR)/dgl.wine.a: .FORCE
	@$(MAKE) -C source/modules/dgl wine

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

frontend:
ifeq ($(HAVE_PYQT),true)
	@$(MAKE) -C source/frontend
endif

interposer:
ifeq ($(LINUX),true)
	@$(MAKE) -C source/interposer
endif

libjack: libs
	@$(MAKE) -C source/libjack

plugin: backend bridges-plugin bridges-ui discovery
	@$(MAKE) -C source/plugin

ifeq ($(WIN32),true)
plugin-wine:
	@$(MAKE) -C source/plugin wine
else
plugin-wine: $(MODULEDIR)/dgl.wine.a
	@$(MAKE) -C source/plugin wine
endif

rest: libs
	@$(MAKE) -C source/rest

theme: libs
	@$(MAKE) -C source/theme

# ---------------------------------------------------------------------------------------------------------------------
# nuitka

nuitka: bin/carla bin/carla-rack bin/carla-plugin

bin/carla:
	python3 -m nuitka \
		-j 8 \
		--recurse-all \
		--python-flag -O --warn-unusual-code --warn-implicit-exceptions \
		--recurse-not-to=PyQt5 \
		--file-reference-choice=runtime \
		-o ./$@ \
		./source/frontend/carla

bin/carla-rack:
	python3 -m nuitka \
		-j 8 \
		--recurse-all \
		--python-flag -O --warn-unusual-code --warn-implicit-exceptions \
		--recurse-not-to=PyQt5 \
		--file-reference-choice=runtime \
		-o ./$@ \
		./source/frontend/carla

bin/carla-plugin:
	python3 -m nuitka \
		-j 8 \
		--recurse-all \
		--python-flag -O --warn-unusual-code --warn-implicit-exceptions \
		--recurse-not-to=PyQt5 \
		--file-reference-choice=runtime \
		-o ./$@ \
		./source/native-plugins/resources/carla-plugin

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

clean:
	$(MAKE) clean -C source/backend
	$(MAKE) clean -C source/bridges-plugin
	$(MAKE) clean -C source/bridges-ui
	$(MAKE) clean -C source/discovery
	$(MAKE) clean -C source/frontend
	$(MAKE) clean -C source/interposer
	$(MAKE) clean -C source/libjack
	$(MAKE) clean -C source/modules
	$(MAKE) clean -C source/native-plugins
	$(MAKE) clean -C source/plugin
	$(MAKE) clean -C source/theme
	rm -f *~ source/*~

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
ifeq ($(LINUX),true)
	install -d $(DESTDIR)$(LIBDIR)/carla/jack
else
	install -d $(DESTDIR)$(LIBDIR)/carla
endif
	install -d $(DESTDIR)$(LIBDIR)/pkgconfig
	install -d $(DESTDIR)$(INCLUDEDIR)/carla/includes

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
	install -d $(DESTDIR)$(DATADIR)/carla/resources
	install -d $(DESTDIR)$(DATADIR)/carla/widgets
endif

	# -------------------------------------------------------------------------------------------------------------

	# Install script files (non-gui)
	install -m 755 \
		data/carla-single \
		$(DESTDIR)$(BINDIR)

	# Adjust PREFIX value in script files (non-gui)
	sed $(SED_ARGS) 's?X-PREFIX-X?$(PREFIX)?' \
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
	sed $(SED_ARGS) 's?X-PREFIX-X?$(PREFIX)?' \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-native-plugin.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-standalone.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-utils.pc

	sed $(SED_ARGS) 's?X-LIBDIR-X?$(LIBDIR)?' \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-native-plugin.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-standalone.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-utils.pc

	sed $(SED_ARGS) 's?X-INCLUDEDIR-X?$(INCLUDEDIR)?' \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-native-plugin.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-standalone.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-utils.pc

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
		source/includes/CarlaNative.h \
		source/includes/CarlaNativePlugin.h \
		$(DESTDIR)$(INCLUDEDIR)/carla/includes

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
	sed $(SED_ARGS) 's?X-PREFIX-X?$(PREFIX)?' \
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

	sed $(SED_ARGS) 's?X-PREFIX-X?$(PREFIX)?' \
		$(DESTDIR)$(BINDIR)/carla-control
endif

	# Install frontend
	install -m 644 \
		source/frontend/carla \
		source/frontend/carla-control \
		source/frontend/carla-jack-multi \
		source/frontend/carla-jack-single \
		source/frontend/carla-patchbay \
		source/frontend/carla-rack \
		source/frontend/*.py \
		$(DESTDIR)$(DATADIR)/carla/

	install -m 644 \
		source/frontend/widgets/*.py \
		$(DESTDIR)$(DATADIR)/carla/widgets/

	# Adjust LIBDIR and DATADIR value in python code
	sed $(SED_ARGS) 's?X_LIBDIR_X = None?X_LIBDIR_X = "$(LIBDIR)"?' \
		$(DESTDIR)$(DATADIR)/carla/carla_shared.py

	sed $(SED_ARGS) 's?X_DATADIR_X = None?X_DATADIR_X = "$(DATADIR)"?' \
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
	$(LINK) ../widgets                     $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_app.py                $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_backend.py            $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_backend_qt.py         $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_config.py             $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_control.py            $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_database.py           $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_host.py               $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_settings.py           $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_skin.py               $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_shared.py             $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_utils.py              $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_widgets.py            $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../externalui.py               $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../patchcanvas.py              $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../patchcanvas_theme.py        $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../resources_rc.py             $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_about.py           $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_add_jack.py        $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_database.py        $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_edit.py            $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_host.py            $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_parameter.py       $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_plugin_calf.py     $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_plugin_classic.py  $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_plugin_compact.py  $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_plugin_default.py  $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_plugin_presets.py  $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_refresh.py         $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_settings.py        $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_settings_driver.py $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_inputdialog_value.py     $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_midipattern.py           $(DESTDIR)$(DATADIR)/carla/resources
endif # HAVE_PYQT

	# -------------------------------------------------------------------------------------------------------------

	# Install lv2 plugin
	install -d $(DESTDIR)$(LIBDIR)/lv2/carla.lv2

	install -m 644 \
		bin/carla.lv2/carla.* \
		bin/carla.lv2/*.ttl \
		$(DESTDIR)$(LIBDIR)/lv2/carla.lv2

	# Link binaries for lv2 plugin
	@for i in `find $(DESTDIR)$(LIBDIR)/carla/ -maxdepth 1 -type f -exec basename {} ';'`; do \
		$(LINK) ../../carla/$$i $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/$$i; \
	done
	rm -f $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/libcarla_standalone2.*

ifeq ($(LINUX),true)
	# Link jack app bridge
	rm -rf $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/jack
	$(LINK) ../../carla/jack $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/jack
endif

	# Link resources for lv2 plugin
	rm -rf $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/resources
	$(LINK) ../../../share/carla/resources $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/resources

ifeq ($(HAVE_PYQT),true)
	# Link styles for lv2 plugin
	rm -rf $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/styles
	$(LINK) ../../carla/styles $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/styles
endif

	# -------------------------------------------------------------------------------------------------------------

ifneq ($(HAIKU),true)
ifeq ($(HAVE_PYQT),true)
	# Install vst plugin
	install -d $(DESTDIR)$(LIBDIR)/vst/carla.vst

	install -m 644 \
		bin/CarlaRack*.* \
		bin/CarlaPatchbay*.* \
		$(DESTDIR)$(LIBDIR)/vst/carla.vst

	# Link binaries for vst plugin
	@for i in `find $(DESTDIR)$(LIBDIR)/carla/ -maxdepth 1 -type f -exec basename {} ';'`; do \
		$(LINK) ../../carla/$$i $(DESTDIR)$(LIBDIR)/vst/carla.vst/$$i; \
	done
	rm -f $(DESTDIR)$(LIBDIR)/vst/carla.vst/libcarla_standalone2.*

	# Link jack app bridge
	rm -rf $(DESTDIR)$(LIBDIR)/vst/carla.vst/jack
	$(LINK) ../../carla/jack $(DESTDIR)$(LIBDIR)/vst/carla.vst/jack

	# Link resources for vst plugin
	rm -rf $(DESTDIR)$(LIBDIR)/vst/carla.vst/resources
	$(LINK) ../../../share/carla/resources $(DESTDIR)$(LIBDIR)/vst/carla.vst/resources

	# Link styles for vst plugin
	rm -rf $(DESTDIR)$(LIBDIR)/vst/carla.vst/styles
	$(LINK) ../../carla/styles $(DESTDIR)$(LIBDIR)/vst/carla.vst/styles
endif
endif

ifneq ($(EXTERNAL_PLUGINS),true)
install_external_plugins:
endif

install: install_main install_external_plugins

# ---------------------------------------------------------------------------------------------------------------------

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/carla*
	rm -f $(DESTDIR)$(LIBDIR)/pkgconfig/carla-native-plugin.pc
	rm -f $(DESTDIR)$(LIBDIR)/pkgconfig/carla-standalone.pc
	rm -f $(DESTDIR)$(LIBDIR)/pkgconfig/carla-utils.pc
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
	@printf -- "Front-End:     $(ANS_YES) (Using $(FEV))\n"
ifneq ($(WIN32),true)
	@printf -- "LV2 plugin:    $(ANS_YES)\n"
else
	@printf -- "LV2 plugin:    $(ANS_NO)  $(mZ)Not available for Windows$(mE)\n"
endif
ifeq ($(LINUX),true)
ifeq ($(HAVE_X11),true)
	@printf -- "VST plugin:    $(ANS_YES)\n"
else # HAVE_X11
	@printf -- "VST plugin:    $(ANS_NO)  $(mS)X11 missing$(mE)\n"
endif
else # LINUX
	@printf -- "VST plugin:    $(ANS_NO)  $(mZ)Linux only$(mE)\n"
endif
else
	@printf -- "Front-End:     $(ANS_NO)  $(mS)Missing PyQt$(mE)\n"
	@printf -- "LV2 plugin:    $(ANS_NO)  $(mS)No front-end$(mE)\n"
	@printf -- "VST plugin:    $(ANS_NO)  $(mS)No front-end$(mE)\n"
endif
ifeq ($(HAVE_HYLIA),true)
	@printf -- "Link support:  $(ANS_YES)\n"
else
ifeq ($(MACOS_OLD),true)
	@printf -- "Link support:  $(ANS_NO)  $(mZ)MacOS >= 10.10 only$(mE)\n"
else
	@printf -- "Link support:  $(ANS_NO)  $(mZ)Linux, MacOS and Windows only$(mE)\n"
endif
endif
ifeq ($(HAVE_LIBLO),true)
	@printf -- "OSC support:   $(ANS_YES)\n"
else
	@printf -- "OSC support:   $(ANS_NO)  $(mS)Missing liblo$(mE)\n"
endif
ifeq ($(WIN32),true)
	@printf -- "Binary detect: $(ANS_YES)\n"
else
ifeq ($(HAVE_LIBMAGIC),true)
	@printf -- "Binary detect: $(ANS_YES)\n"
else
	@printf -- "Binary detect: $(ANS_NO)  $(mS)Missing libmagic/file$(mE)\n"
endif
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
ifeq ($(WIN32),true)
	@printf -- "ASIO:        $(ANS_YES)\n"
	@printf -- "DirectSound: $(ANS_YES)\n"
	@printf -- "WASAPI:      $(ANS_YES)\n"
else
	@printf -- "ASIO:        $(ANS_NO)  $(mZ)Windows only$(mE)\n"
	@printf -- "DirectSound: $(ANS_NO)  $(mZ)Windows only$(mE)\n"
	@printf -- "WASAPI:      $(ANS_NO)  $(mZ)Windows only$(mE)\n"
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
ifeq ($(HAVE_SNDFILE),true)
	@printf -- "Basic: $(ANS_YES)\n"
else
	@printf -- "Basic: $(ANS_NO) $(mS)libsndfile missing$(mE)\n"
endif
ifeq ($(HAVE_FFMPEG),true)
	@printf -- "Extra: $(ANS_YES)\n"
else
	@printf -- "Extra: $(ANS_NO) $(mS)FFmpeg missing or too new$(mE)\n"
endif
ifeq ($(HAVE_FLUIDSYNTH),true)
	@printf -- "SF2/3: $(ANS_YES)\n"
else
	@printf -- "SF2/3: $(ANS_NO) $(mS)FluidSynth missing or too old$(mE)\n"
endif
	@printf -- "SFZ:   $(ANS_YES)\n"
	@printf -- "\n"

	@printf -- "$(tS)---> Internal plugins: $(tE)\n"
	@printf -- "Basic Plugins:    $(ANS_YES)\n"
ifneq ($(WIN32),true)
	@printf -- "Carla-Patchbay:   $(ANS_YES)\n"
	@printf -- "Carla-Rack:       $(ANS_YES)\n"
else
	@printf -- "Carla-Patchbay:   $(ANS_NO) $(mS)Not available for Windows$(mE)\n"
	@printf -- "Carla-Rack:       $(ANS_NO) $(mS)Not available for Windows$(mE)\n"
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
