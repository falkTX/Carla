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
	$(OBJDIR)/zynaddsubfx-src.cpp.o \
	$(OBJDIR)/zynaddsubfx-synth.cpp.o

ifeq ($(HAVE_ZYN_UI_DEPS),true)
TARGETS += ../resources/zynaddsubfx-ui$(APP_EXT)

ZYN_UI_FILES_CPP = \
	zynaddsubfx/UI/ADnoteUI.cpp \
	zynaddsubfx/UI/BankUI.cpp \
	zynaddsubfx/UI/ConfigUI.cpp \
	zynaddsubfx/UI/EffUI.cpp \
	zynaddsubfx/UI/EnvelopeUI.cpp \
	zynaddsubfx/UI/FilterUI.cpp \
	zynaddsubfx/UI/LFOUI.cpp \
	zynaddsubfx/UI/MasterUI.cpp \
	zynaddsubfx/UI/MicrotonalUI.cpp \
	zynaddsubfx/UI/OscilGenUI.cpp \
	zynaddsubfx/UI/PADnoteUI.cpp \
	zynaddsubfx/UI/PartUI.cpp \
	zynaddsubfx/UI/PresetsUI.cpp \
	zynaddsubfx/UI/ResonanceUI.cpp \
	zynaddsubfx/UI/SUBnoteUI.cpp \
	zynaddsubfx/UI/VirKeyboard.cpp

ZYN_UI_FILES_H = \
	zynaddsubfx/UI/ADnoteUI.h \
	zynaddsubfx/UI/BankUI.h \
	zynaddsubfx/UI/ConfigUI.h \
	zynaddsubfx/UI/EffUI.h \
	zynaddsubfx/UI/EnvelopeUI.h \
	zynaddsubfx/UI/FilterUI.h \
	zynaddsubfx/UI/LFOUI.h \
	zynaddsubfx/UI/MasterUI.h \
	zynaddsubfx/UI/MicrotonalUI.h \
	zynaddsubfx/UI/OscilGenUI.h \
	zynaddsubfx/UI/PADnoteUI.h \
	zynaddsubfx/UI/PartUI.h \
	zynaddsubfx/UI/PresetsUI.h \
	zynaddsubfx/UI/ResonanceUI.h \
	zynaddsubfx/UI/SUBnoteUI.h \
	zynaddsubfx/UI/VirKeyboard.h
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# Experimental plugins

ifeq ($(EXPERIMENTAL_PLUGINS),true)
OBJS_all += \
	$(OBJDIR)/zita-at1.cpp.o \
	$(OBJDIR)/zita-bls1.cpp.o \
	$(OBJDIR)/zita-rev1.cpp.o

TARGETS += \
	../resources/zita-at1-ui$(APP_EXT) \
	../resources/zita-bls1-ui$(APP_EXT) \
	../resources/zita-rev1-ui$(APP_EXT)
endif

# ---------------------------------------------------------------------------------------------------------------------

zynaddsubfx/UI/%.cpp: zynaddsubfx/UI/%.fl
	@echo "Generating $@|h"
	@$(FLUID) -c -o zynaddsubfx/UI/$*.cpp -h zynaddsubfx/UI/$*.h $<

zynaddsubfx/UI/%.h: zynaddsubfx/UI/%.fl
	@echo "Generating $@|cpp"
	@$(FLUID) -c -o zynaddsubfx/UI/$*.cpp -h zynaddsubfx/UI/$*.h $<

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/distrho-3bandeq.cpp.o: $(EXTERNAL_DIR)distrho-3bandeq.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(DPF_FLAGS) -DDISTRHO_NAMESPACE=d3BandEQ -Idistrho-3bandeq -c -o $@

$(OBJDIR)/distrho-3bandsplitter.cpp.o: distrho-3bandsplitter.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(DPF_FLAGS) -DDISTRHO_NAMESPACE=d3BandSplitter -Idistrho-3bandsplitter -c -o $@

$(OBJDIR)/distrho-kars.cpp.o: distrho-kars.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(DPF_FLAGS) -DDISTRHO_NAMESPACE=dKars -Idistrho-kars -c -o $@

$(OBJDIR)/distrho-nekobi.cpp.o: distrho-nekobi.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(DPF_FLAGS) -DDISTRHO_NAMESPACE=dNekobi -Idistrho-nekobi -w -c -o $@

$(OBJDIR)/distrho-pingpongpan.cpp.o: distrho-pingpongpan.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(DPF_FLAGS) -DDISTRHO_NAMESPACE=dPingPongPan -Idistrho-pingpongpan -c -o $@

$(OBJDIR)/distrho-prom.cpp.o: distrho-prom.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(DPF_FLAGS) $(PROJECTM_FLAGS) -DDISTRHO_NAMESPACE=dProM -Idistrho-prom -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/distrho-vectorjuice.cpp.o: distrho-vectorjuice.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(DPF_FLAGS) -DDISTRHO_NAMESPACE=dVectorJuice -Idistrho-vectorjuice -c -o $@

$(OBJDIR)/distrho-wobblejuice.cpp.o: distrho-wobblejuice.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(DPF_FLAGS) -DDISTRHO_NAMESPACE=dWobbleJuice -Idistrho-wobblejuice -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/zynaddsubfx-fx.cpp.o: zynaddsubfx-fx.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(ZYN_DSP_FLAGS) -c -o $@

$(OBJDIR)/zynaddsubfx-synth.cpp.o: zynaddsubfx-synth.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(ZYN_DSP_FLAGS) -Wno-unused-parameter -c -o $@

$(OBJDIR)/zynaddsubfx-src.cpp.o: zynaddsubfx-src.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(ZYN_DSP_FLAGS) -Wno-unused-parameter -Wno-unused-variable -c -o $@

$(OBJDIR)/zynaddsubfx-ui.cpp.o: zynaddsubfx-ui.cpp $(ZYN_UI_FILES_H) $(ZYN_UI_FILES_CPP)
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(ZYN_UI_FLAGS) -Wno-unused-parameter -Wno-unused-variable -c -o $@

../resources/zynaddsubfx-ui$(APP_EXT): $(OBJDIR)/zynaddsubfx-ui.cpp.o
	-@mkdir -p ../resources
	@echo "Linking zynaddsubfx-ui"
	@$(CXX) $^ $(LINK_FLAGS) $(ZYN_UI_LIBS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/zita-%-ui.cpp.o: zita-%-ui.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(ZITA_UI_FLAGS) -c -o $@

$(OBJDIR)/zita-%.cpp.o: zita-%.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(ZITA_DSP_FLAGS) -c -o $@

../resources/zita-%-ui$(APP_EXT): $(OBJDIR)/zita-%-ui.cpp.o
	-@mkdir -p ../resources
	@echo "Linking $<"
	@$(CXX) $^ $(LINK_FLAGS) $(ZITA_UI_LIBS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJDIR)/zita-at1-ui.cpp.d
-include $(OBJDIR)/zita-bls1-ui.cpp.d
-include $(OBJDIR)/zita-rev1-ui.cpp.d
-include $(OBJDIR)/zynaddsubfx-ui.cpp.d

# ---------------------------------------------------------------------------------------------------------------------
