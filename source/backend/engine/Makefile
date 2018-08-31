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
	$(OBJDIR)/CarlaEnginePorts.cpp.o \
	$(OBJDIR)/CarlaEngineThread.cpp.o

ifeq ($(HAVE_LIBLO),true)
OBJS += \
	$(OBJDIR)/CarlaEngineOsc.cpp.o \
	$(OBJDIR)/CarlaEngineOscSend.cpp.o
endif

OBJSa = $(OBJS) \
	$(OBJDIR)/CarlaEngineJack.cpp.o \
	$(OBJDIR)/CarlaEngineNative.cpp.o \
	$(OBJDIR)/CarlaEngineRtAudio.cpp.o

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
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(RTAUDIO_FLAGS) $(RTMIDI_FLAGS) -c -o $@

ifeq ($(MACOS),true)
$(OBJDIR)/CarlaEngineNative.cpp.exp.o: CarlaEngineNative.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaEngineNative.cpp (plugin)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_EXPORT -ObjC++ -c -o $@
else
$(OBJDIR)/CarlaEngineNative.cpp.exp.o: CarlaEngineNative.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling CarlaEngineNative.cpp (plugin)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_PLUGIN_EXPORT -c -o $@
endif

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

-include $(OBJSa:%.o=%.d)
-include $(OBJDIR)/CarlaEngineNative.cpp.exp.d

# ----------------------------------------------------------------------------------------------------------------------------
