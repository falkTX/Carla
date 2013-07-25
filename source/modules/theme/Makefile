#!/usr/bin/make -f
# Makefile for theme #
# ------------------ #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I. -I../../includes -I../utils
QT5_CXX_FLAGS    = $(BUILD_CXX_FLAGS) # FIXME: use copy instead of reference

ifeq ($(HAVE_QT4),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore QtGui)
LINK_FLAGS      += $(shell pkg-config --libs QtCore QtGui)
else
BUILD_CXX_FLAGS += $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Widgets)
LINK_FLAGS      += $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Widgets)
endif

ifeq ($(HAVE_QT5),true)
QT5_CXX_FLAGS   += $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Widgets)
endif

# --------------------------------------------------------------

FILES = \
	moc_CarlaStyle.cpp \
	moc_CarlaStylePrivate.cpp \
	moc_CarlaStyleAnimations.cpp \
	resources.cpp

OBJS  = \
	moc_CarlaStyle.cpp.o \
	moc_CarlaStyleAnimations.cpp.o \
	moc_CarlaStylePrivate.cpp.o

OBJS_static = $(OBJS) \
	CarlaStyle.cpp.o

OBJS_shared = $(OBJS) \
	CarlaStyle.cpp.export.o

OBJS_qt5 = \
	moc_CarlaStyle.cpp.qt5.o \
	moc_CarlaStyleAnimations.cpp.qt5.o \
	moc_CarlaStylePrivate.cpp.qt5.o \
	CarlaStyle.cpp.qt5.o

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

# --------------------------------------------------------------

all: ../theme.a

qt5:     ../theme.qt5.a
posix32: ../theme.posix32.a
posix64: ../theme.posix64.a
win32:   ../theme.win32.a
win64:   ../theme.win64.a

# --------------------------------------------------------------

../theme.a: $(FILES) $(OBJS_static)
	$(AR) rs $@ $(OBJS_static)

../theme.qt5.a: $(FILES) $(OBJS_qt5)
	$(AR) rs $@ $(OBJS_qt5)

../theme.posix32.a: $(FILES) $(OBJS_posix32)
	$(AR) rs $@ $(OBJS_posix32)

../theme.posix64.a: $(FILES) $(OBJS_posix64)
	$(AR) rs $@ $(OBJS_posix64)

../theme.win32.a: $(FILES) $(OBJS_win32)
	$(AR) rs $@ $(OBJS_win32)

../theme.win64.a: $(FILES) $(OBJS_win64)
	$(AR) rs $@ $(OBJS_win64)

../carlastyle.dll: $(FILES) $(OBJS_shared)
	$(CXX) $(OBJS_shared) -shared $(LINK_FLAGS) -o $@ $(CMD_STRIP) $@

../carlastyle.dynlib: $(FILES) $(OBJS_shared)
	$(CXX) $(OBJS_shared) -dynamiclib $(LINK_FLAGS) -o $@ $(CMD_STRIP) $@

../carlastyle.so: $(FILES) $(OBJS_shared)
	$(CXX) $(OBJS_shared) -shared $(LINK_FLAGS) -o $@ $(CMD_STRIP) $@

# --------------------------------------------------------------

%.cpp.o: %.cpp CarlaStyle.hpp moc_CarlaStyle.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

%.cpp.qt5.o: %.cpp CarlaStyle.hpp moc_CarlaStyle.cpp
	$(CXX) $< $(QT5_CXX_FLAGS) -c -o $@

%.cpp.export.o: %.cpp CarlaStyle.hpp moc_CarlaStyle.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_EXPORT_STYLE -c -o $@

%.cpp.posix32.o: %.cpp CarlaStyle.hpp moc_CarlaStyle.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

%.cpp.posix64.o: %.cpp CarlaStyle.hpp moc_CarlaStyle.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

%.cpp.win32.o: %.cpp CarlaStyle.hpp moc_CarlaStyle.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

%.cpp.win64.o: %.cpp CarlaStyle.hpp moc_CarlaStyle.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

moc_%.cpp: %.hpp
	$(MOC) $< -o $@

resources.cpp: ../../../resources/resources-theme.qrc
	$(RCC) $< -o $@

# --------------------------------------------------------------

clean:
	rm -f *.o ../theme.* $(FILES)

debug:
	$(MAKE) DEBUG=true
