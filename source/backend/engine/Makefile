#!/usr/bin/make -f
# Makefile for carla-engine #
# ------------------------- #
# Created by falkTX
#

CWD=../..
include ../Makefile.mk

# Workaround GCC bug
ifeq ($(TESTBUILD),true)
ifeq ($(USING_JUCE_AUDIO_DEVICES),true)
BUILD_CXX_FLAGS += -Wno-undef
endif
endif

ifeq ($(USING_JUCE),true)
ifeq ($(WIN32),true)
BUILD_CXX_FLAGS += -I$(CWD)/modules/mingw-std-threads
endif
endif

BUILD_CXX_FLAGS += $(MAGIC_FLAGS)

# ---------------------------------------------------------------------------------------------------------------------

OBJS = \
	$(OBJDIR)/CarlaEngine.cpp.o \
	$(OBJDIR)/CarlaEngineClient.cpp.o \
	$(OBJDIR)/CarlaEngineData.cpp.o \
	$(OBJDIR)/CarlaEngineDummy.cpp.o \
	$(OBJDIR)/CarlaEngineGraph.cpp.o \
	$(OBJDIR)/CarlaEngineInternal.cpp.o \
	$(OBJDIR)/CarlaEnginePorts.cpp.o \
	$(OBJDIR)/CarlaEngineThread.cpp.o

ifeq ($(HAVE_LIBLO),true)
OBJS += \
	$(OBJDIR)/CarlaEngineOsc.cpp.o \
	$(OBJDIR)/CarlaEngineOscHandlers.cpp.o \
	$(OBJDIR)/CarlaEngineOscSend.cpp.o
endif

OBJSa = $(OBJS) \
	$(OBJDIR)/CarlaEngineJack.cpp.o \
	$(OBJDIR)/CarlaEngineNative.cpp.o

ifeq ($(USING_JUCE_AUDIO_DEVICES),true)
OBJSa += \
	$(OBJDIR)/CarlaEngineJuce.cpp.o
endif

ifeq ($(USING_RTAUDIO),true)
OBJSa += \
	$(OBJDIR)/CarlaEngineRtAudio.cpp.o
endif

OBJSp = $(OBJS) \
	$(OBJDIR)/CarlaEngineNative.cpp.exp.o

TARGETS = \
	$(MODULEDIR)/carla_engine.a \
	$(MODULEDIR)/carla_engine_plugin.a

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJSa) $(OBJSp) $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/carla_engine.a: $(OBJSa)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating carla_engine.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(MODULEDIR)/carla_engine_plugin.a: $(OBJSp)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating carla_engine_plugin.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/CarlaEngineRtAudio.cpp.o: CarlaEngineRtAudio.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaEngineRtAudio.cpp"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(RTAUDIO_FLAGS) $(RTMIDI_FLAGS) -c -o $@

ifeq ($(MACOS),true)
$(OBJDIR)/CarlaEngineNative.cpp.exp.o: CarlaEngineNative.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaEngineNative.cpp (plugin)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_BUILD -ObjC++ -c -o $@
else
$(OBJDIR)/CarlaEngineNative.cpp.exp.o: CarlaEngineNative.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaEngineNative.cpp (plugin)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_BUILD -c -o $@
endif

ifeq ($(JACKBRIDGE_DIRECT),true)
$(OBJDIR)/CarlaEngineJack.cpp.o: CarlaEngineJack.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaEngineJack.cpp"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(JACK_FLAGS) -c -o $@
endif

ifeq ($(USING_JUCE),true)
$(OBJDIR)/CarlaEngineJuce.cpp.o: CarlaEngineJuce.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -std=gnu++14 -c -o $@
endif

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

-include $(OBJSa:%.o=%.d)
-include $(OBJDIR)/CarlaEngineNative.cpp.exp.d

# ---------------------------------------------------------------------------------------------------------------------
