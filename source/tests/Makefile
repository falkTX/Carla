#!/usr/bin/make -f
# Makefile for carla tests #
# ------------------------ #
# Created by falkTX
#

# --------------------------------------------------------------

BASE_FLAGS  = -Wall -Wextra -Werror -fPIC -DPIC -pipe -DREAL_BUILD -DDEBUG -O0 -g
BASE_FLAGS += -I. -I../backend -I../includes -I../utils -isystem ../modules
BASE_FLAGS += -Wcast-align -Wcast-qual -Wconversion -Wmissing-declarations -Wsign-conversion -Wuninitialized -Wunused-parameter
BASE_FLAGS += -Wformat -Wformat-security -Wredundant-decls -Wshadow -Wstrict-overflow -fstrict-overflow -Wundef -Wwrite-strings
ifneq ($(CC),clang-3.4)
BASE_FLAGS += -Wlogical-op
endif

# --------------------------------------------------------------

ANSI_FLAGS  = $(BASE_FLAGS) -DBUILD_ANSI_TEST
ANSI_FLAGS += -ansi -pedantic -pedantic-errors -Waggregate-return
# ANSI_FLAGS += -L../backend -lcarla_standalone2

# --------------------------------------------------------------

GNU_CXX_FLAGS  = $(BASE_FLAGS)
GNU_CXX_FLAGS += -std=c++11 -std=gnu++11 -Wzero-as-null-pointer-constant

# --------------------------------------------------------------

PEDANTIC_CXX_FLAGS  = $(BASE_FLAGS)
PEDANTIC_CXX_FLAGS += -std=c++11 -pedantic -pedantic-errors
PEDANTIC_CXX_FLAGS += -isystem /opt/kxstudio/include
PEDANTIC_CXX_FLAGS += -Wno-vla -Wno-vla-extension
ifeq ($(CC),clang-3.4)
PEDANTIC_CXX_FLAGS += -Wno-vla-extension
else
PEDANTIC_CXX_FLAGS += -Wunsafe-loop-optimizations -Wzero-as-null-pointer-constant
endif

# --------------------------------------------------------------

TARGETS  = ansi-pedantic-test_c ansi-pedantic-test_c99
TARGETS += CarlaString CarlaUtils

# ansi-pedantic-test_cxx ansi-pedantic-test_cxx11
# TARGETS +=  EngineEvents PipeServer Print RtLinkedList RtLinkedListGnu Utils

all: $(TARGETS)

# --------------------------------------------------------------

ansi-pedantic-test_c: ansi-pedantic-test.c ../backend/Carla*.h
	$(CC) $< $(ANSI_FLAGS) -Wc++-compat -o $@

ansi-pedantic-test_c99: ansi-pedantic-test.c ../backend/Carla*.h
	$(CC) $< $(ANSI_FLAGS) -std=c99 -pedantic -pedantic-errors -Wc++-compat -o $@

ansi-pedantic-test_cxx: ansi-pedantic-test.cpp ../backend/Carla*.h
	$(CXX) $< $(ANSI_FLAGS) -o $@

ansi-pedantic-test_cxx11: ansi-pedantic-test.cpp ../backend/Carla*.h
	$(CXX) $< $(ANSI_FLAGS) -std=c++11 -pedantic -pedantic-errors -o $@

# --------------------------------------------------------------

CarlaString: CarlaString.cpp ../utils/CarlaString.hpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -o $@
	valgrind --leak-check=full ./$@

CarlaUtils: CarlaUtils.cpp ../utils/*.hpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -o $@
	valgrind --leak-check=full ./$@

# --------------------------------------------------------------

ChildProcess: ChildProcess.cpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) ../modules/juce_core.a -ldl -lpthread -lrt -o $@
# 	valgrind --leak-check=full ./$@

CarlaMutex: CarlaMutex.cpp ../utils/CarlaMutex.hpp
	$(CXX) $< -lpthread -o $@
	valgrind --leak-check=full ./$@

Engine: Engine.cpp
	$(CXX) $< \
	../backend/standalone/CarlaStandalone.cpp.o \
	-Wl,--start-group \
	../backend/carla_engine.a ../backend/carla_plugin.a ../modules/native-plugins.a \
	../modules/dgl.a ../modules/jackbridge.a ../modules/lilv.a ../modules/rtmempool.a ../modules/rtaudio.a ../modules/rtmidi.a \
	-Wl,--end-group \
	$(PEDANTIC_CXX_FLAGS) $(shell pkg-config --libs alsa libpulse-simple liblo QtCore QtXml fluidsynth linuxsampler x11 gl smf fftw3 mxml zlib ntk_images ntk) -o $@
	env LD_LIBRARY_PATH=../backend valgrind --leak-check=full ./$@
# 	../modules/juce_audio_basics.a ../modules/juce_core.a \

Exception: Exception.cpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -o $@
# 	valgrind ./$@

EngineEvents: EngineEvents.cpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -L../backend -lcarla_standalone2 -o $@
	env LD_LIBRARY_PATH=../backend valgrind ./$@

PipeServer: PipeServer.cpp ../utils/CarlaPipeUtils.hpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -lpthread -o $@
# 	valgrind ./$@

Print: Print.cpp ../utils/CarlaUtils.hpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -o $@
	valgrind ./$@

RtLinkedList: RtLinkedList.cpp ../utils/LinkedList.hpp ../utils/RtLinkedList.hpp ../modules/rtmempool.a
	$(CXX) $< ../modules/rtmempool.a $(PEDANTIC_CXX_FLAGS) -lpthread -o $@
	valgrind ./$@

RtLinkedListGnu: RtLinkedList.cpp ../utils/LinkedList.hpp ../utils/RtLinkedList.hpp ../modules/rtmempool.a
	$(CXX) $< ../modules/rtmempool.a $(GNU_CXX_FLAGS) -lpthread -o $@
	valgrind ./$@

# --------------------------------------------------------------

Rewire.exe: Rewire.cpp.o
	$(CXX) $^ -lversion -o $@

Rewire.cpp.o: Rewire.cpp
	$(CXX) $< -Wall -Wextra -DREAL_BUILD -DDEBUG -O0 -g -I../includes -I../utils -std=c++11 -c -o $@

# --------------------------------------------------------------

../modules/%.a:
	$(MAKE) -C ../modules $*

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
