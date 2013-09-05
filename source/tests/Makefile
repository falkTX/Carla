#!/usr/bin/make -f
# Makefile for carla tests #
# ------------------------ #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../backend -I../includes -I../modules -I../modules/distrho -I../modules/theme -I../modules/widgets -I../utils
BUILD_CXX_FLAGS += -DWANT_NATIVE -DWANT_LADSPA -DWANT_DSSI -DWANT_LV2 -DWANT_VST -DWANT_FLUIDSYNTH -DWANT_LINUXSAMPLER
BUILD_CXX_FLAGS += -DWANT_RTAUDIO -DWANT_OPENGL -DWANT_AUDIOFILE -DWANT_MIDIFILE -DWANT_ZYNADDSUBFX -DWANT_ZYNADDSUBFX_UI
BUILD_CXX_FLAGS += -isystem /usr/include/qt4
BUILD_CXX_FLAGS += -isystem /opt/kxstudio/include
# BUILD_CXX_FLAGS += -isystem ../backend/engine/rtaudio-4.0.11
# BUILD_CXX_FLAGS += -I/opt/mingw32/include

ANSI_CXX_FLAGS   = -DBUILD_ANSI_TEST -DVESTIGE_HEADER
ANSI_CXX_FLAGS  += -ansi -pedantic -pedantic-errors -Wunused-parameter -Wuninitialized -Wno-vla
ANSI_CXX_FLAGS  += -std=c++11 -Wzero-as-null-pointer-constant
ANSI_CXX_FLAGS  += -Wcast-qual -Wconversion -Wsign-conversion -Wlogical-op -Waggregate-return

ifeq ($(MACOS),true)
DGL_LIBS = -framework OpenGL -framework Cocoa
else
ifeq ($(WIN32),true)
DGL_LIBS = -lQtCore -lopengl32 -lgdi32 -lole32 -luuid -lws2_32
else
DGL_LIBS = -lQtCore -lGL -lX11
endif
endif

ifeq ($(MACOS),true)
TARGETS = CarlaString DGL1 DGL2 Print
else
TARGETS = ANSI CarlaString DGL1 DGL2 Print RtList Utils Widgets
endif

all: $(TARGETS) RUN

# --------------------------------------------------------------

ANSI: ANSI.cpp #../backend/standalone/CarlaStandalone.cpp.o CarlaBackendDummy.cpp.o ../modules/theme.a
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(ANSI_CXX_FLAGS) $(LINK_FLAGS) -L/opt/kxstudio/lib -lQtCore -lQtGui -llo -o $@

CarlaString: CarlaString.cpp ../modules/utils/CarlaString.hpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@

DGL1: DGL1.cpp DGL1_Artwork.cpp ../modules/dgl.a
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) -o $@

DGL2: DGL2.cpp NekoArtwork.cpp ../modules/dgl.a
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) -o $@ && $(STRIP) $@

RtList: RtList.cpp ../modules/utils/RtList.hpp ../modules/rtmempool.a
	$(CXX) RtList.cpp ../modules/rtmempool.a $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -pthread -lpthread -o $@

Print: Print.cpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@

Utils: Utils.cpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -pthread -lpthread -o $@

Widgets: Widgets.cpp resources.cpp.o ../modules/theme.a ../modules/widgets.a
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -lQtCore -lQtGui -o $@

RUN: $(TARGETS)
	./ANSI && ./CarlaString && ./RtList && ./Print && ./Utils
	./DGL1 && ./DGL2

# --------------------------------------------------------------

%.cpp.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) $(ANSI_CXX_FLAGS) -c -o $@

resources.cpp: ../../resources/resources.qrc
	$(RCC) $< -o resources.cpp

../modules/%.a:
	$(MAKE) -C ../modules $*

# --------------------------------------------------------------

clean:
	rm -f *.o resources.cpp $(TARGETS)

debug:
	$(MAKE) DEBUG=true
