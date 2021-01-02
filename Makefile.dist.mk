#!/usr/bin/make -f
# Makefile for Carla #
# ------------------ #
# Created by falkTX
#

# NOTE to be imported from main Makefile

# ----------------------------------------------------------------------------------------------------------------------------

QT5_PREFIX = $(shell pkg-config --variable=prefix Qt5OpenGLExtensions)

# ----------------------------------------------------------------------------------------------------------------------------

_PLUGIN_UIS = \
	carla-plugin \
	bigmeter-ui \
	midipattern-ui \
	notes-ui \
	xycontroller-ui

_QT5_DLLS = \
	iconengines/libqsvgicon$(LIB_EXT) \
	imageformats/libqjpeg$(LIB_EXT) \
	imageformats/libqsvg$(LIB_EXT) \
	platforms/libqcocoa$(LIB_EXT) \
	platforms/libqminimal$(LIB_EXT) \
	platforms/libqoffscreen$(LIB_EXT) \
	styles/carlastyle$(LIB_EXT)

_THEME_FILES = \
	styles/carlastyle.json \
	styles/carlastyle$(LIB_EXT)

_CARLA_APP_FILES = \
	Carla \
	carla-bridge-lv2-cocoa \
	carla-bridge-lv2$(LIB_EXT) \
	carla-bridge-native \
	carla-discovery-native \
	libcarla_standalone2$(LIB_EXT) \
	libcarla_utils$(LIB_EXT) \
	$(_PLUGIN_UIS:%=resources/%$(APP_EXT)) \
	$(_QT5_DLLS) \
	$(_THEME_FILES)

_CARLA_CONTROL_APP_FILES = \
	Carla-Control \
	libcarla_utils$(LIB_EXT) \
	$(_QT5_DLLS) \
	$(_THEME_FILES)

CARLA_APP_FILES = $(_CARLA_APP_FILES:%=build/Carla.app/Contents/MacOS/%)
CARLA_QT5_DLLS = $(_QT5_DLLS:%=build/Carla.app/Contents/MacOS/%)
CARLA_PLUGIN_ZIPS = $(_PLUGIN_UIS:%=build/%.app/Contents/MacOS/lib/library.zip)

CARLA_CONTROL_APP_FILES = $(_CARLA_CONTROL_APP_FILES:%=build/Carla-Control.app/Contents/MacOS/%)
CARLA_CONTROL_QT5_DLLS = $(_QT5_DLLS:%=build/Carla-Control.app/Contents/MacOS/%)

# ----------------------------------------------------------------------------------------------------------------------------

build/Carla.app/Contents/MacOS/Carla: build/Carla.app/Contents/MacOS/lib/library.zip $(CARLA_QT5_DLLS)
	# cleanup
	find build/Carla.app/Contents/MacOS/ -type f -name "*.py" -delete
	find build/Carla.app/Contents/MacOS/ -type f -name "*.pyi" -delete
	find build/Carla.app/Contents/MacOS/ -type f -name "pylupdate.so" -delete
	find build/Carla.app/Contents/MacOS/ -type f -name "pyrcc.so" -delete
	find build/Carla.app/Contents/MacOS/ -type f -name "QtMacExtras*" -delete
	find build/Carla.app/Contents/MacOS/ -type f -name "QtNetwork*" -delete
	find build/Carla.app/Contents/MacOS/ -type f -name "QtSql*" -delete
	find build/Carla.app/Contents/MacOS/ -type f -name "QtTest*" -delete
	find build/Carla.app/Contents/MacOS/ -type f -name "QtXml*" -delete
	find build/Carla.app/Contents/MacOS/ -type f -name "*.pyc" -delete
	rm -rf build/Carla.app/Contents/MacOS/lib/PyQt5/uic
	# adjust binaries
	(cd build/Carla.app/Contents/MacOS && \
		for f in `find . -type f | grep -e Q -e libq -e carlastyle.dylib`; do \
			install_name_tool -change "@rpath/QtCore.framework/Versions/5/QtCore"                 @executable_path/QtCore         $$f && \
			install_name_tool -change "@rpath/QtGui.framework/Versions/5/QtGui"                   @executable_path/QtGui          $$f && \
			install_name_tool -change "@rpath/QtOpenGL.framework/Versions/5/QtOpenGL"             @executable_path/QtOpenGL       $$f && \
			install_name_tool -change "@rpath/QtPrintSupport.framework/Versions/5/QtPrintSupport" @executable_path/QtPrintSupport $$f && \
			install_name_tool -change "@rpath/QtSvg.framework/Versions/5/QtSvg"                   @executable_path/QtSvg          $$f && \
			install_name_tool -change "@rpath/QtWidgets.framework/Versions/5/QtWidgets"           @executable_path/QtWidgets      $$f && \
			install_name_tool -change "@rpath/QtMacExtras.framework/Versions/5/QtMacExtras"       @executable_path/QtMacExtras    $$f; \
		done)
	# symlink resources
	mkdir -p build/Carla.app/Contents/MacOS/resources
	(cd build/Carla.app/Contents/MacOS/resources && \
		ln -sf ../Qt* ../lib ../iconengines ../imageformats ../platforms ../styles . && \
		ln -sf carla-plugin carla-plugin-patchbay)

build/Carla.app/Contents/MacOS/lib/library.zip: $(CARLA_PLUGIN_ZIPS) data/macos/bundle.py data/macos/*.plist source/frontend/*
	env PYTHONPATH=$(CURDIR)/source/frontend SCRIPT_NAME=Carla python3 ./data/macos/bundle.py bdist_mac --bundle-name=Carla
	# merge all zips into 1
	rm -rf build/Carla.app/Contents/MacOS/lib/_lib
	mkdir build/Carla.app/Contents/MacOS/lib/_lib
	(cd build/Carla.app/Contents/MacOS/lib/_lib && \
		mv ../library.zip ../library-main.zip && \
		$(_CARLA_PLUGIN_UIS:%=unzip -n $(CURDIR)/build/%.app/Contents/MacOS/lib/library.zip &&) \
		unzip -o ../library-main.zip && \
		zip -r -9 ../library.zip *)
	rm -rf build/Carla.app/Contents/MacOS/lib/_lib
	rm -rf build/Carla.app/Contents/MacOS/lib/library-main.zip

build/Carla.app/Contents/MacOS/iconengines/%: ${QT5_PREFIX}/lib/qt5/plugins/iconengines/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/Carla.app/Contents/MacOS/imageformats/%: ${QT5_PREFIX}/lib/qt5/plugins/imageformats/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/Carla.app/Contents/MacOS/platforms/%: ${QT5_PREFIX}/lib/qt5/plugins/platforms/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/Carla.app/Contents/MacOS/resources/%: build/%.app/Contents/MacOS/lib/library.zip source/frontend/%
	-@mkdir -p $(shell dirname $@)
	@cp -v build/$*.app/Contents/MacOS/$* $@

build/Carla.app/Contents/MacOS/%: bin/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

# ----------------------------------------------------------------------------------------------------------------------------

build/Carla-Control.app/Contents/MacOS/Carla-Control: build/Carla-Control.app/Contents/MacOS/lib/library.zip $(CARLA_CONTROL_QT5_DLLS)
	# cleanup
	find build/Carla-Control.app/Contents/MacOS/ -type f -name "*.py" -delete
	find build/Carla-Control.app/Contents/MacOS/ -type f -name "*.pyi" -delete
	find build/Carla-Control.app/Contents/MacOS/ -type f -name "pylupdate.so" -delete
	find build/Carla-Control.app/Contents/MacOS/ -type f -name "pyrcc.so" -delete
	find build/Carla-Control.app/Contents/MacOS/ -type f -name "QtMacExtras*" -delete
	find build/Carla-Control.app/Contents/MacOS/ -type f -name "QtNetwork*" -delete
	find build/Carla-Control.app/Contents/MacOS/ -type f -name "QtSql*" -delete
	find build/Carla-Control.app/Contents/MacOS/ -type f -name "QtTest*" -delete
	find build/Carla-Control.app/Contents/MacOS/ -type f -name "QtXml*" -delete
	find build/Carla-Control.app/Contents/MacOS/ -type f -name "*.pyc" -delete
	rm -rf build/Carla-Control.app/Contents/MacOS/lib/PyQt5/uic
	# adjust binaries
	(cd build/Carla-Control.app/Contents/MacOS && \
		for f in `find . -type f | grep -e Q -e libq -e carlastyle.dylib`; do \
			install_name_tool -change "@rpath/QtCore.framework/Versions/5/QtCore"                 @executable_path/QtCore         $$f && \
			install_name_tool -change "@rpath/QtGui.framework/Versions/5/QtGui"                   @executable_path/QtGui          $$f && \
			install_name_tool -change "@rpath/QtOpenGL.framework/Versions/5/QtOpenGL"             @executable_path/QtOpenGL       $$f && \
			install_name_tool -change "@rpath/QtPrintSupport.framework/Versions/5/QtPrintSupport" @executable_path/QtPrintSupport $$f && \
			install_name_tool -change "@rpath/QtSvg.framework/Versions/5/QtSvg"                   @executable_path/QtSvg          $$f && \
			install_name_tool -change "@rpath/QtWidgets.framework/Versions/5/QtWidgets"           @executable_path/QtWidgets      $$f && \
			install_name_tool -change "@rpath/QtMacExtras.framework/Versions/5/QtMacExtras"       @executable_path/QtMacExtras    $$f; \
		done)

build/Carla-Control.app/Contents/MacOS/iconengines/%: ${QT5_PREFIX}/lib/qt5/plugins/iconengines/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/Carla-Control.app/Contents/MacOS/imageformats/%: ${QT5_PREFIX}/lib/qt5/plugins/imageformats/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/Carla-Control.app/Contents/MacOS/platforms/%: ${QT5_PREFIX}/lib/qt5/plugins/platforms/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

build/Carla-Control.app/Contents/MacOS/resources/%: build/%.app/Contents/MacOS/lib/library.zip source/frontend/%
	-@mkdir -p $(shell dirname $@)
	@cp -v build/$*.app/Contents/MacOS/$* $@

build/Carla-Control.app/Contents/MacOS/%: bin/%
	-@mkdir -p $(shell dirname $@)
	@cp -v $< $@

# ----------------------------------------------------------------------------------------------------------------------------

build/%.app/Contents/MacOS/lib/library.zip: data/macos/bundle.py data/macos/*.plist source/frontend/*
	env PYTHONPATH=$(CURDIR)/source/frontend SCRIPT_NAME=$* python3 ./data/macos/bundle.py bdist_mac --bundle-name=$*

# ----------------------------------------------------------------------------------------------------------------------------

dist: carla_mac

carla_mac: $(CARLA_APP_FILES) $(CARLA_CONTROL_APP_FILES)

# ----------------------------------------------------------------------------------------------------------------------------
