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
BUILD_CXX_FLAGS += -std=c++11 -Wzero-as-null-pointer-constant
BUILD_CXX_FLAGS += -isystem /opt/kxstudio/include
# BUILD_CXX_FLAGS += -isystem /usr/include/qt5
# BUILD_CXX_FLAGS += -I/opt/mingw32/include

# --------------------------------------------------------------

# ANSI_CXX_FLAGS  += -ansi -pedantic -pedantic-errors -Wunused-parameter -Wuninitialized -Wno-vla
# ANSI_CXX_FLAGS  += -Wcast-qual -Wconversion -Wsign-conversion -Wlogical-op -Waggregate-return
# ANSI_CXX_FLAGS  += -std=c++11 -Wzero-as-null-pointer-constant

# --------------------------------------------------------------

ANSI_FLAGS  = -DBUILD_ANSI_TEST -I../backend -I../includes
ANSI_FLAGS += -Wall -Wextra -Werror
BASE_FLAGS += -DDEBUG -O0 -g
BASE_FLAGS += -ansi -pedantic -pedantic-errors
BASE_FLAGS += -Wunused-parameter -Wuninitialized -Wcast-qual -Wconversion -Wsign-conversion -Wlogical-op -Waggregate-return

# --------------------------------------------------------------

TARGETS  = ansi-pedantic-test_c ansi-pedantic-test_c99 ansi-pedantic-test_cxx ansi-pedantic-test_cxx11
# TARGETS += CarlaString Print RtList Utils

all: $(TARGETS)

# --------------------------------------------------------------

ansi-pedantic-test_c: ansi-pedantic-test.c ../backend/Carla*.h
	$(CC) $< $(ANSI_FLAGS) -Wc++-compat -o $@

ansi-pedantic-test_c99: ansi-pedantic-test.c ../backend/Carla*.h
	$(CC) $< $(ANSI_FLAGS) -std=c99 -Wc++-compat -o $@

ansi-pedantic-test_cxx: ansi-pedantic-test.c ../backend/Carla*.h
	$(CXX) $< $(ANSI_FLAGS) -o $@

ansi-pedantic-test_cxx11: ansi-pedantic-test.c ../backend/Carla*.h
	$(CXX) $< $(ANSI_FLAGS) -std=c++11 -Wzero-as-null-pointer-constant -o $@

# --------------------------------------------------------------

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
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(ANSI_CXX_FLAGS) -std=c++11 -Wzero-as-null-pointer-constant $(LINK_FLAGS) -ldl -lpthread -o $@
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
