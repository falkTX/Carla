#!/usr/bin/make -f
# Makefile for carla-frontend #
# --------------------------- #
# Created by falkTX
#

CWD=..
include $(CWD)/Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

BINDIR    := $(CWD)/../bin
RESDIR    := $(CWD)/../resources

ifeq ($(DEBUG),true)
OBJDIR    := $(CWD)/../build/frontend/Debug
MODULEDIR := $(CWD)/../build/modules/Debug
else
OBJDIR    := $(CWD)/../build/frontend/Release
MODULEDIR := $(CWD)/../build/modules/Release
endif

# ---------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += -I. -I$(CWD) -I$(CWD)/backend -I$(CWD)/includes -I$(CWD)/modules -I$(CWD)/utils
BUILD_CXX_FLAGS += -DCARLA_SKIP_HOST_COMMON
BUILD_CXX_FLAGS += -Wno-unused-parameter
BUILD_CXX_FLAGS += -Wno-unused-variable
BUILD_CXX_FLAGS += $(FLUIDSYNTH_FLAGS)
BUILD_CXX_FLAGS += $(QT5_FLAGS)

# ---------------------------------------------------------------------------------------------------------------------
# Resources

ifeq ($(CPPMODE),true)
RES = \
	resources_rc.cpp
else
RES = \
	resources_rc.py \
	$(BINDIR)/resources/patchcanvas \
	$(BINDIR)/resources/widgets \
	$(BINDIR)/resources/carla_app.py \
	$(BINDIR)/resources/carla_backend.py \
	$(BINDIR)/resources/carla_backend_qt.py \
	$(BINDIR)/resources/carla_control.py \
	$(BINDIR)/resources/carla_database.py \
	$(BINDIR)/resources/carla_host.py \
	$(BINDIR)/resources/carla_modgui.py \
	$(BINDIR)/resources/carla_settings.py \
	$(BINDIR)/resources/carla_skin.py \
	$(BINDIR)/resources/carla_shared.py \
	$(BINDIR)/resources/carla_utils.py \
	$(BINDIR)/resources/carla_widgets.py \
	$(BINDIR)/resources/externalui.py \
	$(BINDIR)/resources/resources_rc.py \
	$(BINDIR)/resources/ui_carla_about.py \
	$(BINDIR)/resources/ui_carla_about_juce.py \
	$(BINDIR)/resources/ui_carla_add_jack.py \
	$(BINDIR)/resources/ui_carla_database.py \
	$(BINDIR)/resources/ui_carla_edit.py \
	$(BINDIR)/resources/ui_carla_host.py \
	$(BINDIR)/resources/ui_carla_osc_connect.py \
	$(BINDIR)/resources/ui_carla_parameter.py \
	$(BINDIR)/resources/ui_carla_plugin_calf.py \
	$(BINDIR)/resources/ui_carla_plugin_classic.py \
	$(BINDIR)/resources/ui_carla_plugin_compact.py \
	$(BINDIR)/resources/ui_carla_plugin_default.py \
	$(BINDIR)/resources/ui_carla_plugin_presets.py \
	$(BINDIR)/resources/ui_carla_refresh.py \
	$(BINDIR)/resources/ui_carla_settings.py \
	$(BINDIR)/resources/ui_carla_settings_driver.py \
	$(BINDIR)/resources/ui_inputdialog_value.py \
	$(BINDIR)/resources/ui_midipattern.py
endif

# ---------------------------------------------------------------------------------------------------------------------
# UI code

ifeq ($(CPPMODE),true)
UIs = \
	ui_carla_about.hpp \
	ui_carla_about_juce.hpp \
	ui_carla_add_jack.hpp \
	ui_carla_database.hpp \
	ui_carla_edit.hpp \
	ui_carla_host.hpp \
	ui_carla_osc_connect.hpp \
	ui_carla_parameter.hpp \
	ui_carla_plugin_calf.hpp \
	ui_carla_plugin_classic.hpp \
	ui_carla_plugin_compact.hpp \
	ui_carla_plugin_default.hpp \
	ui_carla_plugin_presets.hpp \
	ui_carla_refresh.hpp \
	ui_carla_settings.hpp \
	ui_carla_settings_driver.hpp \
	ui_inputdialog_value.hpp \
	ui_midipattern.hpp
else
UIs = \
	ui_carla_about.py \
	ui_carla_about_juce.py \
	ui_carla_add_jack.py \
	ui_carla_database.py \
	ui_carla_edit.py \
	ui_carla_host.py \
	ui_carla_osc_connect.py \
	ui_carla_parameter.py \
	ui_carla_plugin_calf.py \
	ui_carla_plugin_classic.py \
	ui_carla_plugin_compact.py \
	ui_carla_plugin_default.py \
	ui_carla_plugin_presets.py \
	ui_carla_refresh.py \
	ui_carla_settings.py \
	ui_carla_settings_driver.py \
	ui_inputdialog_value.py \
	ui_midipattern.py
endif

# ---------------------------------------------------------------------------------------------------------------------

LIBS  = $(MODULEDIR)/carla_engine.a
LIBS += $(MODULEDIR)/carla_plugin.a
LIBS += $(MODULEDIR)/jackbridge.a

LIBS += $(MODULEDIR)/audio_decoder.a
LIBS += $(MODULEDIR)/lilv.a
LIBS += $(MODULEDIR)/native-plugins.a
LIBS += $(MODULEDIR)/rtmempool.a
LIBS += $(MODULEDIR)/sfzero.a
LIBS += $(MODULEDIR)/water.a

ifeq ($(HAVE_DGL),true)
LIBS += $(MODULEDIR)/dgl.a
endif

ifeq ($(HAVE_HYLIA),true)
LIBS += $(MODULEDIR)/hylia.a
endif

ifeq ($(USING_JUCE),true)
LIBS += $(MODULEDIR)/juce_audio_basics.a
LIBS += $(MODULEDIR)/juce_audio_devices.a
LIBS += $(MODULEDIR)/juce_audio_processors.a
LIBS += $(MODULEDIR)/juce_core.a
LIBS += $(MODULEDIR)/juce_data_structures.a
LIBS += $(MODULEDIR)/juce_events.a
LIBS += $(MODULEDIR)/juce_graphics.a
LIBS += $(MODULEDIR)/juce_gui_basics.a
ifeq ($(MACOS),true)
LIBS += $(MODULEDIR)/juce_gui_extra.a
endif
else
LIBS += $(MODULEDIR)/rtaudio.a
LIBS += $(MODULEDIR)/rtmidi.a
endif

# ---------------------------------------------------------------------------------------------------------------------

LINK_FLAGS += $(QT5_LIBS)
LINK_FLAGS += $(HYLIA_LIBS)
LINK_FLAGS += $(JACKBRIDGE_LIBS)
LINK_FLAGS += $(LILV_LIBS)
LINK_FLAGS += $(NATIVE_PLUGINS_LIBS)
LINK_FLAGS += $(RTMEMPOOL_LIBS)
LINK_FLAGS += $(WATER_LIBS)

LINK_FLAGS += $(LIBLO_LIBS)
LINK_FLAGS += $(MAGIC_LIBS)
LINK_FLAGS += $(FLUIDSYNTH_LIBS)
LINK_FLAGS += $(X11_LIBS)

ifeq ($(USING_JUCE),true)
LINK_FLAGS += $(JUCE_AUDIO_BASICS_LIBS)
LINK_FLAGS += $(JUCE_AUDIO_DEVICES_LIBS)
LINK_FLAGS += $(JUCE_AUDIO_PROCESSORS_LIBS)
LINK_FLAGS += $(JUCE_CORE_LIBS)
LINK_FLAGS += $(JUCE_DATA_STRUCTURES_LIBS)
LINK_FLAGS += $(JUCE_EVENTS_LIBS)
LINK_FLAGS += $(JUCE_GRAPHICS_LIBS)
LINK_FLAGS += $(JUCE_GUI_BASICS_LIBS)
ifeq ($(MACOS),true)
LINK_FLAGS += $(JUCE_GUI_EXTRA_LIBS)
endif
else
LINK_FLAGS += $(RTAUDIO_LIBS)
LINK_FLAGS += $(RTMIDI_LIBS)
endif

ifeq ($(JACKBRIDGE_DIRECT),true)
LINK_FLAGS += $(JACK_LIBS)
endif

# ---------------------------------------------------------------------------------------------------------------------

OBJS = \
	$(OBJDIR)/carla_app.cpp.o \
	$(OBJDIR)/carla_database.cpp.o \
	$(OBJDIR)/carla_host.cpp.o \
	$(OBJDIR)/carla_settings.cpp.o \
	$(OBJDIR)/carla_skin.cpp.o \
	$(OBJDIR)/carla_shared.cpp.o \
	$(OBJDIR)/carla_widgets.cpp.o \
	$(OBJDIR)/patchcanvas/theme.cpp.o \
	$(OBJDIR)/moc_carla_database.cpp.o \
	$(OBJDIR)/moc_carla_host.cpp.o \
	$(OBJDIR)/moc_carla_settings.cpp.o \
	$(OBJDIR)/moc_carla_widgets.cpp.o \
	$(OBJDIR)/resources_rc.cpp.o \
	$(OBJDIR)/CarlaStandalone.cpp.o \
	$(OBJDIR)/CarlaStandaloneNSM.cpp.o \
	$(OBJDIR)/Information.cpp.o

# ---------------------------------------------------------------------------------------------------------------------

TARGETS = $(RES) $(UIs)

ifeq ($(CPPMODE),true)
TARGETS += $(BINDIR)/carla
TARGETS += $(BINDIR)/carla-database
TARGETS += $(BINDIR)/carla-settings
endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/carla: $(OBJS) $(OBJDIR)/carla.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking carla"
	$(CXX) $^ $(LIBS_START) $(LIBS) $(LIBS_END) $(LINK_FLAGS) -o $@

$(BINDIR)/carla-%: $(OBJS) $(OBJDIR)/carla_%_app.cpp.o
	-@mkdir -p $(BINDIR)
	@echo "Linking carla_$*"
	$(CXX) $^ $(LIBS_START) $(LIBS) $(LIBS_END) $(LINK_FLAGS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

moc_%.cpp: %.hpp
	$(MOC_QT5) $< -o $@

ui_%.hpp: $(RESDIR)/ui/%.ui
	$(UIC_QT5) $< -o $@

ui_%.py: $(RESDIR)/ui/%.ui
	$(PYUIC) $< -o $@

resources_rc.cpp: $(RESDIR)/resources.qrc $(RESDIR)/*/*.png $(RESDIR)/*/*.svg $(RESDIR)/*/*.svgz
	$(RCC_QT5) $< -o $@

resources_rc.py: $(RESDIR)/resources.qrc $(RESDIR)/*/*.png $(RESDIR)/*/*.svg $(RESDIR)/*/*.svgz
	$(PYRCC) $< -o $@

$(BINDIR)/resources/%: %
	-@mkdir -p $(BINDIR)/resources
	$(LINK) $(CURDIR)/$* $(BINDIR)/resources/

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.cpp.o: %.cpp $(UIs)
	-@mkdir -p $(OBJDIR)/patchcanvas
	@echo "Compiling $<"
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(OBJDIR)/Information.cpp.o: $(CWD)/backend/utils/Information.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(OBJDIR)/CarlaStandalone.cpp.o: $(CWD)/backend/CarlaStandalone.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(OBJDIR)/CarlaStandaloneNSM.cpp.o: $(CWD)/backend/CarlaStandaloneNSM.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -rf $(UIs) $(RES) __pycache__ *.pyc

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJS:%.o=%.d)
-include $(OBJDIR)/carla.cpp.d
-include $(OBJDIR)/carla_database_app.cpp.d
-include $(OBJDIR)/carla_settings_app.cpp.d

# ---------------------------------------------------------------------------------------------------------------------
