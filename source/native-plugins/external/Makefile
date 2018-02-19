#!/usr/bin/make -f
# Makefile for native-plugins #
# --------------------------- #
# Created by falkTX
#

# NOTE: this file assumes Makefile.mk has been included before

# ---------------------------------------------------------------------------------------------------------------------
# DPF plugins

OBJS_all += \
	$(OBJDIR)/distrho-3bandeq.cpp.o \
	$(OBJDIR)/distrho-3bandsplitter.cpp.o \
	$(OBJDIR)/distrho-kars.cpp.o \
	$(OBJDIR)/distrho-nekobi.cpp.o \
	$(OBJDIR)/distrho-pingpongpan.cpp.o

ifeq ($(HAVE_DGL),true)
ifeq ($(HAVE_PROJECTM),true)
OBJS += $(OBJDIR)/distrho-prom.cpp.o
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# DPF plugins (Juice)

OBJS_all += \
	$(OBJDIR)/distrho-vectorjuice.cpp.o \
	$(OBJDIR)/distrho-wobblejuice.cpp.o

# ---------------------------------------------------------------------------------------------------------------------
# ZynAddSubFX

ifeq ($(HAVE_ZYN_DEPS),true)
OBJS_all += \
	$(OBJDIR)/zynaddsubfx-fx.cpp.o \
	$(OBJDIR)/zynaddsubfx-src.cpp.o

ifneq ($(SKIP_ZYN_SYNTH),true)
OBJS_all += \
	$(OBJDIR)/zynaddsubfx-synth.cpp.o
endif

ifeq ($(HAVE_ZYN_UI_DEPS),true)
TARGETS += resources/zynaddsubfx-ui$(APP_EXT)

ZYN_UI_FILES_CPP = \
	external/zynaddsubfx/UI/ADnoteUI.cpp \
	external/zynaddsubfx/UI/BankUI.cpp \
	external/zynaddsubfx/UI/ConfigUI.cpp \
	external/zynaddsubfx/UI/EffUI.cpp \
	external/zynaddsubfx/UI/EnvelopeUI.cpp \
	external/zynaddsubfx/UI/FilterUI.cpp \
	external/zynaddsubfx/UI/LFOUI.cpp \
	external/zynaddsubfx/UI/MasterUI.cpp \
	external/zynaddsubfx/UI/MicrotonalUI.cpp \
	external/zynaddsubfx/UI/OscilGenUI.cpp \
	external/zynaddsubfx/UI/PADnoteUI.cpp \
	external/zynaddsubfx/UI/PartUI.cpp \
	external/zynaddsubfx/UI/PresetsUI.cpp \
	external/zynaddsubfx/UI/ResonanceUI.cpp \
	external/zynaddsubfx/UI/SUBnoteUI.cpp \
	external/zynaddsubfx/UI/VirKeyboard.cpp

ZYN_UI_FILES_H = \
	external/zynaddsubfx/UI/ADnoteUI.h \
	external/zynaddsubfx/UI/BankUI.h \
	external/zynaddsubfx/UI/ConfigUI.h \
	external/zynaddsubfx/UI/EffUI.h \
	external/zynaddsubfx/UI/EnvelopeUI.h \
	external/zynaddsubfx/UI/FilterUI.h \
	external/zynaddsubfx/UI/LFOUI.h \
	external/zynaddsubfx/UI/MasterUI.h \
	external/zynaddsubfx/UI/MicrotonalUI.h \
	external/zynaddsubfx/UI/OscilGenUI.h \
	external/zynaddsubfx/UI/PADnoteUI.h \
	external/zynaddsubfx/UI/PartUI.h \
	external/zynaddsubfx/UI/PresetsUI.h \
	external/zynaddsubfx/UI/ResonanceUI.h \
	external/zynaddsubfx/UI/SUBnoteUI.h \
	external/zynaddsubfx/UI/VirKeyboard.h
endif
endif

# ---------------------------------------------------------------------------------------------------------------------

external/zynaddsubfx/UI/%.cpp: external/zynaddsubfx/UI/%.fl
	@echo "Generating $@|h"
	@$(FLUID) -c -o external/zynaddsubfx/UI/$*.cpp -h external/zynaddsubfx/UI/$*.h $<

external/zynaddsubfx/UI/%.h: external/zynaddsubfx/UI/%.fl
	@echo "Generating $@|cpp"
	@$(FLUID) -c -o external/zynaddsubfx/UI/$*.cpp -h external/zynaddsubfx/UI/$*.h $<

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/distrho-3bandeq.cpp.o: external/distrho-3bandeq.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(DPF_FLAGS) -DDISTRHO_NAMESPACE=d3BandEQ -Idistrho-3bandeq -c -o $@

$(OBJDIR)/distrho-3bandsplitter.cpp.o: external/distrho-3bandsplitter.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(DPF_FLAGS) -DDISTRHO_NAMESPACE=d3BandSplitter -Idistrho-3bandsplitter -c -o $@

$(OBJDIR)/distrho-kars.cpp.o: external/distrho-kars.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(DPF_FLAGS) -DDISTRHO_NAMESPACE=dKars -Idistrho-kars -c -o $@

$(OBJDIR)/distrho-nekobi.cpp.o: external/distrho-nekobi.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(DPF_FLAGS) -DDISTRHO_NAMESPACE=dNekobi -Idistrho-nekobi -w -c -o $@

$(OBJDIR)/distrho-pingpongpan.cpp.o: external/distrho-pingpongpan.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(DPF_FLAGS) -DDISTRHO_NAMESPACE=dPingPongPan -Idistrho-pingpongpan -c -o $@

$(OBJDIR)/distrho-prom.cpp.o: external/distrho-prom.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(DPF_FLAGS) $(PROJECTM_FLAGS) -DDISTRHO_NAMESPACE=dProM -Idistrho-prom -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/distrho-vectorjuice.cpp.o: external/distrho-vectorjuice.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(DPF_FLAGS) -DDISTRHO_NAMESPACE=dVectorJuice -Idistrho-vectorjuice -c -o $@

$(OBJDIR)/distrho-wobblejuice.cpp.o: external/distrho-wobblejuice.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(DPF_FLAGS) -DDISTRHO_NAMESPACE=dWobbleJuice -Idistrho-wobblejuice -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/zynaddsubfx-fx.cpp.o: external/zynaddsubfx-fx.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(ZYN_DSP_FLAGS) -c -o $@

$(OBJDIR)/zynaddsubfx-synth.cpp.o: external/zynaddsubfx-synth.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(ZYN_DSP_FLAGS) -Wno-unused-parameter -c -o $@

$(OBJDIR)/zynaddsubfx-src.cpp.o: external/zynaddsubfx-src.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(ZYN_DSP_FLAGS) -Wno-unused-parameter -Wno-unused-variable -c -o $@

$(OBJDIR)/zynaddsubfx-ui.cpp.o: external/zynaddsubfx-ui.cpp $(ZYN_UI_FILES_H) $(ZYN_UI_FILES_CPP)
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(ZYN_UI_FLAGS) -Wno-unused-parameter -Wno-unused-variable -c -o $@

resources/zynaddsubfx-ui$(APP_EXT): $(OBJDIR)/zynaddsubfx-ui.cpp.o
	-@mkdir -p ../resources
	@echo "Linking zynaddsubfx-ui"
	@$(CXX) $^ $(LINK_FLAGS) $(ZYN_UI_LIBS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJDIR)/zynaddsubfx-ui.cpp.d

# ---------------------------------------------------------------------------------------------------------------------
