#!/usr/bin/make -f
# Makefile for carla-frontend #
# --------------------------- #
# Created by falkTX
#

CWD=../..
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
BUILD_CXX_FLAGS += $(FLUIDSYNTH_FLAGS)
BUILD_CXX_FLAGS += $(QT5_FLAGS)

# ---------------------------------------------------------------------------------------------------------------------

OBJS = \
	$(OBJDIR)/carla.cpp.o \
	$(OBJDIR)/app.cpp.o \
	$(OBJDIR)/host.cpp.o \
	$(OBJDIR)/moc_host.cpp.o \
	$(OBJDIR)/Information.cpp.o \
	$(OBJDIR)/CarlaStandalone.cpp.o \
	$(OBJDIR)/CarlaStandaloneNSM.cpp.o

TARGETS = $(BINDIR)/carla

# ---------------------------------------------------------------------------------------------------------------------
# UI code

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

# ---------------------------------------------------------------------------------------------------------------------
# Resources

RES = \
	resources_rc.hpp

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS) $(UIs) $(RES)

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJS) $(UIs) $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

moc_%.cpp: %.hpp
	$(MOC_QT5) $< -o $@

ui_%.hpp: $(RESDIR)/ui/%.ui
	$(UIC_QT5) $< -o $@

resources_rc.hpp: $(RESDIR)/resources.qrc $(RESDIR)/*/*.png $(RESDIR)/*/*.svg
	$(RCC_QT5) $< -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/carla: $(OBJS)
	-@mkdir -p $(BINDIR)
	@echo "Linking carla"
	$(CXX) $(OBJS) $(LIBS_START) $(LIBS) $(LIBS_END) $(LINK_FLAGS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.cpp.o: %.cpp $(UIs) $(RES)
	-@mkdir -p $(OBJDIR)
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

-include $(OBJS:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
