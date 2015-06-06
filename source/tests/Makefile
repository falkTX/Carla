#!/usr/bin/make -f
# Makefile for carla tests #
# ------------------------ #
# Created by falkTX
#

# --------------------------------------------------------------

CLANG   ?= clang
CXXLANG ?= clang++

MODULEDIR=../../build/modules/Debug

# --------------------------------------------------------------

BASE_FLAGS  = -Wall -Wextra -Werror -pipe -DBUILDING_CARLA -DREAL_BUILD -DDEBUG -O0 -g
BASE_FLAGS += -I. -I../backend -I../includes -I../modules -I../utils
BASE_FLAGS += -Wcast-align -Wmissing-declarations
# BASE_FLAGS += -Wcast-qual -Wconversion -Wsign-conversion -Wundef
BASE_FLAGS += -Wformat -Wformat-security -Wredundant-decls -Wshadow -Wstrict-overflow -fstrict-overflow -Wwrite-strings
BASE_FLAGS += -Wpointer-arith -Wabi -Winit-self -Wuninitialized -Wunused-parameter
# -Wfloat-equal
ifneq ($(CC),clang-3.4)
BASE_FLAGS += -Wlogical-op -Wunsafe-loop-optimizations
endif

# --------------------------------------------------------------

ANSI_FLAGS  = $(BASE_FLAGS) -DBUILD_ANSI_TEST
# ANSI_FLAGS += -ansi -pedantic -pedantic-errors -Waggregate-return
# ANSI_FLAGS += -L../backend -lcarla_standalone2

# --------------------------------------------------------------

GNU_CXX_FLAGS  = $(BASE_FLAGS)
GNU_CXX_FLAGS += -std=c++11 -std=gnu++11 -Weffc++ -Wnon-virtual-dtor -Woverloaded-virtual
ifneq ($(CC),clang-3.4)
GNU_CXX_FLAGS += -Wzero-as-null-pointer-constant
endif

# --------------------------------------------------------------

PEDANTIC_CXX_FLAGS  = $(BASE_FLAGS)
PEDANTIC_CXX_FLAGS += -std=c++11 -pedantic -pedantic-errors
PEDANTIC_CXX_FLAGS += -isystem /opt/kxstudio/include
# PEDANTIC_CXX_FLAGS += -Weffc++
PEDANTIC_CXX_FLAGS += -Wnon-virtual-dtor -Woverloaded-virtual -Wno-vla
ifeq ($(CC),clang-3.4)
PEDANTIC_CXX_FLAGS += -Wno-vla-extension
else
PEDANTIC_CXX_FLAGS += -Wzero-as-null-pointer-constant
endif

# --------------------------------------------------------------

# TARGETS  = ansi-pedantic-test_c_ansi
# TARGETS += ansi-pedantic-test_c89
# TARGETS += ansi-pedantic-test_c99
# TARGETS += ansi-pedantic-test_c11
# TARGETS += ansi-pedantic-test_clang
# TARGETS += ansi-pedantic-test_cxx_ansi
# TARGETS += ansi-pedantic-test_cxx03
# TARGETS += ansi-pedantic-test_cxx11
# TARGETS += ansi-pedantic-test_cxxlang
# TARGETS += CarlaPipeUtils
# TARGETS += CarlaRingBuffer
# TARGETS += CarlaString
TARGETS += CarlaUtils1
# ifneq ($(WIN32),true)
# TARGETS += CarlaUtils2
# endif
# TARGETS += CarlaUtils3
# TARGETS += CarlaUtils4
# TARGETS += Exceptions
# TARGETS += Print
# TARGETS += RDF

all: $(TARGETS)

# --------------------------------------------------------------

ansi-pedantic-test_c_ansi: ansi-pedantic-test.c ../backend/Carla*.h ../includes/*.h
	$(CC) $< $(ANSI_FLAGS) -ansi -pedantic -pedantic-errors -Wc++-compat -L../../bin -lcarla_standalone2 -o $@

ansi-pedantic-test_c89: ansi-pedantic-test.c ../backend/Carla*.h ../includes/*.h
	$(CC) $< $(ANSI_FLAGS) -std=c89 -pedantic -pedantic-errors -Wc++-compat -L../../bin -lcarla_standalone2 -o $@

ansi-pedantic-test_c99: ansi-pedantic-test.c ../backend/Carla*.h ../includes/*.h
	$(CC) $< $(ANSI_FLAGS) -std=c99 -pedantic -pedantic-errors -Wc++-compat -L../../bin -lcarla_standalone2 -o $@

ansi-pedantic-test_c11: ansi-pedantic-test.c ../backend/Carla*.h ../includes/*.h
	$(CC) $< $(ANSI_FLAGS) -std=c11 -pedantic -pedantic-errors -Wc++-compat -L../../bin -lcarla_standalone2 -o $@

ansi-pedantic-test_clang: ansi-pedantic-test.c ../backend/Carla*.h ../includes/*.h
	$(CLANG) $< $(ANSI_FLAGS) -ansi -pedantic -pedantic-errors -Wc++-compat -Weverything -Wno-padded -Wno-unknown-warning-option -L../../bin -lcarla_standalone2 -o $@

ansi-pedantic-test_cxx_ansi: ansi-pedantic-test.cpp ../backend/Carla*.h ../includes/*.h
	$(CXX) $< $(ANSI_FLAGS) -ansi -pedantic -pedantic-errors -L../../bin -lcarla_standalone2 -o $@

ansi-pedantic-test_cxx03: ansi-pedantic-test.cpp ../backend/Carla*.h ../includes/*.h
	$(CXX) $< $(ANSI_FLAGS) -std=c++03 -pedantic -pedantic-errors -L../../bin -lcarla_standalone2 -o $@

ansi-pedantic-test_cxx11: ansi-pedantic-test.cpp ../backend/Carla*.h ../includes/*.h
	$(CXX) $< $(ANSI_FLAGS) -std=c++11 -pedantic -pedantic-errors -L../../bin -lcarla_standalone2 -o $@

ansi-pedantic-test_cxxlang: ansi-pedantic-test.cpp ../backend/Carla*.h ../includes/*.h
	$(CXXLANG) $< $(ANSI_FLAGS) -ansi -pedantic -pedantic-errors -Weverything -Wno-padded -Wno-unknown-warning-option -L../../bin -lcarla_standalone2 -o $@

# --------------------------------------------------------------

CarlaRingBuffer: CarlaRingBuffer.cpp ../utils/CarlaRingBuffer.hpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -o $@
ifneq ($(WIN32),true)
	set -e; ./$@ && valgrind --leak-check=full ./$@
endif

CarlaString: CarlaString.cpp ../utils/CarlaString.hpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -o $@
ifneq ($(WIN32),true)
	set -e; ./$@ && valgrind --leak-check=full ./$@
endif

CarlaPipeUtils: CarlaPipeUtils.cpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -o $@ $(MODULEDIR)/juce_core.a -ldl -lpthread
ifneq ($(WIN32),true)
	set -e;  valgrind --leak-check=full ./$@
# 	./$@ &&
endif

CarlaPipeUtils.exe: CarlaPipeUtils.cpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -o $@ $(MODULEDIR)/juce_core.a -lole32 -lshlwapi -lversion -lwsock32 -lwininet -lwinmm -lws2_32 -lpthread

CarlaUtils1: CarlaUtils1.cpp ../utils/*.hpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -o $@
ifneq ($(WIN32),true)
	set -e; ./$@ && valgrind --leak-check=full ./$@
endif

CarlaUtils2: CarlaUtils2.cpp ../utils/*.hpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -o $@ \
		 $(MODULEDIR)/juce_core.a $(MODULEDIR)/lilv.a -ldl -lpthread -lrt
	set -e; ./$@ && valgrind --leak-check=full ./$@

CarlaUtils3: CarlaUtils3.cpp ../utils/*.hpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -o $@ -ldl -lrt
ifneq ($(WIN32),true)
	set -e; ./$@ && valgrind --leak-check=full ./$@
endif

CarlaUtils4: CarlaUtils4.cpp ../utils/CarlaStateUtils.cpp ../utils/*.hpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -o $@ \
		$(MODULEDIR)/juce_core.a -ldl -lpthread -lrt
ifneq ($(WIN32),true)
	set -e; ./$@ && valgrind --leak-check=full ./$@
endif

Exceptions: Exceptions.cpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -o $@
ifneq ($(WIN32),true)
	set -e; ./$@ && valgrind --leak-check=full ./$@
endif

Print: Print.cpp ../utils/CarlaUtils.hpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -o $@
ifneq ($(WIN32),true)
	set -e; ./$@ && valgrind --leak-check=full ./$@
endif

RDF: RDF.cpp ../modules/ladspa_rdf.hpp ../modules/lv2_rdf.hpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -o $@
ifneq ($(WIN32),true)
	set -e; ./$@ && valgrind --leak-check=full ./$@
endif

# --------------------------------------------------------------

sem: sem.cpp $(MODULEDIR)/jackbridge/*
	$(CXX) $< -Wall -Wextra -O0 -g -I../utils -I../includes -DREAL_BUILD -o $@
	./$@

# --------------------------------------------------------------

ChildProcess: ChildProcess.cpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) $(MODULEDIR)/juce_core.a -ldl -lpthread -lrt -o $@
	valgrind --leak-check=full ./$@

CarlaMutex: CarlaMutex.cpp ../utils/CarlaMutex.hpp
	$(CXX) $< -lpthread -o $@
	valgrind --leak-check=full ./$@

# ifneq ($(MACOS_OR_WIN32),true)
# 	 $(MODULEDIR)/rtaudio.a $(MODULEDIR)/rtmidi.a \
# endif

Engine: Engine.cpp
	$(CXX) $< \
	../backend/standalone/CarlaStandalone.cpp.o \
	-Wl,--start-group \
	../backend/carla_engine.a ../backend/carla_plugin.a $(MODULEDIR)/native-plugins.a \
	$(MODULEDIR)/dgl.a $(MODULEDIR)/jackbridge.a $(MODULEDIR)/lilv.a $(MODULEDIR)/rtmempool.a \
	-Wl,--end-group \
	$(PEDANTIC_CXX_FLAGS) $(shell pkg-config --libs alsa libpulse-simple liblo QtCore QtXml fluidsynth linuxsampler x11 gl smf fftw3 mxml zlib ntk_images ntk) -o $@
	env LD_LIBRARY_PATH=../backend valgrind --leak-check=full ./$@
# 	$(MODULEDIR)/juce_audio_basics.a $(MODULEDIR)/juce_core.a \

EngineEvents: EngineEvents.cpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -L../backend -lcarla_standalone2 -o $@
	env LD_LIBRARY_PATH=../backend valgrind ./$@

PipeServer: PipeServer.cpp ../utils/CarlaPipeUtils.hpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -lpthread -o $@
	valgrind --leak-check=full ./$@

RtLinkedList: RtLinkedList.cpp ../utils/LinkedList.hpp ../utils/RtLinkedList.hpp $(MODULEDIR)/rtmempool.a
	$(CXX) $< $(MODULEDIR)/rtmempool.a $(PEDANTIC_CXX_FLAGS) -lpthread -o $@
	valgrind --leak-check=full ./$@

RtLinkedListGnu: RtLinkedList.cpp ../utils/LinkedList.hpp ../utils/RtLinkedList.hpp $(MODULEDIR)/rtmempool.a
	$(CXX) $< $(MODULEDIR)/rtmempool.a $(GNU_CXX_FLAGS) -lpthread -o $@
	valgrind --leak-check=full ./$@

# --------------------------------------------------------------

clean:
	rm -f *.o $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

DISTRHO: DISTRHO.cpp ../modules/distrho/*.hpp ../modules/distrho/src/*.cpp
	$(CXX) $< ../modules/dgl.a $(PEDANTIC_CXX_FLAGS) -I../modules/distrho -I../modules/native-plugins/nekobi $(LINK_FLAGS) $(DGL_LIBS) -lpthread -o $@
	./DISTRHO

DISTRHO.so: DISTRHO.cpp ../modules/distrho/*.hpp ../modules/distrho/src/*.cpp
	$(CXX) $< ../modules/dgl.a $(PEDANTIC_CXX_FLAGS) -I../modules/distrho -I../modules/native-plugins/nekobi -DSHARED_DLL $(LINK_FLAGS) $(DGL_LIBS) -lpthread -shared -Wl,--no-undefined -o $@

DGL: DGL.cpp ../modules/distrho/dgl/src/Window.cpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) -o $@
# 	./DGL
# 	valgrind ./DGL

# --------------------------------------------------------------
