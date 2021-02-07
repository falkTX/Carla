#!/usr/bin/make -f
# Makefile for Carla #
# ------------------ #
# Created by falkTX
#

# NOTE to be imported from main Makefile

# ----------------------------------------------------------------------------------------------------------------------------

PYTHON = $(EXE_WRAPPER) $(shell which python3$(APP_EXT))

ifeq ($(WIN32),true)
QT5_DLL_EXT = .dll
QT5_DLL_V = 5
endif

ifeq ($(MACOS),true)
QT5_LIB_PREFIX = lib
endif

QT5_PREFIX = $(shell pkg-config --variable=prefix Qt5OpenGLExtensions)

# ----------------------------------------------------------------------------------------------------------------------------

_PLUGIN_UIS = \
	carla-plugin \
	bigmeter-ui \
	midipattern-ui \
	notes-ui \
	xycontroller-ui

_QT5_DLLS = \
	Qt$(QT5_DLL_V)Core$(QT5_DLL_EXT) \
	Qt$(QT5_DLL_V)Gui$(QT5_DLL_EXT) \
	Qt$(QT5_DLL_V)OpenGL$(QT5_DLL_EXT) \
	Qt$(QT5_DLL_V)PrintSupport$(QT5_DLL_EXT) \
	Qt$(QT5_DLL_V)Svg$(QT5_DLL_EXT) \
	Qt$(QT5_DLL_V)Widgets$(QT5_DLL_EXT)

_QT5_PLUGINS = \
	iconengines/$(QT5_LIB_PREFIX)qsvgicon$(LIB_EXT) \
	imageformats/$(QT5_LIB_PREFIX)qjpeg$(LIB_EXT) \
	imageformats/$(QT5_LIB_PREFIX)qsvg$(LIB_EXT)

ifeq ($(MACOS),true)
_QT5_PLUGINS += \
	platforms/$(QT5_LIB_PREFIX)qcocoa$(LIB_EXT)
else ifeq ($(WIN32),true)
_QT5_PLUGINS += \
	platforms/$(QT5_LIB_PREFIX)qwindows$(LIB_EXT)
endif

# NOTE this has to be hardcoded for now. oh well
ifeq ($(WIN32),true)
_PYTHON_FILES = \
	libpython3.8.dll
endif

_THEME_FILES = \
	styles/carlastyle.json \
	styles/carlastyle$(LIB_EXT)

_CARLA_HOST_FILES = \
	carla-bridge-lv2$(LIB_EXT) \
	carla-bridge-native$(APP_EXT) \
	carla-discovery-native$(APP_EXT) \
	libcarla_utils$(LIB_EXT)

ifeq ($(MACOS),true)
_CARLA_HOST_FILES += \
	$(_PLUGIN_UIS:%=resources/%$(APP_EXT)) \
	carla-bridge-lv2-cocoa$(APP_EXT)
else ifeq ($(WIN32),true)
# TODO plugin UIs on windows
_CARLA_HOST_FILES += \
	carla-bridge-lv2-windows$(APP_EXT)
endif

_CARLA_APP_FILES = \
	Carla$(APP_EXT) \
	libcarla_standalone2$(LIB_EXT) \
	$(_CARLA_HOST_FILES) \
	$(_QT5_DLLS) \
	$(_QT5_PLUGINS) \
	$(_PYTHON_FILES) \
	$(_THEME_FILES)

_CARLA_CONTROL_APP_FILES = \
	Carla-Control$(APP_EXT) \
	libcarla_utils$(LIB_EXT) \
	$(_QT5_DLLS) \
	$(_QT5_PLUGINS) \
	$(_PYTHON_FILES) \
	$(_THEME_FILES)

_CARLA_LV2_PLUGIN_FILES = \
	carla.lv2/carla$(LIB_EXT) \
	carla.lv2/manifest.ttl \
	carla.lv2/resources/carla-plugin-patchbay \
	carla.lv2/resources/lib/library.zip \
	$(_CARLA_HOST_FILES:%=carla.lv2/%) \
	$(_QT5_DLLS:%=carla.lv2/resources/%) \
	$(_QT5_PLUGINS:%=carla.lv2/resources/%) \
	$(_THEME_FILES:%=carla.lv2/resources/%)

_CARLA_VST2FX_PLUGIN_FILES = \
	carlafx.vst/Contents/MacOS/CarlaVstFxShell \
	carlafx.vst/Contents/MacOS/resources/carla-plugin-patchbay \
	carlafx.vst/Contents/MacOS/resources/lib/library.zip \
	$(_CARLA_HOST_FILES:%=carlafx.vst/Contents/MacOS/%) \
	$(_QT5_DLLS:%=carlafx.vst/Contents/MacOS/resources/%) \
	$(_QT5_PLUGINS:%=carlafx.vst/Contents/MacOS/resources/%) \
	$(_THEME_FILES:%=carlafx.vst/Contents/MacOS/resources/%)

_CARLA_VST2SYN_PLUGIN_FILES = \
	carla.vst/Contents/MacOS/CarlaVstShell \
	carla.vst/Contents/MacOS/resources/carla-plugin-patchbay \
	carla.vst/Contents/MacOS/resources/lib/library.zip \
	$(_CARLA_HOST_FILES:%=carla.vst/Contents/MacOS/%) \
	$(_QT5_DLLS:%=carla.vst/Contents/MacOS/resources/%) \
	$(_QT5_PLUGINS:%=carla.vst/Contents/MacOS/resources/%) \
	$(_THEME_FILES:%=carla.vst/Contents/MacOS/resources/%)

ifeq ($(WIN32),true)
CARLA_APP_FILES = $(_CARLA_APP_FILES:%=build/Carla/%)
CARLA_APP_ZIPS = $(_PLUGIN_UIS:%=build/%/lib/library.zip)
CARLA_CONTROL_APP_FILES = $(_CARLA_CONTROL_APP_FILES:%=build/Carla-Control/%)
else
CARLA_APP_FILES = $(_CARLA_APP_FILES:%=build/Carla.app/Contents/MacOS/%)
CARLA_APP_ZIPS = $(_PLUGIN_UIS:%=build/%.app/Contents/MacOS/lib/library.zip)
CARLA_CONTROL_APP_FILES = $(_CARLA_CONTROL_APP_FILES:%=build/Carla-Control.app/Contents/MacOS/%)
endif

# ----------------------------------------------------------------------------------------------------------------------------
# entry point

ifeq ($(WIN64),true)
TARGETS = Carla-$(VERSION)-win64.zip
else ifeq ($(WIN32),true)
TARGETS = Carla-$(VERSION)-win32.zip
else
TARGETS = Carla-$(VERSION)-macOS.dmg
endif

dist: $(TARGETS)

# ----------------------------------------------------------------------------------------------------------------------------
# create final file

Carla-$(VERSION)-macOS.dmg: build/Carla.app/Contents/Info.plist build/Carla-Control.app/Contents/Info.plist build/Carla-Plugins.pkg
	rm -rf build/macos-pkg $@
	mkdir build/macos-pkg
	cp -r build/Carla.app build/Carla-Control.app build/Carla-Plugins.pkg data/macos/README build/macos-pkg/
	hdiutil create $@ -srcfolder build/macos-pkg -volname "Carla-$(VERSION)" -fs HFS+ -ov
	rm -rf build/macos-pkg

Carla-$(VERSION)-win64.zip: $(CARLA_APP_FILES)
	$(call CLEANUP_AND_PATCH_CXFREEZE_FILES,Carla)
# 	env PYTHONPATH=$(CURDIR)/source/frontend $(PYTHON) ./data/windows/app-gui.py build_exe

# ----------------------------------------------------------------------------------------------------------------------------
# macOS final cleanup, after everything is in place

define PATCH_QT_DEPENDENCIES
	install_name_tool -change "@rpath/QtCore.framework/Versions/5/QtCore"                 @executable_path/QtCore         ${1} && \
	install_name_tool -change "@rpath/QtGui.framework/Versions/5/QtGui"                   @executable_path/QtGui          ${1} && \
	install_name_tool -change "@rpath/QtOpenGL.framework/Versions/5/QtOpenGL"             @executable_path/QtOpenGL       ${1} && \
	install_name_tool -change "@rpath/QtPrintSupport.framework/Versions/5/QtPrintSupport" @executable_path/QtPrintSupport ${1} && \
	install_name_tool -change "@rpath/QtSvg.framework/Versions/5/QtSvg"                   @executable_path/QtSvg          ${1} && \
	install_name_tool -change "@rpath/QtWidgets.framework/Versions/5/QtWidgets"           @executable_path/QtWidgets      ${1} && \
	install_name_tool -change "@rpath/QtMacExtras.framework/Versions/5/QtMacExtras"       @executable_path/QtMacExtras    ${1}
endef

ifeq ($(MACOS),true)
define CLEANUP_AND_PATCH_CXFREEZE_FILES
	# cleanup
	find build/${1}.app/Contents/MacOS/ -type f -name "*.py" -delete
	find build/${1}.app/Contents/MacOS/ -type f -name "*.pyi" -delete
	find build/${1}.app/Contents/MacOS/ -type f -name "pylupdate.so" -delete
	find build/${1}.app/Contents/MacOS/ -type f -name "pyrcc.so" -delete
	find build/${1}.app/Contents/MacOS/ -type f -name "QtMacExtras*" -delete
	find build/${1}.app/Contents/MacOS/ -type f -name "QtNetwork*" -delete
	find build/${1}.app/Contents/MacOS/ -type f -name "QtSql*" -delete
	find build/${1}.app/Contents/MacOS/ -type f -name "QtTest*" -delete
	find build/${1}.app/Contents/MacOS/ -type f -name "QtXml*" -delete
	#find build/${1}.app/Contents/MacOS/ -type f -name "*.pyc" -delete
	rm -rf build/${1}.app/Contents/MacOS/lib/PyQt5/uic
	# adjust binaries
	(cd build/${1}.app/Contents/MacOS && \
		for f in `find . -type f | grep -e /Qt -e QOpenGL -e libq -e carlastyle.dylib -e sip.so`; do \
			$(call PATCH_QT_DEPENDENCIES,$$f); \
		done)
endef
else ifeq ($(WIN32),true)
define CLEANUP_AND_PATCH_CXFREEZE_FILES
	# cleanup
	find build/${1}/ -type f -name "*.py" -delete
	find build/${1}/ -type f -name "*.pyi" -delete
	find build/${1}/ -type f -name "pylupdate.so" -delete
	find build/${1}/ -type f -name "pyrcc.so" -delete
	find build/${1}/ -type f -name "QtMacExtras*" -delete
	find build/${1}/ -type f -name "QtNetwork*" -delete
	find build/${1}/ -type f -name "QtSql*" -delete
	find build/${1}/ -type f -name "QtTest*" -delete
	find build/${1}/ -type f -name "QtXml*" -delete
	#find build/${1}/ -type f -name "*.pyc" -delete
	rm -rf build/${1}/lib/PyQt5/uic
endef
endif

build/Carla.app/Contents/Info.plist: $(CARLA_APP_FILES)
	$(call CLEANUP_AND_PATCH_CXFREEZE_FILES,Carla)
	# extra step for standalone, symlink resources used in plugin UIs
	mkdir -p build/Carla.app/Contents/MacOS/resources
	(cd build/Carla.app/Contents/MacOS/resources && \
		ln -sf ../Qt* ../lib ../iconengines ../imageformats ../platforms ../styles . && \
		ln -sf carla-plugin$(APP_EXT) carla-plugin-patchbay$(APP_EXT))
	# mark as done
	touch $@

build/Carla-Control.app/Contents/Info.plist: $(CARLA_CONTROL_APP_FILES)
	$(call CLEANUP_AND_PATCH_CXFREEZE_FILES,Carla-Control)
	# mark as done
	touch $@

# ----------------------------------------------------------------------------------------------------------------------------
# application bundle, depends on cxfreeze library.zip

ifeq ($(WIN32),true)
define GENERATE_LIBRARY_ZIP
	# FIXME
	env PYTHONPATH=$(CURDIR)/source/frontend SCRIPT_NAME=${1} $(PYTHON) ./data/windows/app-gui.py build_exe
endef
else
define GENERATE_LIBRARY_ZIP
	env PYTHONPATH=$(CURDIR)/source/frontend SCRIPT_NAME=${1} $(PYTHON) ./data/macos/bundle.py bdist_mac --bundle-name=${1}
endef
endif

# ----------------------------------------------------------------------------------------------------------------------------

build/Carla.app/Contents/MacOS/Carla: build/Carla.app/Contents/MacOS/lib/library.zip

build/Carla.app/Contents/MacOS/lib/library.zip: $(CARLA_APP_ZIPS) data/macos/bundle.py data/macos/Carla.plist source/frontend/*
	$(call GENERATE_LIBRARY_ZIP,Carla)
	# merge all zips into 1
	rm -rf build/Carla.app/Contents/MacOS/lib/_lib
	mkdir build/Carla.app/Contents/MacOS/lib/_lib
	(cd build/Carla.app/Contents/MacOS/lib/_lib && \
		mv ../library.zip ../library-main.zip && \
		$(_PLUGIN_UIS:%=unzip -n $(CURDIR)/build/%.app/Contents/MacOS/lib/library.zip &&) \
		unzip -o ../library-main.zip && \
		zip -r -9 ../library.zip *)
	rm -rf build/Carla.app/Contents/MacOS/lib/_lib
	rm -rf build/Carla.app/Contents/MacOS/lib/library-main.zip

# ----------------------------------------------------------------------------------------------------------------------------

build/Carla/Carla.exe: build/Carla/lib/library.zip

build/Carla/lib/library.zip: data/windows/app-gui.py source/frontend/*
	$(call GENERATE_LIBRARY_ZIP,Carla)

# $(CARLA_APP_ZIPS)

# ----------------------------------------------------------------------------------------------------------------------------

build/Carla-Control.app/Contents/MacOS/Carla-Control: build/Carla-Control.app/Contents/MacOS/lib/library.zip

build/Carla-Control.app/Contents/MacOS/lib/library.zip: data/macos/bundle.py data/macos/Carla-Control.plist source/frontend/*
	$(call GENERATE_LIBRARY_ZIP,Carla-Control)

# ----------------------------------------------------------------------------------------------------------------------------
# macOS plugin UIs (stored in resources, depend on their respective startup script and generation of matching library.zip)

build/Carla.app/Contents/MacOS/resources/%: build/%.app/Contents/MacOS/lib/library.zip
	-@mkdir -p $(shell dirname $@)
	@cp -v build/$*.app/Contents/MacOS/$* $@

build/%.app/Contents/MacOS/lib/library.zip: data/macos/bundle.py source/frontend/%
	$(call GENERATE_LIBRARY_ZIP,$*)

# ----------------------------------------------------------------------------------------------------------------------------
# macOS generic bundle files (either Qt or Carla binaries)

build/Carla.app/Contents/MacOS/Qt% build/Carla-Control.app/Contents/MacOS/Qt%: $(QT5_PREFIX)/lib/Qt%.framework
	-@mkdir -p $(shell dirname $@)
	@cp -v $</Versions/5/Qt$* $@
	$(call PATCH_QT_DEPENDENCIES,$@)

build/Carla.app/Contents/MacOS/iconengines/% build/Carla-Control.app/Contents/MacOS/iconengines/%: $(QT5_PREFIX)/lib/qt5/plugins/iconengines/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@
	$(call PATCH_QT_DEPENDENCIES,$@)

build/Carla.app/Contents/MacOS/imageformats/% build/Carla-Control.app/Contents/MacOS/imageformats/%: $(QT5_PREFIX)/lib/qt5/plugins/imageformats/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@
	$(call PATCH_QT_DEPENDENCIES,$@)

build/Carla.app/Contents/MacOS/platforms/% build/Carla-Control.app/Contents/MacOS/platforms/%: $(QT5_PREFIX)/lib/qt5/plugins/platforms/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@
	$(call PATCH_QT_DEPENDENCIES,$@)

build/Carla.app/Contents/MacOS/styles/%.dylib build/Carla-Control.app/Contents/MacOS/styles/%.dylib: bin/styles/%.dylib
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@
	$(call PATCH_QT_DEPENDENCIES,$@)

build/Carla.app/Contents/MacOS/% build/Carla-Control.app/Contents/MacOS/%: bin/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

# ----------------------------------------------------------------------------------------------------------------------------
# win32 generic bundle files (either Qt or Carla binaries)

build/Carla/libpython3% build/Carla-Control/libpython3%: $(QT5_PREFIX)/bin/libpython3%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/Carla/Qt5% build/Carla-Control/Qt5%: $(QT5_PREFIX)/bin/Qt5%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/Carla/iconengines/% build/Carla-Control/iconengines/%: $(QT5_PREFIX)/lib/qt5/plugins/iconengines/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/Carla/imageformats/% build/Carla-Control/imageformats/%: $(QT5_PREFIX)/lib/qt5/plugins/imageformats/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/Carla/platforms/% build/Carla-Control/platforms/%: $(QT5_PREFIX)/lib/qt5/plugins/platforms/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/Carla/% build/Carla-Control/%: bin/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

# ----------------------------------------------------------------------------------------------------------------------------
# Plugin rules

MACOS_PACKAGE_EXP  = -e 's/version="0"/version="$(VERSION)"/'
ifeq ($(CPU_ARM64),true)
MACOS_PACKAGE_EXP += -e 's/hostArchitectures="x86_64"/hostArchitectures="arm64,x86_64"/'
endif

build/Carla-Plugins.pkg: build/carla-lv2.pkg build/carla-vst2fx.pkg build/carla-vst2syn.pkg build/package.xml
	productbuild \
		--distribution build/package.xml \
		--identifier studio.kx.carla \
		--package-path "build" \
		--version "$(VERSION)" \
		"$@"

build/package.xml: data/macos/package.xml
	sed $(MACOS_PACKAGE_EXP) $< > $@

build/carla-lv2.pkg: $(_CARLA_LV2_PLUGIN_FILES:%=build/%)
	pkgbuild \
		--identifier "studio.kx.carla.lv2" \
		--install-location "/Library/Audio/Plug-Ins/LV2/carla.lv2/" \
		--root "build/carla.lv2/" \
		"$@"

build/carla-vst2fx.pkg: $(_CARLA_VST2FX_PLUGIN_FILES:%=build/%)
	pkgbuild \
		--identifier "studio.kx.carla.vst2fx" \
		--install-location "/Library/Audio/Plug-Ins/VST/carlafx.vst/" \
		--root "build/carlafx.vst/" \
		"$@"

build/carla-vst2syn.pkg: $(_CARLA_VST2SYN_PLUGIN_FILES:%=build/%)
	pkgbuild \
		--identifier "studio.kx.carla.vst2syn" \
		--install-location "/Library/Audio/Plug-Ins/VST/carla.vst/" \
		--root "build/carla.vst/" \
		"$@"

# ----------------------------------------------------------------------------------------------------------------------------
# LV2 specific files

build/carla.lv2/%$(LIB_EXT): bin/carla.lv2/%$(LIB_EXT)
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/carla.lv2/manifest.ttl: bin/carla.lv2/manifest.ttl
	-@mkdir -p $(shell dirname $@)
	@cp -v bin/carla.lv2/*.ttl build/carla.lv2/

# ----------------------------------------------------------------------------------------------------------------------------
# VST2 specific files

_MACVST_FILES = Contents/Info.plist Contents/PkgInfo Contents/Resources/empty.lproj

build/carla.vst/Contents/MacOS/CarlaVstShell: bin/CarlaVstShell.dylib $(_MACVST_FILES:%=build/carla.vst/%)
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/carlafx.vst/Contents/MacOS/CarlaVstFxShell: bin/CarlaVstFxShell.dylib $(_MACVST_FILES:%=build/carlafx.vst/%)
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

# ----------------------------------------------------------------------------------------------------------------------------

build/carla.vst/Contents/Info.plist: data/macos/CarlaVstShell.plist
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/carlafx.vst/Contents/Info.plist: data/macos/CarlaVstFxShell.plist
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/%.vst/Contents/PkgInfo:
	-@mkdir -p $(shell dirname $@)
	@echo "BNDL????" > $@

build/%.vst/Contents/Resources/empty.lproj:
	-@mkdir -p $(shell dirname $@)
	@touch $@

# ----------------------------------------------------------------------------------------------------------------------------
# Generic plugin rules

build/carla%/resources/carla-plugin-patchbay$(APP_EXT): build/carla%/resources/carla-plugin$(APP_EXT)
	-@mkdir -p $(shell dirname $@)
	@ln -sfv carla-plugin$(APP_EXT) $@

# ----------------------------------------------------------------------------------------------------------------------------

build/carla.lv2/resources/% build/carla.vst/Contents/MacOS/resources/% build/carlafx.vst/Contents/MacOS/resources/%: build/Carla.app/Contents/MacOS/resources/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/carla.lv2/resources/lib/library.zip build/carla.vst/Contents/MacOS/resources/lib/library.zip build/carlafx.vst/Contents/MacOS/resources/lib/library.zip: build/Carla.app/Contents/MacOS/resources/lib/library.zip
	-@mkdir -p $(shell dirname $@)
	@cp -rv build/Carla.app/Contents/MacOS/resources/lib/* $(shell dirname $@)/

build/carla.lv2/% build/carla.vst/Contents/MacOS/% build/carlafx.vst/Contents/MacOS/%: build/Carla.app/Contents/MacOS/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

# ----------------------------------------------------------------------------------------------------------------------------
