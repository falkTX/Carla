#!/usr/bin/make -f
# Makefile for tests #
# ------------------ #
# Created by falkTX
#

CWD=..
include $(CWD)/Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

BINDIR    := $(CWD)/../bin

ifeq ($(DEBUG),true)
OBJDIR    := $(CWD)/../build/tests/Debug
MODULEDIR := $(CWD)/../build/modules/Debug
else
OBJDIR    := $(CWD)/../build/tests/Release
MODULEDIR := $(CWD)/../build/modules/Release
endif

# ---------------------------------------------------------------------------------------------------------------------

BASE_FLAGS      += -DBUILDING_CARLA
BASE_FLAGS      += -I. -I.. -I../backend -I../includes -I../modules -I../utils
BUILD_CXX_FLAGS += -I$(CWD)/modules

# ---------------------------------------------------------------------------------------------------------------------

PEDANTIC_FLAGS  = -pedantic -pedantic-errors
PEDANTIC_FLAGS += -Wall -Wextra -Werror -pipe

PEDANTIC_FLAGS += -Wabi=98 -Wcast-qual -Wclobbered -Wconversion -Wdisabled-optimization
PEDANTIC_FLAGS += -Wcast-align -Wcast-qual -Wclobbered -Wconversion -Wdisabled-optimization
PEDANTIC_FLAGS += -Wdouble-promotion -Wfloat-equal -Wlogical-op -Wpointer-arith -Wsign-conversion
PEDANTIC_FLAGS += -Wformat=2 -Woverlength-strings
PEDANTIC_FLAGS += -Wformat-truncation=2 -Wformat-overflow=2
PEDANTIC_FLAGS += -Wstringop-overflow=4
PEDANTIC_FLAGS += -Wmissing-declarations -Wredundant-decls
PEDANTIC_FLAGS += -Wshadow  -Wundef -Wuninitialized -Wunused
PEDANTIC_FLAGS += -Wstrict-aliasing -fstrict-aliasing
PEDANTIC_FLAGS += -Wstrict-overflow -fstrict-overflow
PEDANTIC_FLAGS += -Wduplicated-branches -Wduplicated-cond -Wnull-dereference
# -Wstringop-truncation

PEDANTIC_CFLAGS  = $(PEDANTIC_FLAGS)
PEDANTIC_CFLAGS += -Wc++-compat
PEDANTIC_CFLAGS += -Winit-self -Wjump-misses-init -Wnested-externs -Wwrite-strings
PEDANTIC_CFLAGS += -Wmissing-prototypes -Wstrict-prototypes

PEDANTIC_CXXFLAGS  = $(PEDANTIC_FLAGS)
PEDANTIC_CXXFLAGS += -Wc++0x-compat -Wc++11-compat
PEDANTIC_CXXFLAGS += -Wnon-virtual-dtor -Woverloaded-virtual
PEDANTIC_CXXFLAGS += -Wold-style-cast -Wuseless-cast
PEDANTIC_CXXFLAGS += -Wzero-as-null-pointer-constant
PEDANTIC_CXXFLAGS += -Weffc++

PEDANTIC_CXX03_SKIPS  = -Wno-pedantic
PEDANTIC_CXX03_SKIPS += -Wno-zero-as-null-pointer-constant

PEDANTIC_CXX98_SKIPS  = $(PEDANTIC_CXX03_SKIPS)
PEDANTIC_CXX98_SKIPS += -Wno-variadic-macros

PEDANTIC_LDFLAGS = -Wl,-rpath,$(BINDIR) -L$(BINDIR)

# ---------------------------------------------------------------------------------------------------------------------
# Set targets

TARGETS = \
	ansi-pedantic-test_c_ansi_run \
	ansi-pedantic-test_c89_run \
	ansi-pedantic-test_c99_run \
	ansi-pedantic-test_c11_run \
	ansi-pedantic-test_cxx_ansi_run \
	ansi-pedantic-test_cxx98_run \
	ansi-pedantic-test_cxx03_run \
	ansi-pedantic-test_cxx11_run \
	carla-host-plugin_run \
	carla-engine-sdl

ifeq ($(WASM),true)
TARGETS = carla-engine-sdl$(APP_EXT)
endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

# ---------------------------------------------------------------------------------------------------------------------

ansi-%_run: $(BINDIR)/ansi-%
	$(BINDIR)/ansi-$*

carla-%_run: $(BINDIR)/carla-%
# 	valgrind $(BINDIR)/carla-$*
	valgrind --leak-check=full --show-leak-kinds=all --suppressions=valgrind.supp $(BINDIR)/carla-$*

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/ansi-pedantic-test_c_ansi: ansi-pedantic-test.c ../backend/Carla*.h ../includes/*.h
	$(CC) $< $(PEDANTIC_CFLAGS) $(PEDANTIC_LDFLAGS) -lcarla_standalone2 -lcarla_utils -ansi -o $@

$(BINDIR)/ansi-pedantic-test_c89: ansi-pedantic-test.c ../backend/Carla*.h ../includes/*.h
	$(CC) $< $(PEDANTIC_CFLAGS) $(PEDANTIC_LDFLAGS) -lcarla_standalone2 -lcarla_utils -std=c89 -o $@

$(BINDIR)/ansi-pedantic-test_c99: ansi-pedantic-test.c ../backend/Carla*.h ../includes/*.h
	$(CC) $< $(PEDANTIC_CFLAGS) $(PEDANTIC_LDFLAGS) -lcarla_standalone2 -lcarla_utils -std=c99 -o $@

$(BINDIR)/ansi-pedantic-test_c11: ansi-pedantic-test.c ../backend/Carla*.h ../includes/*.h
	$(CC) $< $(PEDANTIC_CFLAGS) $(PEDANTIC_LDFLAGS) -lcarla_standalone2 -lcarla_utils -std=c11 -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/ansi-pedantic-test_cxx_ansi: ansi-pedantic-test.cpp ../backend/Carla*.h ../backend/Carla*.hpp ../includes/*.h
	$(CXX) $< $(PEDANTIC_CXXFLAGS) $(PEDANTIC_CXX98_SKIPS) $(PEDANTIC_LDFLAGS) -lcarla_standalone2 -lcarla_utils -ansi -Wno-effc++ -o $@

$(BINDIR)/ansi-pedantic-test_cxx98: ansi-pedantic-test.cpp ../backend/Carla*.h ../backend/Carla*.hpp ../includes/*.h
	$(CXX) $< $(PEDANTIC_CXXFLAGS) $(PEDANTIC_CXX98_SKIPS) $(PEDANTIC_LDFLAGS) -lcarla_standalone2 -lcarla_utils -std=c++98 -Wno-effc++ -o $@

$(BINDIR)/ansi-pedantic-test_cxx03: ansi-pedantic-test.cpp ../backend/Carla*.h ../backend/Carla*.hpp ../includes/*.h
	$(CXX) $< $(PEDANTIC_CXXFLAGS) $(PEDANTIC_CXX03_SKIPS) $(PEDANTIC_LDFLAGS) -lcarla_standalone2 -lcarla_utils -std=c++03 -Wno-effc++ -o $@

$(BINDIR)/ansi-pedantic-test_cxx11: ansi-pedantic-test.cpp ../backend/Carla*.h ../backend/Carla*.hpp ../includes/*.h
	$(CXX) $< $(PEDANTIC_CXXFLAGS) $(PEDANTIC_LDFLAGS) -lcarla_standalone2 -lcarla_utils -std=c++11 -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/carla-host-plugin: carla-host-plugin.c
	$(CC) $< $(PEDANTIC_CFLAGS) $(PEDANTIC_LDFLAGS) -g -O0 -Wno-declaration-after-statement -Wno-pedantic -lcarla_host-plugin -std=c99 -o $@

# ---------------------------------------------------------------------------------------------------------------------

.PHONY: carla-engine-sdl$(APP_EXT)
carla-engine-sdl$(APP_EXT): $(OBJDIR)/carla-engine-sdl.c.o $(OBJDIR)/carla-engine-sdl-extra.cpp.o
	$(CC) $^ \
		$(CWD)/../build/plugin/Release/carla-host-plugin.cpp.o \
		$(MODULEDIR)/carla_engine.a \
		$(MODULEDIR)/carla_plugin.a \
		$(MODULEDIR)/lilv.a \
		$(MODULEDIR)/native-plugins.a \
		$(MODULEDIR)/sfzero.a \
		$(MODULEDIR)/audio_decoder.a \
		$(MODULEDIR)/rtmempool.a \
		$(MODULEDIR)/water.a \
		$(MODULEDIR)/ysfx.a \
		$(MODULEDIR)/zita-resampler.a \
		$(SDL_LIBS) --preload-file foolme.mp3 -sALLOW_MEMORY_GROWTH \
		-o $@

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(BINDIR)/ansi-pedantic-test_* $(BINDIR)/carla-host-plugin

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.c.o: %.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CC) $< $(BUILD_C_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------
