#!/usr/bin/make -f
# Makefile for carla tests #
# ------------------------ #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../backend -I../includes -I../modules -I../modules/distrho -I../utils
BUILD_CXX_FLAGS += -DWANT_NATIVE -DWANT_LADSPA -DWANT_DSSI -DWANT_LV2 -DWANT_VST -DWANT_AU -DWANT_CSOUND -DWANT_FLUIDSYNTH -DWANT_LINUXSAMPLER
BUILD_CXX_FLAGS += -DWANT_OPENGL -DWANT_AUDIOFILE -DWANT_MIDIFILE -DWANT_ZYNADDSUBFX -DWANT_ZYNADDSUBFX_UI
BUILD_CXX_FLAGS += -isystem /usr/include/qt5
BUILD_CXX_FLAGS += -isystem /opt/kxstudio/include
# BUILD_CXX_FLAGS += -isystem ../backend/engine/rtaudio-4.0.11
# BUILD_CXX_FLAGS += -I/opt/mingw32/include

ANSI_CXX_FLAGS   = -DBUILD_ANSI_TEST -DVESTIGE_HEADER
ANSI_CXX_FLAGS  += -ansi -pedantic -pedantic-errors -Wunused-parameter -Wuninitialized -Wno-vla
# ANSI_CXX_FLAGS  += -std=c++11 -Wzero-as-null-pointer-constant
ANSI_CXX_FLAGS  += -Wcast-qual -Wconversion -Wsign-conversion -Wlogical-op -Waggregate-return

ifeq ($(MACOS),true)
DGL_LIBS = -framework OpenGL -framework Cocoa
else
ifeq ($(WIN32),true)
DGL_LIBS = -lopengl32 -lgdi32
else
DGL_LIBS = -lGL -lX11
endif
endif

TARGETS = CarlaString Print RtList Utils

all: $(TARGETS)

# --------------------------------------------------------------

ANSI: ANSI.cpp #../backend/standalone/CarlaStandalone.cpp.o CarlaBackendDummy.cpp.o ../modules/theme.a
	$(CXX) $< $(BUILD_CXX_FLAGS) $(ANSI_CXX_FLAGS) $(LINK_FLAGS) -L/opt/kxstudio/lib -lQtCore -lQtGui -llo -o $@

CarlaString: CarlaString.cpp ../utils/CarlaString.hpp
	$(CXX) $< $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@
# 	valgrind ./CarlaString

DGL: DGL.cpp ../modules/distrho/dgl/src/Window.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) -o $@
# 	./DGL
# 	valgrind ./DGL

Print: Print.cpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@
# 	valgrind ./Print

RtList: RtList.cpp ../utils/RtList.hpp ../modules/rtmempool.a
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -lpthread -o $@
# 	valgrind ./RtList

Utils: Utils.cpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@
# 	valgrind ./Utils

# --------------------------------------------------------------

%.cpp.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) $(ANSI_CXX_FLAGS) -c -o $@

../modules/%.a:
	$(MAKE) -C ../modules $*

# --------------------------------------------------------------

clean:
	rm -f *.o $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
