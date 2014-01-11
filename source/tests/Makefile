#!/usr/bin/make -f
# Makefile for carla tests #
# ------------------------ #
# Created by falkTX
#

# --------------------------------------------------------------

BASE_FLAGS  = -Wall -Wextra -Werror -fPIC -DPIC -pipe -DREAL_BUILD
BASE_FLAGS += -Wcast-qual -Wconversion -Wlogical-op
BASE_FLAGS += -Wsign-conversion -Wuninitialized -Wunused-parameter
BASE_FLAGS += -DDEBUG -O0 -g
BASE_FLAGS += -I. -I../backend -I../includes -I../utils
BASE_FLAGS += -isystem ../modules

# --------------------------------------------------------------

ANSI_FLAGS  = $(BASE_FLAGS) -DBUILD_ANSI_TEST
ANSI_FLAGS += -ansi -pedantic -pedantic-errors -Waggregate-return
ANSI_FLAGS += -I../backend -I../includes
# ANSI_FLAGS += -L../backend -lcarla_standalone2

# --------------------------------------------------------------

GNU_CXX_FLAGS  = $(BASE_FLAGS)
GNU_CXX_FLAGS += -std=c++11 -std=gnu++11 -Wzero-as-null-pointer-constant

# --------------------------------------------------------------

PEDANTIC_CXX_FLAGS  = $(BASE_FLAGS)
PEDANTIC_CXX_FLAGS += -std=c++11 -pedantic -pedantic-errors -Wzero-as-null-pointer-constant -Wno-vla
PEDANTIC_CXX_FLAGS += -isystem /opt/kxstudio/include

# --------------------------------------------------------------

# TARGETS  = ansi-pedantic-test_c ansi-pedantic-test_c99 ansi-pedantic-test_cxx ansi-pedantic-test_cxx11
TARGETS += CarlaString Print RtLinkedList RtLinkedListGnu Utils

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
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -o $@
	valgrind ./CarlaString

Print: Print.cpp ../utils/CarlaUtils.hpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -o $@
	valgrind ./Print

RtLinkedList: RtLinkedList.cpp ../utils/LinkedList.hpp ../utils/RtLinkedList.hpp ../modules/rtmempool.a
	$(CXX) $< ../modules/rtmempool.a $(PEDANTIC_CXX_FLAGS) -lpthread -o $@
	valgrind ./RtLinkedList

RtLinkedListGnu: RtLinkedList.cpp ../utils/LinkedList.hpp ../utils/RtLinkedList.hpp ../modules/rtmempool.a
	$(CXX) $< ../modules/rtmempool.a $(GNU_CXX_FLAGS) -lpthread -o $@
	valgrind ./RtLinkedListGnu

Utils: Utils.cpp ../utils/*.hpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) -ldl -lpthread -o $@
	valgrind ./Utils

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
	$(CXX) $< ../modules/dgl.a $(PEDANTIC_CXX_FLAGS) -I../modules/distrho -I../modules/carla_native/nekobi $(LINK_FLAGS) $(DGL_LIBS) -lpthread -o $@
	./DISTRHO

DISTRHO.so: DISTRHO.cpp ../modules/distrho/*.hpp ../modules/distrho/src/*.cpp
	$(CXX) $< ../modules/dgl.a $(PEDANTIC_CXX_FLAGS) -I../modules/distrho -I../modules/carla_native/nekobi -DSHARED_DLL $(LINK_FLAGS) $(DGL_LIBS) -lpthread -shared -Wl,--no-undefined -o $@

DGL: DGL.cpp ../modules/distrho/dgl/src/Window.cpp
	$(CXX) $< $(PEDANTIC_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) -o $@
# 	./DGL
# 	valgrind ./DGL

# --------------------------------------------------------------
