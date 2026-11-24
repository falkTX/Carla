#!/usr/bin/make -f
# Makefile for Franca #
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
TARGETS += bridges-plugin bridges-ui discovery interposer libjack
endif

all: $(TARGETS)

# ---------------------------------------------------------------------------------------------------------------------
# Binaries (native)

ifneq ($(STATIC_PLUGIN_TARGET),true)
ALL_LIBS += $(MODULEDIR)/carla_engine.a
else
ALL_LIBS += $(MODULEDIR)/carla_engine_plugin.a
endif
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
	@$(MAKE) -C source/backend/engine standalone

$(MODULEDIR)/carla_engine_plugin.a: $(MODULEDIR)/carla_engine.a
	@$(MAKE) -C source/backend/engine plugin

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

plugin: backend bridges-plugin bridges-ui discovery $(MODULEDIR)/carla_engine_plugin.a
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

static-plugin: static-backend $(MODULEDIR)/carla_engine_plugin.a
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

.PHONY: check-franca rust-ui check-rust-ui
check-franca: backend discovery
	PYTHONPATH=source/frontend python3 source/tests/franca-smoke.py

rust-ui:
	cargo build --release --manifest-path source/frontend-rs/Cargo.toml
	install -m 755 source/frontend-rs/target/release/franca-ui bin/franca-ui

check-rust-ui: backend discovery
	cargo test --manifest-path source/frontend-rs/Cargo.toml

stoat:
	stoat --recursive ./build/ --suppression ./data/stoat-supression.txt --whitelist ./data/stoat-whitelist.txt --graph-view ./data/stoat-callgraph.png

# 	stoat --recursive ./build/ \
# 	--suppression ./data/stoat-supression.txt \
# 	--whitelist   ./data/stoat-whitelist.txt  \
# 	--graph-view  ./data/stoat-callgraph.png

# ---------------------------------------------------------------------------------------------------------------------

FRANCA_LIBDIR = $(LIBDIR)/franca
FRANCA_DATADIR = $(DATADIR)/franca
FRANCA_PYTHON = source/frontend
FRANCA_HELPERS = $(wildcard bin/carla-bridge-* bin/carla-discovery-* bin/jackbridge-wine*.dll bin/libcarla_interposer*)
FRANCA_UI_HELPERS = $(wildcard bin/resources/*-ui bin/resources/zynaddsubfx) \
	bin/resources/carla-plugin bin/resources/carla-plugin-patchbay

install_main:
ifeq ($(HAVE_FRONTEND),true)
	install -d "$(DESTDIR)$(BINDIR)" "$(DESTDIR)$(FRANCA_LIBDIR)" "$(DESTDIR)$(FRANCA_DATADIR)/resources/translations"
	install -d "$(DESTDIR)$(DATADIR)/applications" "$(DESTDIR)$(DATADIR)/mime/packages" \
		"$(DESTDIR)$(DATADIR)/icons/hicolor/256x256/apps"
	install -m 755 data/franca "$(DESTDIR)$(BINDIR)/franca"
	sed $(SED_ARGS) 's?X-LIBDIR-X?$(LIBDIR)?' -e 's?X-DATADIR-X?$(DATADIR)?' "$(DESTDIR)$(BINDIR)/franca"
	install -m 644 data/desktop/franca.desktop "$(DESTDIR)$(DATADIR)/applications/"
	install -m 644 data/carla.xml "$(DESTDIR)$(DATADIR)/mime/packages/franca.xml"
	install -m 644 resources/256x256/franca.png "$(DESTDIR)$(DATADIR)/icons/hicolor/256x256/apps/franca.png"

	install -m 644 bin/libcarla_standalone2$(LIB_EXT) bin/libcarla_utils$(LIB_EXT) \
		bin/libcarla_frontend$(LIB_EXT) "$(DESTDIR)$(FRANCA_LIBDIR)/"
	install -m 755 $(FRANCA_HELPERS) "$(DESTDIR)$(FRANCA_LIBDIR)/"
ifneq ($(wildcard bin/franca-ui),)
	install -m 755 bin/franca-ui "$(DESTDIR)$(FRANCA_LIBDIR)/franca-ui"
endif
ifneq ($(wildcard bin/jack/libjack.so.0),)
	install -Dm755 bin/jack/libjack.so.0 "$(DESTDIR)$(FRANCA_LIBDIR)/jack/libjack.so.0"
endif
ifneq ($(wildcard bin/styles/*),)
	install -d "$(DESTDIR)$(FRANCA_LIBDIR)/styles"
	install -m 644 bin/styles/* "$(DESTDIR)$(FRANCA_LIBDIR)/styles/"
endif

	install -m 644 $(FRANCA_PYTHON)/franca $(FRANCA_PYTHON)/*.py "$(DESTDIR)$(FRANCA_DATADIR)/"
	@for module in common patchcanvas utils widgets; do \
		install -d "$(DESTDIR)$(FRANCA_DATADIR)/$$module" && \
		install -m 644 "$(FRANCA_PYTHON)/$$module/"*.py "$(DESTDIR)$(FRANCA_DATADIR)/$$module/" || exit 1; \
	done
	sed $(SED_ARGS) 's?X_LIBDIR_X = None?X_LIBDIR_X = "$(LIBDIR)"?' \
		-e 's?X_DATADIR_X = None?X_DATADIR_X = "$(DATADIR)"?' "$(DESTDIR)$(FRANCA_DATADIR)/carla_shared.py"
	install -m 755 $(FRANCA_UI_HELPERS) "$(DESTDIR)$(FRANCA_DATADIR)/resources/"
	@for resource in "$(DESTDIR)$(FRANCA_DATADIR)/"*.py common patchcanvas utils widgets; do \
		ln -sfn "../$$(basename "$$resource")" "$(DESTDIR)$(FRANCA_DATADIR)/resources/" || exit 1; \
	done
	$(foreach l,$(I18N_LANGUAGES),install -m 644 $(FRANCA_PYTHON)/translations/carla_$(l).qm \
		"$(DESTDIR)$(FRANCA_DATADIR)/resources/translations/";)
	ln -sfn "$$(realpath -m --relative-to="$(FRANCA_LIBDIR)" "$(FRANCA_DATADIR)/resources")" \
		"$(DESTDIR)$(FRANCA_LIBDIR)/resources"
else
	@echo "Installing Franca requires Qt and PyQt."
	@false
endif

install: install_main

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/franca" "$(DESTDIR)$(DATADIR)/applications/franca.desktop" \
		"$(DESTDIR)$(DATADIR)/mime/packages/franca.xml" "$(DESTDIR)$(DATADIR)/icons/hicolor/256x256/apps/franca.png"
	rm -rf "$(DESTDIR)$(FRANCA_LIBDIR)" "$(DESTDIR)$(FRANCA_DATADIR)"

# ---------------------------------------------------------------------------------------------------------------------

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
