#!/usr/bin/make -f
# Makefile for carla-engine #
# ------------------------- #
# Created by falkTX
#

CWD=../..
include ../Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

OBJS = \
	$(OBJDIR)/CarlaEngine.cpp.o \
	$(OBJDIR)/CarlaEngineClient.cpp.o \
	$(OBJDIR)/CarlaEngineData.cpp.o \
	$(OBJDIR)/CarlaEngineGraph.cpp.o \
	$(OBJDIR)/CarlaEngineInternal.cpp.o \
	$(OBJDIR)/CarlaEngineOsc.cpp.o \
	$(OBJDIR)/CarlaEngineOscSend.cpp.o \
	$(OBJDIR)/CarlaEnginePorts.cpp.o \
	$(OBJDIR)/CarlaEngineThread.cpp.o

OBJSa = $(OBJS) \
	$(OBJDIR)/CarlaEngineJack.cpp.o \
	$(OBJDIR)/CarlaEngineNative.cpp.o

ifeq ($(MACOS_OR_WIN32),true)
OBJSa += \
	$(OBJDIR)/CarlaEngineJuce.cpp.o
else
OBJSa += \
	$(OBJDIR)/CarlaEngineRtAudio.cpp.o
endif

OBJSp = $(OBJS) \
	$(OBJDIR)/CarlaEngineNative.cpp.exp.o

TARGETS = \
	$(MODULEDIR)/carla_engine.a \
	$(MODULEDIR)/carla_engine_plugin.a

# ----------------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

# ----------------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJSa) $(OBJSp) $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/carla_engine.a: $(OBJSa)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating carla_engine.a"
	@rm -f $@
	@$(AR) crs $@ $^

$(MODULEDIR)/carla_engine_plugin.a: $(OBJSp)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating carla_engine_plugin.a"
	@rm -f $@
	@$(AR) crs $@ $^

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/CarlaEngineRtAudio.cpp.o: CarlaEngineRtAudio.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaEngineRtAudio.cpp"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(RTAUDIO_FLAGS) $(RTMIDI_FLAGS) -MMD -c -o $@

$(OBJDIR)/CarlaEngineNative.cpp.exp.o: CarlaEngineNative.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaEngineNative.cpp (plugin)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_EXPORT -MMD -c -o $@

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -MMD -c -o $@

-include $(OBJS:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
