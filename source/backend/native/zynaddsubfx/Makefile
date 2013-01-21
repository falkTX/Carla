#!/usr/bin/make -f
# Makefile for carla-native plugins #
# ----------------------------------------------- #
# Created by falkTX
#

include ../../Makefile.mk

WANT_GUI = $(shell pkg-config --exists ntk ntk_images && echo true)

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -fvisibility=hidden -fPIC -I.
BUILD_CXX_FLAGS += -DASM_F2I_YES -DVERSION=\"2.4.3\"
# BUILD_CXX_FLAGS += -DIN_DEFAULT=\"\" -DOUT_DEFAULT=\"\"
BUILD_CXX_FLAGS += -DIN_DEFAULT=\"JACK\" -DOUT_DEFAULT=\"JACK\" -DJACK=1
BUILD_CXX_FLAGS += $(shell pkg-config --cflags fftw3 mxml jack)
LINK_FLAGS      += $(shell pkg-config --libs fftw3 mxml jack)

OBJS = \
	DSP/AnalogFilter.o \
	DSP/FFTwrapper.o \
	DSP/Filter.o \
	DSP/FormantFilter.o \
	DSP/SVFilter.o \
	DSP/Unison.o \
	Effects/Alienwah.o \
	Effects/Chorus.o \
	Effects/Distorsion.o \
	Effects/DynamicFilter.o \
	Effects/Echo.o \
	Effects/Effect.o \
	Effects/EffectLFO.o \
	Effects/EffectMgr.o \
	Effects/EQ.o \
	Effects/Phaser.o \
	Effects/Reverb.o \
	Misc/Bank.o \
	Misc/Config.o \
	Misc/Dump.o \
	Misc/Master.o \
	Misc/Microtonal.o \
	Misc/Part.o \
	Misc/Recorder.o \
	Misc/Util.o \
	Misc/WavFile.o \
	Misc/WaveShapeSmps.o \
	Misc/XMLwrapper.o \
	Nio/AudioOut.o \
	Nio/Engine.o \
	Nio/EngineMgr.o \
	Nio/Nio.o \
	Nio/NulEngine.o \
	Nio/InMgr.o \
	Nio/OutMgr.o \
	Nio/WavEngine.o \
	Params/ADnoteParameters.o \
	Params/Controller.o \
	Params/EnvelopeParams.o \
	Params/FilterParams.o \
	Params/LFOParams.o \
	Params/PADnoteParameters.o \
	Params/Presets.o \
	Params/PresetsArray.o \
	Params/PresetsStore.o \
	Params/SUBnoteParameters.o \
	Synth/ADnote.o \
	Synth/Envelope.o \
	Synth/LFO.o \
	Synth/OscilGen.o \
	Synth/PADnote.o \
	Synth/Resonance.o \
	Synth/SUBnote.o \
	Synth/SynthNote.o
# 	Misc/Stereo.o
# 	Nio/MidiIn.o

OBJS += Nio/JackEngine.o

# ifeq ($(WANT_GUI),true)
# BUILD_CXX_FLAGS += -DFLTK_GUI=1
BUILD_CXX_FLAGS += -DNTK_GUI=1
# BUILD_CXX_FLAGS += -DVSTAUDIOOUT=1 # TESTING
BUILD_CXX_FLAGS += -DPIXMAP_PATH=\"/usr/share/zynaddsubfx/pixmaps/\"
BUILD_CXX_FLAGS += -DSOURCE_DIR=\"/usr/share/zynaddsubfx/pixmaps/nothing-here\"

# BUILD_CXX_FLAGS += $(shell fltk-config --use-images --cxxflags)
# LINK_FLAGS      += $(shell fltk-config --use-images --ldflags)

BUILD_CXX_FLAGS += $(shell pkg-config --cflags ntk ntk_images)
LINK_FLAGS      += $(shell pkg-config --libs ntk ntk_images)

FILES = \
	UI/ADnoteUI.cc \
	UI/BankUI.cc \
	UI/ConfigUI.cc \
	UI/EffUI.cc \
	UI/EnvelopeUI.cc \
	UI/FilterUI.cc \
	UI/LFOUI.cc \
	UI/MasterUI.cc \
	UI/MicrotonalUI.cc \
	UI/OscilGenUI.cc \
	UI/PADnoteUI.cc \
	UI/PartUI.cc \
	UI/PresetsUI.cc \
	UI/ResonanceUI.cc \
	UI/SUBnoteUI.cc \
	UI/VirKeyboard.cc

HEADERS = \
	UI/ADnoteUI.h \
	UI/BankUI.h \
	UI/ConfigUI.h \
	UI/EffUI.h \
	UI/EnvelopeUI.h \
	UI/FilterUI.h \
	UI/LFOUI.h \
	UI/MasterUI.h \
	UI/MicrotonalUI.h \
	UI/OscilGenUI.h \
	UI/PADnoteUI.h \
	UI/PartUI.h \
	UI/PresetsUI.h \
	UI/ResonanceUI.h \
	UI/SUBnoteUI.h \
	UI/VirKeyboard.h

OBJS += \
	UI/ADnoteUI.o \
	UI/BankUI.o \
	UI/ConfigUI.o \
	UI/EffUI.o \
	UI/EnvelopeUI.o \
	UI/FilterUI.o \
	UI/LFOUI.o \
	UI/MasterUI.o \
	UI/MicrotonalUI.o \
	UI/OscilGenUI.o \
	UI/PADnoteUI.o \
	UI/PartUI.o \
	UI/PresetsUI.o \
	UI/ResonanceUI.o \
	UI/SUBnoteUI.o \
	UI/VirKeyboard.o \
	UI/WidgetPDial.o \
	UI/NioUI.o
# endif

TARGET = ../zynaddsubfx.a

# --------------------------------------------------------------

all: $(TARGET)

$(TARGET): $(FILES) $(OBJS)
	$(AR) rs $@ $(OBJS)

test: $(FILES) $(OBJS) main.o
	$(CXX) $(OBJS) main.o $(LINK_FLAGS) -o $@

# --------------------------------------------------------------

%.cc: %.fl
	# FIXME
	cd UI; ntk-fluid -c ../$<; cd ..
# 	cd UI; fluid -c ../$<; cd ..

.cc.o:
	$(CXX) -c $< $(BUILD_CXX_FLAGS) -o $@

.cpp.o:
	$(CXX) -c $< $(BUILD_CXX_FLAGS) -o $@

clean:
	rm -f $(TARGET) $(FILES) $(HEADERS) $(OBJS) main.o
