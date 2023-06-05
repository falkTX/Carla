#!/usr/bin/make -f
# Makefile for Carla #
# ------------------ #
# Created by falkTX
#

# NOTE to be imported from main Makefile

# ---------------------------------------------------------------------------------------------------------------------

PYTHON = $(EXE_WRAPPER) $(shell which python3$(APP_EXT))

ifeq ($(WINDOWS),true)
QT5_DLL_EXT = .dll
QT5_DLL_V = 5
endif

ifeq ($(MACOS),true)
QT5_LIB_PREFIX = lib
endif

ifeq ($(HAVE_QT5PKG),true)
QT5_PREFIX = $(shell pkg-config --variable=prefix Qt5OpenGLExtensions)
endif

# ---------------------------------------------------------------------------------------------------------------------

_PLUGIN_UIS = \
	carla-plugin \
	carla-plugin-patchbay \
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
_QT5_PLUGINS += platforms/$(QT5_LIB_PREFIX)qcocoa$(LIB_EXT)
else ifeq ($(WINDOWS),true)
_QT5_PLUGINS += platforms/$(QT5_LIB_PREFIX)qwindows$(LIB_EXT)
endif

# NOTE this has to be hardcoded for now. oh well
ifeq ($(WINDOWS),true)
_PYTHON_FILES = \
	libpython3.8.dll
endif

_THEME_FILES = \
	styles/carlastyle.json \
	styles/carlastyle$(LIB_EXT)

_CARLA_HOST_FILES = \
	carla-bridge-lv2$(LIB_EXT) \
	carla-bridge-lv2-gtk2$(APP_EXT) \
	carla-bridge-lv2-gtk3$(APP_EXT) \
	carla-bridge-native$(APP_EXT) \
	carla-discovery-native$(APP_EXT) \
	libcarla_frontend$(LIB_EXT) \
	libcarla_utils$(LIB_EXT) \
	$(_PLUGIN_UIS:%=resources/%$(APP_EXT))

ifeq ($(MACOS),true)
_CARLA_HOST_FILES += carla-bridge-lv2-cocoa$(APP_EXT)
_CARLA_HOST_FILES += magic.mgc
else ifeq ($(WINDOWS),true)
_CARLA_HOST_FILES += carla-bridge-lv2-windows$(APP_EXT)
endif

ifeq ($(MACOS),true)
ifneq ($(MACOS_UNIVERSAL),true)
_CARLA_HOST_FILES += carla-bridge-posix32
_CARLA_HOST_FILES += carla-discovery-posix32
endif
endif

ifeq ($(WIN64),true)
_CARLA_HOST_FILES += carla-bridge-win32$(APP_EXT)
_CARLA_HOST_FILES += carla-discovery-win32$(APP_EXT)
endif

_CARLA_APP_FILES = \
	$(_CARLA_HOST_FILES) \
	$(_PYTHON_FILES) \
	$(_QT5_DLLS) \
	$(_QT5_PLUGINS) \
	$(_THEME_FILES) \
	resources/lib

ifneq ($(EMBED_TARGET),true)
_CARLA_APP_FILES += \
	Carla$(APP_EXT) \
	libcarla_host-plugin$(LIB_EXT) \
	libcarla_native-plugin$(LIB_EXT) \
	libcarla_standalone2$(LIB_EXT)

_CARLA_CONTROL_APP_FILES = \
	Carla-Control$(APP_EXT) \
	libcarla_frontend$(LIB_EXT) \
	libcarla_utils$(LIB_EXT) \
	$(_PYTHON_FILES) \
	$(_QT5_DLLS) \
	$(_QT5_PLUGINS) \
	$(_THEME_FILES)

_CARLA_LV2_PLUGIN_FILES = \
	carla.lv2/carla$(LIB_EXT) \
	carla.lv2/manifest.ttl \
	carla.lv2/resources/lib/library.zip \
	$(_CARLA_HOST_FILES:%=carla.lv2/%) \
	$(_PYTHON_FILES:%=carla.lv2/resources/%) \
	$(_QT5_DLLS:%=carla.lv2/resources/%) \
	$(_QT5_PLUGINS:%=carla.lv2/resources/%) \
	$(_THEME_FILES:%=carla.lv2/resources/%)

ifeq ($(MACOS),true)
_CARLA_VST2FX_PLUGIN_FILES = \
	carlafx.vst/Contents/MacOS/CarlaVstFxShell \
	carlafx.vst/Contents/MacOS/resources/lib/library.zip \
	$(_CARLA_HOST_FILES:%=carlafx.vst/Contents/MacOS/%) \
	$(_QT5_DLLS:%=carlafx.vst/Contents/MacOS/resources/%) \
	$(_QT5_PLUGINS:%=carlafx.vst/Contents/MacOS/resources/%) \
	$(_THEME_FILES:%=carlafx.vst/Contents/MacOS/resources/%)

_CARLA_VST2SYN_PLUGIN_FILES = \
	carla.vst/Contents/MacOS/CarlaVstShell \
	carla.vst/Contents/MacOS/resources/lib/library.zip \
	$(_CARLA_HOST_FILES:%=carla.vst/Contents/MacOS/%) \
	$(_QT5_DLLS:%=carla.vst/Contents/MacOS/resources/%) \
	$(_QT5_PLUGINS:%=carla.vst/Contents/MacOS/resources/%) \
	$(_THEME_FILES:%=carla.vst/Contents/MacOS/resources/%)
else ifeq ($(WINDOWS),true)
_CARLA_VST2_PLUGIN_FILES = \
	carla.vst/CarlaVstShell.dll \
	carla.vst/CarlaVstFxShell.dll \
	carla.vst/resources/lib/library.zip \
	$(_CARLA_HOST_FILES:%=carla.vst/%) \
	$(_PYTHON_FILES:%=carla.vst/resources/%) \
	$(_QT5_DLLS:%=carla.vst/resources/%) \
	$(_QT5_PLUGINS:%=carla.vst/resources/%) \
	$(_THEME_FILES:%=carla.vst/resources/%)
endif
endif # EMBED_TARGET

ifeq ($(MACOS),true)
CARLA_APP_FILES = $(_CARLA_APP_FILES:%=build/Carla.app/Contents/MacOS/%)
CARLA_CONTROL_APP_FILES = $(_CARLA_CONTROL_APP_FILES:%=build/Carla-Control.app/Contents/MacOS/%)
CARLA_PLUGIN_ZIPS = $(_PLUGIN_UIS:%=build/%.app/Contents/MacOS/lib/library.zip)
else ifeq ($(WINDOWS),true)
CARLA_APP_FILES = $(_CARLA_APP_FILES:%=build/Carla/%)
CARLA_CONTROL_APP_FILES = $(_CARLA_CONTROL_APP_FILES:%=build/Carla-Control/%)
CARLA_PLUGIN_FILES = $(_CARLA_LV2_PLUGIN_FILES:%=build/%) $(_CARLA_VST2_PLUGIN_FILES:%=build/%)
CARLA_PLUGIN_ZIPS = $(_PLUGIN_UIS:%=build/%-resources/lib/library.zip)
endif

# ---------------------------------------------------------------------------------------------------------------------
# entry point

ifeq ($(MACOS_UNIVERSAL),true)
TARGETS = Carla-$(VERSION)-macOS-universal.dmg
else ifeq ($(MACOS),true)
TARGETS = Carla-$(VERSION)-macOS.dmg
else ifeq ($(WINDOWS),true)
ifeq ($(CPU_X86_64),true)
TARGETS = Carla-$(VERSION)-win64.zip
else
TARGETS = Carla-$(VERSION)-win32.zip
endif
endif

ifeq ($(HAVE_QT5PKG),true)
dist: $(TARGETS)
else
dist:
	@echo make dist not supported in this configuration
endif

# ---------------------------------------------------------------------------------------------------------------------
# create final file

Carla-$(VERSION)-macOS.dmg: $(CARLA_APP_FILES) $(CARLA_CONTROL_APP_FILES) $(CARLA_PLUGIN_FILES)build/Carla.app/Contents/Info.plist build/Carla-Control.app/Contents/Info.plist build/Carla-Plugins.pkg
	$(call GENERATE_FINAL_DMG,intel)

Carla-$(VERSION)-macOS-universal.dmg: $(CARLA_APP_FILES) $(CARLA_CONTROL_APP_FILES) $(CARLA_PLUGIN_FILES)build/Carla.app/Contents/Info.plist build/Carla-Control.app/Contents/Info.plist build/Carla-Plugins.pkg
	$(call GENERATE_FINAL_DMG,universal)

Carla-$(VERSION)-win32.zip: $(CARLA_APP_FILES) $(CARLA_CONTROL_APP_FILES) $(CARLA_PLUGIN_FILES)
	$(call GENERATE_FINAL_ZIP,win32)

Carla-$(VERSION)-win64.zip: $(CARLA_APP_FILES) $(CARLA_CONTROL_APP_FILES) $(CARLA_PLUGIN_FILES)
	$(call GENERATE_FINAL_ZIP,win64)

ifneq ($(TESTING),true)
define GENERATE_FINAL_DMG
	rm -rf build/macos-pkg $@
	mkdir build/macos-pkg
	cp -r build/Carla.app build/Carla-Control.app build/Carla-Plugins.pkg data/macos/README build/macos-pkg/
	hdiutil create $@ -srcfolder build/macos-pkg -volname "Carla-$(VERSION)-${1}" -fs HFS+ -ov
	rm -rf build/macos-pkg
endef
define GENERATE_FINAL_ZIP
	rm -rf build/Carla-$(VERSION)-${1} $@
	mkdir build/Carla-$(VERSION)-${1}
	cp -r build/Carla build/Carla-Control data/windows/README.txt build/Carla-$(VERSION)-${1}/
	cp -r build/carla.lv2 build/Carla-$(VERSION)-${1}/Carla.lv2
	cp -r build/carla.vst build/Carla-$(VERSION)-${1}/Carla.vst
	(cd build && \
		zip -r -9 ../$@ Carla-$(VERSION)-${1})
	rm -rf build/Carla-$(VERSION)-${1}
endef
else
define GENERATE_FINAL_DMG
endef
define GENERATE_FINAL_ZIP
endef
endif

# ---------------------------------------------------------------------------------------------------------------------
# macOS plist files

build/Carla.app/Contents/Info.plist: build/Carla.app/Contents/MacOS/Carla
	$(call CLEANUP_AND_PATCH_CXFREEZE_FILES,Carla)
	# extra step for standalone, symlink resources used in plugin UIs
	mkdir -p build/Carla.app/Contents/MacOS/resources
	(cd build/Carla.app/Contents/MacOS/resources && \
		ln -sf ../Qt* ../lib ../iconengines ../imageformats ../platforms ../styles .)
	# mark as done
	touch $@

build/Carla-Control.app/Contents/Info.plist: build/Carla-Control.app/Contents/MacOS/Carla-Control
	$(call CLEANUP_AND_PATCH_CXFREEZE_FILES,Carla-Control)
	# mark as done
	touch $@

# ---------------------------------------------------------------------------------------------------------------------
# macOS main executables

build/Carla.app/Contents/MacOS/Carla: build/Carla.app/Contents/MacOS/lib/library.zip
	$(call CLEANUP_AND_PATCH_CXFREEZE_FILES,Carla)
	# mark as done
	touch $@

build/Carla-Control.app/Contents/MacOS/Carla-Control: build/Carla-Control.app/Contents/MacOS/lib/library.zip
	$(call CLEANUP_AND_PATCH_CXFREEZE_FILES,Carla-Control)
	# mark as done
	touch $@

# ---------------------------------------------------------------------------------------------------------------------
# win32 main executables

build/Carla/Carla.exe: build/Carla/lib/library.zip
	$(call CLEANUP_AND_PATCH_CXFREEZE_FILES,Carla)
	# mark as done
	touch $@

build/Carla-Control/Carla-Control.exe: build/Carla-Control/lib/library.zip
	$(call CLEANUP_AND_PATCH_CXFREEZE_FILES,Carla-Control)
	# mark as done
	touch $@

# ---------------------------------------------------------------------------------------------------------------------
# cleanup functions

ifeq ($(MACOS),true)
define PATCH_QT_DEPENDENCIES
	install_name_tool -change "@rpath/QtCore.framework/Versions/5/QtCore"                 @executable_path/QtCore         ${1} && \
	install_name_tool -change "@rpath/QtGui.framework/Versions/5/QtGui"                   @executable_path/QtGui          ${1} && \
	install_name_tool -change "@rpath/QtOpenGL.framework/Versions/5/QtOpenGL"             @executable_path/QtOpenGL       ${1} && \
	install_name_tool -change "@rpath/QtPrintSupport.framework/Versions/5/QtPrintSupport" @executable_path/QtPrintSupport ${1} && \
	install_name_tool -change "@rpath/QtSvg.framework/Versions/5/QtSvg"                   @executable_path/QtSvg          ${1} && \
	install_name_tool -change "@rpath/QtWidgets.framework/Versions/5/QtWidgets"           @executable_path/QtWidgets      ${1} && \
	install_name_tool -change "@rpath/QtMacExtras.framework/Versions/5/QtMacExtras"       @executable_path/QtMacExtras    ${1}
endef

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
else ifeq ($(WINDOWS),true)
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

# ---------------------------------------------------------------------------------------------------------------------
# cxfreeze library.zip generation function

ifeq ($(MACOS),true)
define GENERATE_LIBRARY_ZIP
	rm -rf build/exe.*
	env PYTHONPATH=$(CURDIR)/source/frontend SCRIPT_NAME=${1} $(PYTHON) ./data/macos/bundle.py bdist_mac --bundle-name=${1} 1>/dev/null
endef
else ifeq ($(WINDOWS),true)
define GENERATE_LIBRARY_ZIP
	env PYTHONPATH="$(CURDIR)/source/frontend;$(QT5_PREFIX)/lib/python3/site-packages" SCRIPT_NAME=${1} $(PYTHON) ./data/windows/app-gui.py build_exe 1>/dev/null
endef
endif

# ---------------------------------------------------------------------------------------------------------------------
# macOS application library zip files

build/Carla.app/Contents/MacOS/lib/library.zip: $(CARLA_PLUGIN_ZIPS) data/macos/bundle.py data/macos/Carla.plist source/frontend/*
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

build/Carla-Control.app/Contents/MacOS/lib/library.zip: data/macos/bundle.py data/macos/Carla-Control.plist source/frontend/*
	$(call GENERATE_LIBRARY_ZIP,Carla-Control)

build/%.app/Contents/MacOS/lib/library.zip: data/macos/bundle.py source/frontend/%
	$(call GENERATE_LIBRARY_ZIP,$*)

# ---------------------------------------------------------------------------------------------------------------------
# win32 application library zip files

build/Carla/lib/library.zip: $(CARLA_PLUGIN_ZIPS) data/windows/app-gui.py source/frontend/* resources/ico/carla.ico
	$(call GENERATE_LIBRARY_ZIP,Carla)
	# merge all zips into 1
	rm -rf build/Carla/lib/_lib
	mkdir build/Carla/lib/_lib
	(cd build/Carla/lib/_lib && \
		mv ../library.zip ../library-main.zip && \
		$(_PLUGIN_UIS:%=unzip -n $(CURDIR)/build/%-resources/lib/library.zip &&) \
		unzip -o ../library-main.zip && \
		zip -r -9 ../library.zip *)
	rm -rf build/Carla/lib/_lib
	rm -rf build/Carla/lib/library-main.zip
ifeq ($(EMBED_TARGET),true)
	rm -f build/Carla/Carla.exe
endif

build/Carla-Control/lib/library.zip: data/windows/app-gui.py source/frontend/* resources/ico/carla-control.ico
	$(call GENERATE_LIBRARY_ZIP,Carla-Control)

build/%-resources/lib/library.zip: data/windows/app-gui.py source/frontend/% resources/ico/carla.ico
	$(call GENERATE_LIBRARY_ZIP,$*)
	# delete useless files
	rm -rf build/$*-resources/lib/*.dll build/$*-resources/lib/*.pyd build/$*-resources/lib/PyQt5

# ---------------------------------------------------------------------------------------------------------------------
# macOS plugin UIs (stored in resources, depend on their respective library.zip)

build/Carla.app/Contents/MacOS/resources/%: build/%.app/Contents/MacOS/lib/library.zip
	-@mkdir -p $(shell dirname $@)
	@cp -v build/$*.app/Contents/MacOS/$* $@

# ---------------------------------------------------------------------------------------------------------------------
# win32 plugin UIs (stored in resources, depend on their respective library.zip)

build/Carla/resources/%.exe: build/%-resources/lib/library.zip
	-@mkdir -p $(shell dirname $@)
	@cp -v build/$*-resources/$*.exe $@

# ---------------------------------------------------------------------------------------------------------------------
# common generic bundle files (either Qt or Carla binaries)

ifeq ($(MACOS),true)
_BUNDLE_EXTRA_PATH = /Contents/MacOS
_APP_BUNDLE_EXTRA_PATH = .app$(_BUNDLE_EXTRA_PATH)
endif

build/Carla$(_APP_BUNDLE_EXTRA_PATH)/iconengines/% build/Carla-Control$(_APP_BUNDLE_EXTRA_PATH)/iconengines/%: $(QT5_PREFIX)/lib/qt5/plugins/iconengines/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/Carla$(_APP_BUNDLE_EXTRA_PATH)/imageformats/% build/Carla-Control$(_APP_BUNDLE_EXTRA_PATH)/imageformats/%: $(QT5_PREFIX)/lib/qt5/plugins/imageformats/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/Carla$(_APP_BUNDLE_EXTRA_PATH)/platforms/% build/Carla-Control$(_APP_BUNDLE_EXTRA_PATH)/platforms/%: $(QT5_PREFIX)/lib/qt5/plugins/platforms/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/Carla$(_APP_BUNDLE_EXTRA_PATH)/styles/% build/Carla-Control$(_APP_BUNDLE_EXTRA_PATH)/styles/%: bin/styles/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/Carla$(_APP_BUNDLE_EXTRA_PATH)/resources/lib: build/Carla$(_APP_BUNDLE_EXTRA_PATH)/lib/library.zip
	-@mkdir -p $(shell dirname $@)
	@ln -sfv ../lib $@

build/Carla$(_APP_BUNDLE_EXTRA_PATH)/% build/Carla-Control$(_APP_BUNDLE_EXTRA_PATH)/%: bin/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

# ---------------------------------------------------------------------------------------------------------------------
# macOS generic bundle files (either Qt or Carla binaries)

build/Carla.app/Contents/MacOS/Qt% build/Carla-Control.app/Contents/MacOS/Qt%: $(QT5_PREFIX)/lib/Qt%.framework
	-@mkdir -p $(shell dirname $@)
	@cp -v $</Versions/5/Qt$* $@

# ---------------------------------------------------------------------------------------------------------------------
# win32 generic bundle files (either Qt or Carla binaries)

build/Carla/libpython3% build/Carla-Control/libpython3%: $(QT5_PREFIX)/bin/libpython3%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/Carla$(_APP_BUNDLE_EXTRA_PATH)/Qt5% build/Carla-Control$(_APP_BUNDLE_EXTRA_PATH)/Qt5%: $(QT5_PREFIX)/bin/Qt5%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

# ---------------------------------------------------------------------------------------------------------------------
# common plugin rules

build/Carla$(_APP_BUNDLE_EXTRA_PATH)/resources/carla-plugin-patchbay$(APP_EXT): build/Carla$(_APP_BUNDLE_EXTRA_PATH)/resources/carla-plugin$(APP_EXT)
	@ln -sfv carla-plugin$(APP_EXT) $@

build/Carla$(_APP_BUNDLE_EXTRA_PATH)/magic.mgc build/carla.lv2/magic.mgc build/carla.vst$(_BUNDLE_EXTRA_PATH)/magic.mgc build/carlafx.vst$(_BUNDLE_EXTRA_PATH)/magic.mgc: $(QT5_PREFIX)/share/misc/magic.mgc
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/carla.lv2/resources/lib/library.zip build/carla.vst$(_BUNDLE_EXTRA_PATH)/resources/lib/library.zip build/carlafx.vst$(_BUNDLE_EXTRA_PATH)/resources/lib/library.zip: build/Carla$(_APP_BUNDLE_EXTRA_PATH)/resources/lib/library.zip
	-@mkdir -p $(shell dirname $@)
	@cp -rv build/Carla$(_APP_BUNDLE_EXTRA_PATH)/resources/lib/* $(shell dirname $@)/

build/carla.lv2/resources/Qt% build/carla.vst$(_BUNDLE_EXTRA_PATH)/resources/Qt% build/carlafx.vst$(_BUNDLE_EXTRA_PATH)/resources/Qt%: build/Carla$(_APP_BUNDLE_EXTRA_PATH)/Qt%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/carla.lv2/resources/iconengines/% build/carla.vst$(_BUNDLE_EXTRA_PATH)/resources/iconengines/% build/carlafx.vst$(_BUNDLE_EXTRA_PATH)/resources/iconengines/%: build/Carla$(_APP_BUNDLE_EXTRA_PATH)/iconengines/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/carla.lv2/resources/imageformats/% build/carla.vst$(_BUNDLE_EXTRA_PATH)/resources/imageformats/% build/carlafx.vst$(_BUNDLE_EXTRA_PATH)/resources/imageformats/%: build/Carla$(_APP_BUNDLE_EXTRA_PATH)/imageformats/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/carla.lv2/resources/platforms/% build/carla.vst$(_BUNDLE_EXTRA_PATH)/resources/platforms/% build/carlafx.vst$(_BUNDLE_EXTRA_PATH)/resources/platforms/%: build/Carla$(_APP_BUNDLE_EXTRA_PATH)/platforms/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/carla.lv2/resources/styles/% build/carla.vst$(_BUNDLE_EXTRA_PATH)/resources/styles/% build/carlafx.vst$(_BUNDLE_EXTRA_PATH)/resources/styles/%: build/Carla$(_APP_BUNDLE_EXTRA_PATH)/styles/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/carla.lv2/resources/% build/carla.vst$(_BUNDLE_EXTRA_PATH)/resources/% build/carlafx.vst$(_BUNDLE_EXTRA_PATH)/resources/%: build/Carla$(_APP_BUNDLE_EXTRA_PATH)/resources/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/carla.lv2/%$(LIB_EXT) build/carla.vst$(_BUNDLE_EXTRA_PATH)/%$(LIB_EXT) build/carlafx.vst$(_BUNDLE_EXTRA_PATH)/%$(LIB_EXT): bin/%$(LIB_EXT)
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/carla.lv2/% build/carla.vst$(_BUNDLE_EXTRA_PATH)/% build/carlafx.vst$(_BUNDLE_EXTRA_PATH)/%: build/Carla$(_APP_BUNDLE_EXTRA_PATH)/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

# build/carla.lv2/resources/% build/carla.vst/resources/%: build/Carla/resources/%
# 	-@mkdir -p $(shell dirname $@)
# 	@cp -v $< $@

# ---------------------------------------------------------------------------------------------------------------------
# win32 plugin rules

build/carla.lv2/resources/libpython3% build/carla.vst/resources/libpython3%: build/Carla/libpython3%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

# ---------------------------------------------------------------------------------------------------------------------
# LV2 specific files

build/carla.lv2/manifest.ttl: bin/carla.lv2/manifest.ttl
	-@mkdir -p $(shell dirname $@)
	@cp -v bin/carla.lv2/*.ttl build/carla.lv2/

build/carla.lv2/carla$(LIB_EXT): bin/carla.lv2/carla$(LIB_EXT)
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

# ---------------------------------------------------------------------------------------------------------------------
# macOS plugin rules

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

# ---------------------------------------------------------------------------------------------------------------------
# macOS VST2 specific files

_MACVST_FILES = Contents/Info.plist Contents/PkgInfo Contents/Resources/empty.lproj

build/carla.vst/Contents/MacOS/CarlaVstShell: bin/CarlaVstShell.dylib $(_MACVST_FILES:%=build/carla.vst/%)
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/carlafx.vst/Contents/MacOS/CarlaVstFxShell: bin/CarlaVstFxShell.dylib $(_MACVST_FILES:%=build/carlafx.vst/%)
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

# ---------------------------------------------------------------------------------------------------------------------

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

# ---------------------------------------------------------------------------------------------------------------------
