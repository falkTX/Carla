#!/usr/bin/make -f
# Makefile for theme #
# ------------------ #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I. -I../includes -I../utils

ifeq ($(HAVE_QT4),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore QtGui)
LINK_FLAGS      += $(shell pkg-config --libs QtCore QtGui)
else
BUILD_CXX_FLAGS += $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Widgets)
LINK_FLAGS      += $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Widgets)
endif

# --------------------------------------------------------------

FILES = \
	moc_CarlaStyle.cpp \
	moc_CarlaStylePrivate.cpp \
	moc_CarlaStyleAnimations.cpp \
	resources.cpp

OBJS = \
	moc_CarlaStyle.cpp.o \
	moc_CarlaStyleAnimations.cpp.o \
	moc_CarlaStylePrivate.cpp.o

OBJS_posix32 = \
	moc_CarlaStyle.cpp.posix32.o \
	moc_CarlaStyleAnimations.cpp.posix32.o \
	moc_CarlaStylePrivate.cpp.posix32.o \
	CarlaStyle.cpp.posix32.o

OBJS_posix64 = \
	moc_CarlaStyle.cpp.posix64.o \
	moc_CarlaStyleAnimations.cpp.posix64.o \
	moc_CarlaStylePrivate.cpp.posix64.o \
	CarlaStyle.cpp.posix64.o

OBJS_win32 = \
	moc_CarlaStyle.cpp.win32.o \
	moc_CarlaStyleAnimations.cpp.win32.o \
	moc_CarlaStylePrivate.cpp.win32.o \
	CarlaStyle.cpp.win32.o

OBJS_win64 = \
	moc_CarlaStyle.cpp.win64.o \
	moc_CarlaStyleAnimations.cpp.win64.o \
	moc_CarlaStylePrivate.cpp.win64.o \
	CarlaStyle.cpp.win64.o

OBJS_shared = $(OBJS) \
	CarlaStyle.export.cpp.o

OBJS_static = $(OBJS) \
	CarlaStyle.cpp.o

TARGET = ../libs/theme.a

# --------------------------------------------------------------

all: $(TARGET)

clean:
	rm -f $(FILES) $(TARGET) *.o

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

$(TARGET): $(FILES) $(OBJS_static)
	$(AR) rs $@ $(OBJS_static)

carlastyle.dll: $(FILES) $(OBJS_shared)
	$(CXX) $(OBJS_shared) $(LINK_FLAGS) -shared -o $@ $(CMD_STRIP) $@

carlastyle.dynlib: $(FILES) $(OBJS_shared)
	$(CXX) $(OBJS_shared) $(LINK_FLAGS) -dynamiclib -o $@ $(CMD_STRIP) $@

carlastyle.so: $(FILES) $(OBJS_shared)
	$(CXX) $(OBJS_shared) $(LINK_FLAGS) -shared -o $@ $(CMD_STRIP) $@

# --------------------------------------------------------------

%.cpp.o: %.cpp CarlaStyle.hpp moc_CarlaStyle.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

%.export.cpp.o: %.cpp CarlaStyle.hpp moc_CarlaStyle.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_EXPORT_STYLE -c -o $@

%.cpp.posix32.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

%.cpp.posix64.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

%.cpp.win32.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

%.cpp.win64.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

moc_%.cpp: %.hpp
	$(MOC) $< -o $@

resources.cpp: ../../resources/resources-theme.qrc
	$(RCC) $< -o $@
