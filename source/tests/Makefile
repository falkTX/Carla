#!/usr/bin/make -f
# Makefile for carla tests #
# ------------------------ #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I. -I../backend -I../includes -I../modules -I../modules/distrho -I../utils
BUILD_CXX_FLAGS += -std=c++11 -std=gnu++11 -Wzero-as-null-pointer-constant
BUILD_CXX_FLAGS += -isystem /opt/kxstudio/include

# --------------------------------------------------------------

ANSI_FLAGS  = -DBUILD_ANSI_TEST -DREAL_BUILD -I../backend -I../includes
ANSI_FLAGS += -Wall -Wextra -Werror
ANSI_FLAGS += -DDEBUG -O0 -g
ANSI_FLAGS += -ansi -pedantic -pedantic-errors
ANSI_FLAGS += -Wunused-parameter -Wuninitialized -Wcast-qual -Wconversion -Wsign-conversion -Wlogical-op -Waggregate-return
ANSI_FLAGS += -L../backend -lcarla_standalone2

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

DISTRHO: DISTRHO.cpp ../modules/distrho/*.hpp ../modules/distrho/src/*.cpp
	$(CXX) $< ../modules/dgl.a $(BUILD_CXX_FLAGS) -I../modules/distrho -I../modules/carla_native/nekobi $(LINK_FLAGS) $(DGL_LIBS) -lpthread -o $@
	./DISTRHO

DISTRHO.so: DISTRHO.cpp ../modules/distrho/*.hpp ../modules/distrho/src/*.cpp
	$(CXX) $< ../modules/dgl.a $(BUILD_CXX_FLAGS) -I../modules/distrho -I../modules/carla_native/nekobi -DSHARED_DLL $(LINK_FLAGS) $(DGL_LIBS) -lpthread -shared -Wl,--no-undefined -o $@

DGL: DGL.cpp ../modules/distrho/dgl/src/Window.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) -o $@
# 	./DGL
# 	valgrind ./DGL

LibOpenMain: LibOpenMain.cpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -ldl -o $@

Print: Print.cpp
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@
# 	valgrind ./Print

RtLinkedList: RtLinkedList.cpp ../utils/LinkedList.hpp ../utils/RtLinkedList.hpp ../modules/rtmempool.a
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -lpthread -o $@
	valgrind ./RtLinkedList

RtLinkedListGnu: RtLinkedList.cpp ../utils/LinkedList.hpp ../utils/RtLinkedList.hpp ../modules/rtmempool.a
	$(CXX) $^ $(BUILD_CXX_FLAGS) -std=gnu++11 $(LINK_FLAGS) -lpthread -o $@
	valgrind ./RtLinkedListGnu

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
