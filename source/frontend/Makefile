#!/usr/bin/make -f
# Makefile for carla-frontend #
# --------------------------- #
# Created by falkTX
#

CWD=..
include $(CWD)/Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

BINDIR := $(CWD)/../bin
RESDIR := $(CWD)/../resources

ifeq ($(DEBUG),true)
OBJDIR := $(CWD)/../build/frontend/Debug
else
OBJDIR := $(CWD)/../build/frontend/Release
endif

# ---------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += -I. -Iutils
BUILD_CXX_FLAGS += -I../backend
BUILD_CXX_FLAGS += -I../includes
BUILD_CXX_FLAGS += -I../modules
BUILD_CXX_FLAGS += -I../utils

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(WINDOWS),true)
NON_STATIC_LINK_FLAGS = $(shell echo $(LINK_FLAGS) | awk 'sub(" -static","")') -static-libgcc
else
NON_STATIC_LINK_FLAGS = $(LINK_FLAGS)
endif

ifeq ($(STATIC_PLUGIN_TARGET),true)

ifeq ($(WINDOWS),true)
QT_LINK_FLAGS += -L$(BINDIR) $(BINDIR)/libcarla_utils.dll
else
QT_LINK_FLAGS += -L$(BINDIR) -lcarla_utils
endif

else

ifeq ($(WINDOWS),true)
QT_LINK_FLAGS += -L$(BINDIR) $(BINDIR)/libcarla_standalone2.dll $(BINDIR)/libcarla_utils.dll
else
QT_LINK_FLAGS += -L$(BINDIR) -lcarla_standalone2 -lcarla_utils
endif

endif

ifeq ($(MACOS),true)
QT_LINK_FLAGS += -install_name @rpath/libcarla_frontend.dylib
# FIXME this does not work: -Wl,-rpath,@loader_path
else
QT_LINK_FLAGS += -Wl,-rpath,'$${ORIGIN}'
endif

# ---------------------------------------------------------------------------------------------------------------------
# Translations

TSs = $(patsubst %,translations/carla_%.ts,$(I18N_LANGUAGES))
QMs = $(patsubst %,translations/carla_%.qm,$(I18N_LANGUAGES))

# ---------------------------------------------------------------------------------------------------------------------
# C++ files

CPP_FILES = \
	carla_frontend.cpp \
	dialogs/aboutdialog.cpp \
	dialogs/jackappdialog.cpp \
	pluginlist/pluginlistdialog.cpp

OBJS = $(CPP_FILES:%=$(OBJDIR)/%.o)

# ---------------------------------------------------------------------------------------------------------------------
# Resources

RES = \
	qt_config.py \
	resources_rc.py \
	$(BINDIR)/resources/patchcanvas \
	$(BINDIR)/resources/widgets \
	$(BINDIR)/resources/bigmeter-ui \
	$(BINDIR)/resources/carla-plugin \
	$(BINDIR)/resources/carla-plugin-patchbay \
	$(BINDIR)/resources/carla_app.py \
	$(BINDIR)/resources/carla_backend.py \
	$(BINDIR)/resources/carla_backend_qt.py \
	$(BINDIR)/resources/carla_host.py \
	$(BINDIR)/resources/carla_host_control.py \
	$(BINDIR)/resources/carla_settings.py \
	$(BINDIR)/resources/carla_skin.py \
	$(BINDIR)/resources/carla_shared.py \
	$(BINDIR)/resources/carla_utils.py \
	$(BINDIR)/resources/carla_widgets.py \
	$(BINDIR)/resources/externalui.py \
	$(BINDIR)/resources/midipattern-ui \
	$(BINDIR)/resources/notes-ui \
	$(BINDIR)/resources/xycontroller-ui \
	$(BINDIR)/resources/resources_rc.py \
	$(BINDIR)/resources/ui_carla_edit.py \
	$(BINDIR)/resources/ui_carla_host.py \
	$(BINDIR)/resources/ui_carla_osc_connect.py \
	$(BINDIR)/resources/ui_carla_parameter.py \
	$(BINDIR)/resources/ui_carla_plugin_calf.py \
	$(BINDIR)/resources/ui_carla_plugin_classic.py \
	$(BINDIR)/resources/ui_carla_plugin_compact.py \
	$(BINDIR)/resources/ui_carla_plugin_default.py \
	$(BINDIR)/resources/ui_carla_plugin_presets.py \
	$(BINDIR)/resources/ui_carla_settings.py \
	$(BINDIR)/resources/ui_carla_settings_driver.py \
	$(BINDIR)/resources/ui_inputdialog_value.py \
	$(BINDIR)/resources/ui_midipattern.py \
	$(BINDIR)/resources/ui_xycontroller.py

ifneq ($(SKIP_ZYN_SYNTH),true)
ifeq ($(HAVE_ZYN_UI_DEPS),true)
RES += \
	$(BINDIR)/resources/zynaddsubfx \
	$(BINDIR)/resources/zynaddsubfx-ui
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# UI code

DIALOG_UI_FILES     = $(wildcard dialogs/*.ui)
PLUGINLIST_UI_FILES = $(wildcard pluginlist/*.ui)

UIs  = $(DIALOG_UI_FILES:dialogs/%.ui=dialogs/ui_%.h)
UIs += $(PLUGINLIST_UI_FILES:pluginlist/%.ui=pluginlist/ui_%.h)

# old stuff, not yet converted
UIs += \
	ui_carla_edit.py \
	ui_carla_host.py \
	ui_carla_osc_connect.py \
	ui_carla_parameter.py \
	ui_carla_plugin_calf.py \
	ui_carla_plugin_classic.py \
	ui_carla_plugin_compact.py \
	ui_carla_plugin_default.py \
	ui_carla_plugin_presets.py \
	ui_carla_settings.py \
	ui_carla_settings_driver.py \
	ui_inputdialog_value.py \
	ui_midipattern.py \
	ui_xycontroller.py

# ---------------------------------------------------------------------------------------------------------------------

all: $(BINDIR)/libcarla_frontend$(LIB_EXT) $(QMs) $(RES) $(UIs)

# ---------------------------------------------------------------------------------------------------------------------

dialogs/ui_%.h: dialogs/%.ui qt_config.py
	$(UIC) $< -o $@

pluginlist/ui_%.h: pluginlist/%.ui qt_config.py
	$(UIC) $< -o $@

%_ui.py: %.ui qt_config.py
	$(PYUIC) $< -o $@

ui_%.py: $(RESDIR)/ui/%.ui qt_config.py
	$(PYUIC) $< -o $@

qt_config.py:
ifneq (,$(FRONTEND_TYPE))
	echo "#!/usr/bin/env python3" > $@
	echo "qt = $(FRONTEND_TYPE)" >> $@
else
	@echo Error: Trying to build frontend without Qt and/or PyQt, cannot continue
	@false
endif

resources_rc.py: $(RESDIR)/resources.qrc $(RESDIR)/*/*.png $(RESDIR)/*/*.svg $(RESDIR)/*/*.svgz
ifeq ($(FRONTEND_TYPE),6)
	$(PYRCC) $< | sed -e 's/PySide6/PyQt6/' > $@
else
	$(PYRCC) $< -o $@
endif

test: $(OBJS) $(LIBS) test.cpp
	@echo "Linking test"
	$(SILENT)$(CXX) $(OBJS) test.cpp $(BUILD_CXX_FLAGS) $(NON_STATIC_LINK_FLAGS) -Wl,-rpath,$(abspath $(BINDIR)) $(QT_LINK_FLAGS) -o $@

$(BINDIR)/resources/%: %
	-@mkdir -p $(BINDIR)/resources
	$(LINK) $(CURDIR)/$* $(BINDIR)/resources/

$(BINDIR)/resources/zynaddsubfx: ../native-plugins/resources/zynaddsubfx
	-@mkdir -p $(BINDIR)/resources
	$(LINK) $(CURDIR)/$< $(BINDIR)/resources/

$(BINDIR)/resources/zynaddsubfx-ui: ../native-plugins/resources/zynaddsubfx-ui
	-@mkdir -p $(BINDIR)/resources
	$(LINK) $(CURDIR)/$< $(BINDIR)/resources/

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/libcarla_frontend$(LIB_EXT): $(OBJS) $(LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking libcarla_frontend$(LIB_EXT)"
	$(SILENT)$(CXX) $(OBJS) $(BUILD_CXX_FLAGS) $(QT_CXX_FLAGS) $(NON_STATIC_LINK_FLAGS) $(QT_LINK_FLAGS) $(SHARED) -o $@
ifeq ($(MACOS),true)
# FIXME this does not work: -Wl,-rpath,@loader_path
	$(SILENT)install_name_tool -change ../../../bin/libcarla_utils.dylib @loader_path/libcarla_utils.dylib $@
endif

$(OBJDIR)/%.cpp.o: %.cpp $(UIs)
	-@mkdir -p $(shell dirname $@)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(QT_CXX_FLAGS) -c -o $@

-include $(OBJS:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -rf $(BINDIR)/libcarla_frontend$(LIB_EXT) $(UIs) $(RES) $(QMs) __pycache__ *.pyc
	# old files
	rm -f ui_carla_add_jack.py
	rm -f ui_carla_database.py
	rm -f ui_carla_refresh.py

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

lint:
	pylint \
	--extension-pkg-whitelist=PyQt5 \
	--max-line-length=120 \
	--max-locals=25 \
	--max-statements=100 \
	--disable=bare-except,duplicate-code,invalid-name,missing-docstring,too-many-branches,too-many-instance-attributes,too-many-statements \
	$(wildcard widgets/*.py)

# ---------------------------------------------------------------------------------------------------------------------

i18n_update: $(TSs)
i18n_release: $(QMs)

translations/%.ts:
	@install -d translations
	pylupdate5 *.py $(RESDIR)/ui/*.ui -ts $@

%.qm: %.ts
	lrelease $< -qm $@

# ---------------------------------------------------------------------------------------------------------------------
