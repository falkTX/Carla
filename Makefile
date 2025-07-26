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

# see also cmake/CMakeLists.txt, source/common/__init__.py and source/includes/CarlaDefines.h
VERSION   := 2.6.0-alpha1

-include Makefile.user.mk

# ---------------------------------------------------------------------------------------------------------------------

TARGETS = backend frontend theme

ifneq ($(WASM),true)
TARGETS += bridges-plugin bridges-ui discovery interposer libjack plugin
endif

all: $(TARGETS)

# ---------------------------------------------------------------------------------------------------------------------
# Binaries (native)

ifneq ($(STATIC_PLUGIN_TARGET),true)
ALL_LIBS += $(MODULEDIR)/carla_engine.a
endif
ALL_LIBS += $(MODULEDIR)/carla_engine_plugin.a
ALL_LIBS += $(MODULEDIR)/carla_plugin.a
ALL_LIBS += $(MODULEDIR)/jackbridge.a
ifeq ($(STATIC_PLUGIN_TARGET),true)
ALL_LIBS += $(MODULEDIR)/jackbridge.min.a
endif
ALL_LIBS += $(MODULEDIR)/native-plugins.a
ALL_LIBS += $(MODULEDIR)/rtmempool.a

3RD_LIBS += $(MODULEDIR)/audio_decoder.a
3RD_LIBS += $(MODULEDIR)/lilv.a
ifneq ($(STATIC_PLUGIN_TARGET),true)
3RD_LIBS += $(MODULEDIR)/sfzero.a
endif
3RD_LIBS += $(MODULEDIR)/water.a
3RD_LIBS += $(MODULEDIR)/zita-resampler.a

ifeq ($(HAVE_DGL),true)
ifneq ($(USING_CUSTOM_DPF),true)
3RD_LIBS += $(MODULEDIR)/dgl.a
endif
endif

ifeq ($(HAVE_HYLIA),true)
3RD_LIBS += $(MODULEDIR)/hylia.a
endif

ifeq ($(HAVE_QT4),true)
3RD_LIBS += $(MODULEDIR)/theme.qt4.a
endif

ifeq ($(HAVE_QT5),true)
3RD_LIBS += $(MODULEDIR)/theme.qt5.a
endif

ifeq ($(HAVE_QT6),true)
3RD_LIBS += $(MODULEDIR)/theme.qt6.a
endif

ifeq ($(HAVE_YSFX),true)
3RD_LIBS += $(MODULEDIR)/ysfx.a
endif

ifeq ($(USING_RTAUDIO),true)
3RD_LIBS += $(MODULEDIR)/rtaudio.a
3RD_LIBS += $(MODULEDIR)/rtmidi.a
endif

ALL_LIBS += $(3RD_LIBS)

3rd: $(3RD_LIBS)
	@$(MAKE) -C source/theme
ifeq ($(HAVE_QT4),true)
	@$(MAKE) -C source/bridges-ui ui_lv2-qt4
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

$(MODULEDIR)/theme.qt6.a: .FORCE
	@$(MAKE) -C source/theme qt6

$(MODULEDIR)/%.arm32.a: .FORCE
ifneq ($(WINDOWS),true)
	@$(MAKE) -C source/modules/$* arm32
else
	$(error Trying to build ARM binaries with a Windows toolchain, this cannot work)
endif

$(MODULEDIR)/%.posix32.a: .FORCE
ifneq ($(WINDOWS),true)
	@$(MAKE) -C source/modules/$* posix32
else
	$(error Trying to build POSIX binaries with a Windows toolchain, this cannot work)
endif

$(MODULEDIR)/%.posix64.a: .FORCE
ifneq ($(WINDOWS),true)
	@$(MAKE) -C source/modules/$* posix64
else
	$(error Trying to build POSIX binaries with a Windows toolchain, this cannot work)
endif

$(MODULEDIR)/%.win32.a: .FORCE
ifeq ($(WINDOWS),true)
	@$(MAKE) -C source/modules/$* win32
else
	$(error Trying to build Windows binaries with a regular toolchain, this cannot work)
endif

$(MODULEDIR)/%.win64.a: .FORCE
ifeq ($(WINDOWS),true)
	@$(MAKE) -C source/modules/$* win64
else
	$(error Trying to build Windows binaries with a regular toolchain, this cannot work)
endif

$(MODULEDIR)/dgl.wine.a: .FORCE
	@$(MAKE) -C source/modules/dgl wine

$(MODULEDIR)/water.files.a: .FORCE
	@$(MAKE) -C source/modules/water files

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

frontend: backend
ifeq ($(HAVE_FRONTEND),true)
	@$(MAKE) -C source/frontend
endif

interposer:
ifeq ($(LINUX),true)
	@$(MAKE) -C source/interposer
endif

libjack: libs
	@$(MAKE) -C source/libjack

lv2-bundles-dep: $(MODULEDIR)/audio_decoder.a $(MODULEDIR)/water.a $(MODULEDIR)/zita-resampler.a
	@$(MAKE) -C source/native-plugins bundles

lv2-bundles: lv2-bundles-dep
	@$(MAKE) -C source/plugin bundles

plugin: backend bridges-plugin bridges-ui discovery
	@$(MAKE) -C source/plugin

ifeq ($(WINDOWS),true)
plugin-wine:
	@$(MAKE) -C source/plugin wine
else
plugin-wine: $(MODULEDIR)/dgl.wine.a
	@$(MAKE) -C source/plugin wine
ifeq ($(CPU_X86_64),true)
	@$(MAKE) plugin-wine AR=x86_64-w64-mingw32-ar CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++
else ifeq ($(CPU_I386),true)
	@$(MAKE) plugin-wine AR=i686-w64-mingw32-ar CC=i686-w64-mingw32-gcc CXX=i686-w64-mingw32-g++
endif
endif

rest: libs
	@$(MAKE) -C source/rest

theme: libs
	@$(MAKE) -C source/theme

# ---------------------------------------------------------------------------------------------------------------------
# static targets

static-backend: libs
	@$(MAKE) -C source/backend static

static-plugin: static-backend
	@$(MAKE) -C source/plugin static

# ---------------------------------------------------------------------------------------------------------------------
# hacks

msys2fix:
	rm -rf source/includes/serd
	rm -rf source/includes/sord
	rm -rf source/includes/sratom
	rm -rf source/includes/lilv
	cp -r source/modules/lilv/serd-0.24.0/serd source/includes/serd
	cp -r source/modules/lilv/sord-0.16.0/sord source/includes/sord
	cp -r source/modules/lilv/sratom-0.6.0/sratom source/includes/sratom
	cp -r source/modules/lilv/lilv-0.24.0/lilv source/includes/lilv

# ---------------------------------------------------------------------------------------------------------------------
# Binaries (arm32)

LIBS_ARM32  = $(MODULEDIR)/jackbridge.arm32.a
LIBS_ARM32 += $(MODULEDIR)/lilv.arm32.a
LIBS_ARM32 += $(MODULEDIR)/rtmempool.arm32.a
LIBS_ARM32 += $(MODULEDIR)/water.arm32.a

arm32: $(LIBS_ARM32)
	$(MAKE) -C source/bridges-plugin arm32
	$(MAKE) -C source/discovery arm32

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

LIBS_WIN32 += $(MODULEDIR)/lilv.win32.a
LIBS_WIN32 += $(MODULEDIR)/rtmempool.win32.a
LIBS_WIN32 += $(MODULEDIR)/water.win32.a

LIBS_WINE32 = $(LIBS_WIN32) $(MODULEDIR)/jackbridge.win32e.a
LIBS_RWIN32 = $(LIBS_WIN32) $(MODULEDIR)/jackbridge.win32.a

ifeq ($(CC),x86_64-w64-mingw32-gcc)
win32:
	$(MAKE) AR=i686-w64-mingw32-ar CC=i686-w64-mingw32-gcc CXX=i686-w64-mingw32-g++ win32-i686

win32r:
	$(MAKE) AR=i686-w64-mingw32-ar CC=i686-w64-mingw32-gcc CXX=i686-w64-mingw32-g++ win32r-i686
else
win32: win32-i686
win32r: win32r-i686
endif

win32-i686: $(LIBS_WINE32)
	$(MAKE) BUILDING_FOR_WINE=true -C source/bridges-plugin win32
	$(MAKE) BUILDING_FOR_WINE=true -C source/discovery win32

win32r-i686: $(LIBS_RWIN32)
	$(MAKE) -C source/bridges-plugin win32
	$(MAKE) -C source/discovery win32

# ---------------------------------------------------------------------------------------------------------------------
# Binaries (win64)

LIBS_WIN64 += $(MODULEDIR)/lilv.win64.a
LIBS_WIN64 += $(MODULEDIR)/rtmempool.win64.a
LIBS_WIN64 += $(MODULEDIR)/water.win64.a

LIBS_WINE64 = $(LIBS_WIN64) $(MODULEDIR)/jackbridge.win64e.a
LIBS_RWIN64 = $(LIBS_WIN64) $(MODULEDIR)/jackbridge.win64.a

win64: $(LIBS_WINE64)
	$(MAKE) BUILDING_FOR_WINE=true -C source/bridges-plugin win64
	$(MAKE) BUILDING_FOR_WINE=true -C source/discovery win64

win64r: $(LIBS_RWIN64)
ifeq ($(CC),i686-w64-mingw32-gcc)
	$(MAKE) CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++ -C source/bridges-plugin win64
	$(MAKE) CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++ -C source/discovery win64
else
	$(MAKE) -C source/bridges-plugin win64
	$(MAKE) -C source/discovery win64
endif

mingw64:
	$(MAKE) AR=i686-w64-mingw32-ar CC=i686-w64-mingw32-gcc CXX=i686-w64-mingw32-g++ win32
	$(MAKE) AR=x86_64-w64-mingw32-ar CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++ win64

# ---------------------------------------------------------------------------------------------------------------------
# Binaries (wine)

wine32:
	$(MAKE) -C source/jackbridge wine32
	cp -f $(MODULEDIR)/jackbridge-wine32.dll$(LIB_EXT) $(CURDIR)/bin/jackbridge-wine32.dll

wine64:
	$(MAKE) -C source/jackbridge wine64
	cp -f $(MODULEDIR)/jackbridge-wine64.dll$(LIB_EXT) $(CURDIR)/bin/jackbridge-wine64.dll

# ---------------------------------------------------------------------------------------------------------------------

clean:
	$(MAKE) clean -C source/backend
	$(MAKE) clean -C source/bridges-plugin
	$(MAKE) clean -C source/bridges-ui
	$(MAKE) clean -C source/jackbridge
	$(MAKE) clean -C source/modules
	$(MAKE) clean -C source/native-plugins
	$(MAKE) clean -C source/plugin
ifneq ($(STATIC_PLUGIN_TARGET),true)
	$(MAKE) clean -C source/discovery
	$(MAKE) clean -C source/frontend
	$(MAKE) clean -C source/interposer
	$(MAKE) clean -C source/libjack
	$(MAKE) clean -C source/tests
	$(MAKE) clean -C source/theme
endif
	rm -f *~ source/*~

distclean: clean
	rm -f bin/*.exe bin/*.dll bin/*.dylib bin/*.so
	rm -rf build build-lv2

cpp:
	$(MAKE) CPPMODE=true

debug:
	$(MAKE) DEBUG=true

doxygen:
	$(MAKE) doxygen -C source/backend

tests:
	$(MAKE) -C source/tests

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
	install -d $(DESTDIR)$(DATADIR)/carla/resources
ifeq ($(LINUX),true)
	install -d $(DESTDIR)$(LIBDIR)/carla/jack
else
	install -d $(DESTDIR)$(LIBDIR)/carla
endif
	install -d $(DESTDIR)$(LIBDIR)/pkgconfig
	install -d $(DESTDIR)$(INCLUDEDIR)/carla/includes
	install -d $(DESTDIR)$(INCLUDEDIR)/carla/utils
ifeq ($(LINUX),true)
ifeq ($(HAVE_JACK),true)
ifeq ($(JACKBRIDGE_DIRECT),true)
	install -d $(DESTDIR)$(JACK_LIBDIR)
endif
endif
endif

ifeq ($(HAVE_FRONTEND),true)
	# Create directories (frontend)
	install -d $(DESTDIR)$(LIBDIR)/carla/styles
	install -d $(DESTDIR)$(PREFIX)/share/appdata
	install -d $(DESTDIR)$(DATADIR)/applications
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/16x16/apps
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/32x32/apps
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/48x48/apps
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/128x128/apps
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/256x256/apps
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/scalable/apps
	install -d $(DESTDIR)$(DATADIR)/mime/packages
	install -d $(DESTDIR)$(DATADIR)/carla/resources/translations
	install -d $(DESTDIR)$(DATADIR)/carla/common
	install -d $(DESTDIR)$(DATADIR)/carla/patchcanvas
	install -d $(DESTDIR)$(DATADIR)/carla/utils
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
ifeq ($(HAVE_JACK),true)
ifeq ($(JACKBRIDGE_DIRECT),true)
	# Install internal jack client
	ln -sf \
		$(LIBDIR)/carla/libcarla_standalone2.so \
		$(DESTDIR)$(JACK_LIBDIR)/carla.so
endif
endif

ifneq ($(JACKBRIDGE_DIRECT),true)
	# Install custom libjack
	install -m 755 \
		bin/jack/libjack.so.0 \
		$(DESTDIR)$(LIBDIR)/carla/jack
endif
endif

	# Install pkg-config files
	install -m 644 \
		data/pkgconfig/*.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig

	# Adjust PREFIX, LIBDIR, INCLUDEDIR and VERSION in pkg-config files
	sed $(SED_ARGS) 's?X-PREFIX-X?$(PREFIX)?' \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-host-plugin.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-native-plugin.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-standalone.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-utils.pc

	sed $(SED_ARGS) 's?X-LIBDIR-X?$(LIBDIR)?' \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-host-plugin.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-native-plugin.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-standalone.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-utils.pc

	sed $(SED_ARGS) 's?X-INCLUDEDIR-X?$(INCLUDEDIR)?' \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-host-plugin.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-native-plugin.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-standalone.pc \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-utils.pc

	sed $(SED_ARGS) 's?X-VERSION-X?$(VERSION)?' \
		$(DESTDIR)$(LIBDIR)/pkgconfig/carla-host-plugin.pc \
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
		$(DESTDIR)$(INCLUDEDIR)/carla

	install -m 644 \
		source/includes/CarlaDefines.h \
		source/includes/CarlaMIDI.h \
		source/includes/CarlaNative.h \
		source/includes/CarlaNativePlugin.h \
		$(DESTDIR)$(INCLUDEDIR)/carla/includes

	install -m 644 \
		source/utils/CarlaUtils.hpp \
		$(DESTDIR)$(INCLUDEDIR)/carla/utils

	# -------------------------------------------------------------------------------------------------------------

ifeq ($(HAVE_FRONTEND),true)
ifneq ($(CPPMODE),true)
	# Install script files (gui)
	install -m 755 \
		data/carla \
		data/carla-database \
		data/carla-jack-multi \
		data/carla-jack-single \
		data/carla-jack-patchbayplugin \
		data/carla-osc-gui \
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
		$(DESTDIR)$(BINDIR)/carla-jack-patchbayplugin \
		$(DESTDIR)$(BINDIR)/carla-osc-gui \
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
		source/frontend/common/*.py \
		$(DESTDIR)$(DATADIR)/carla/common/

	install -m 644 \
		source/frontend/patchcanvas/*.py \
		$(DESTDIR)$(DATADIR)/carla/patchcanvas/

	install -m 644 \
		source/frontend/utils/*.py \
		$(DESTDIR)$(DATADIR)/carla/utils/

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
endif # CPPMODE

ifeq ($(HAVE_THEME),true)
	# Install theme
	install -m 644 \
		bin/styles/* \
		$(DESTDIR)$(LIBDIR)/carla/styles
endif

	# Install desktop files
	install -m 644 data/desktop/carla.desktop             $(DESTDIR)$(DATADIR)/applications
	install -m 644 data/desktop/carla-rack.desktop        $(DESTDIR)$(DATADIR)/applications
	install -m 644 data/desktop/carla-patchbay.desktop    $(DESTDIR)$(DATADIR)/applications
	install -m 644 data/desktop/carla-jack-single.desktop $(DESTDIR)$(DATADIR)/applications
	install -m 644 data/desktop/carla-jack-multi.desktop  $(DESTDIR)$(DATADIR)/applications
ifeq ($(HAVE_LIBLO),true)
	install -m 644 data/desktop/carla-control.desktop     $(DESTDIR)$(DATADIR)/applications
endif

	# Install app data
	sed -e 's?X-VERSION-X?$(VERSION)?' \
		data/appdata.xml.in > $(DESTDIR)$(PREFIX)/share/appdata/studio.kx.carla.appdata.xml

	# Install mime package
	install -m 644 data/carla.xml $(DESTDIR)$(DATADIR)/mime/packages

	# Install icons, 16x16
	install -m 644 resources/16x16/carla.png            $(DESTDIR)$(DATADIR)/icons/hicolor/16x16/apps
	install -m 644 resources/16x16/carla-control.png    $(DESTDIR)$(DATADIR)/icons/hicolor/16x16/apps

	# Install icons, 32x32
	install -m 644 resources/32x32/carla.png            $(DESTDIR)$(DATADIR)/icons/hicolor/32x32/apps
	install -m 644 resources/32x32/carla-control.png    $(DESTDIR)$(DATADIR)/icons/hicolor/32x32/apps

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
	$(LINK) ../common                      $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../patchcanvas                 $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../utils                       $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../widgets                     $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_app.py                $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_backend.py            $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_backend_qt.py         $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_frontend.py           $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_host.py               $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_host_control.py       $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_settings.py           $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_skin.py               $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_shared.py             $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_utils.py              $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../carla_widgets.py            $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../externalui.py               $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../qt_compat.py                $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../qt_config.py                $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../resources_rc.py             $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_about.py           $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_edit.py            $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_host.py            $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_parameter.py       $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_plugin_calf.py     $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_plugin_classic.py  $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_plugin_compact.py  $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_plugin_default.py  $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_plugin_presets.py  $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_settings.py        $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_carla_settings_driver.py $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_inputdialog_value.py     $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_midipattern.py           $(DESTDIR)$(DATADIR)/carla/resources
	$(LINK) ../ui_xycontroller.py          $(DESTDIR)$(DATADIR)/carla/resources

	# Install translations
	$(foreach l,$(I18N_LANGUAGES),install -m 644 \
		source/frontend/translations/carla_$(l).qm \
		$(DESTDIR)$(DATADIR)/carla/resources/translations/;)

	# Link resources for internal plugin
	rm -rf $(DESTDIR)$(LIBDIR)/carla/resources
	$(LINK) ../../share/carla/resources $(DESTDIR)$(LIBDIR)/carla/resources
endif # HAVE_FRONTEND

	# -------------------------------------------------------------------------------------------------------------

ifeq ($(CAN_GENERATE_LV2_TTL),true)
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
endif # CAN_GENERATE_LV2_TTL

ifeq ($(HAVE_FRONTEND),true)
	# Link resources for lv2 plugin
	rm -rf $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/resources
	$(LINK) ../../../share/carla/resources $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/resources

	# Link styles for lv2 plugin
	rm -rf $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/styles
	$(LINK) ../../carla/styles $(DESTDIR)$(LIBDIR)/lv2/carla.lv2/styles
endif

	# -------------------------------------------------------------------------------------------------------------

ifneq ($(HAIKU),true)
ifeq ($(HAVE_FRONTEND),true)
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

# ---------------------------------------------------------------------------------------------------------------------

ifneq ($(EXTERNAL_PLUGINS),true)
install_external_plugins:
endif

install: install_main install_external_plugins

# ---------------------------------------------------------------------------------------------------------------------

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/carla*
	rm -f $(DESTDIR)$(LIBDIR)/pkgconfig/carla-host-plugin.pc
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

ifeq ($(MACOS),true)
ifneq ($(MACOS_OLD),true)
HAVE_DIST = true
endif
endif

ifeq ($(WINDOWS),true)
HAVE_DIST = true
endif

ifeq ($(HAVE_DIST),true)
include Makefile.dist.mk
else
dist:
endif

include Makefile.print.mk

# ---------------------------------------------------------------------------------------------------------------------

.FORCE:
.PHONY: .FORCE

# ---------------------------------------------------------------------------------------------------------------------
