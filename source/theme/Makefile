#!/usr/bin/make -f
# Makefile for theme #
# ------------------ #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -fvisibility=hidden -I.

ifeq ($(HAVE_QT5),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Widgets)
LINK_FLAGS      += $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Widgets)
else
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore QtGui)
LINK_FLAGS      += $(shell pkg-config --libs QtCore QtGui)
endif

FILES = \
	moc_CarlaStyle.cpp \
	moc_CarlaStylePrivate.cpp \
	moc_CarlaStyleAnimations.cpp \
	resources.cpp

OBJS = \
	moc_CarlaStyle.cpp.o \
	moc_CarlaStyleAnimations.cpp.o \
	moc_CarlaStylePrivate.cpp.o

OBJS_shared = $(OBJS) \
	CarlaStyle.export.cpp.o

OBJS_static = $(OBJS) \
	CarlaStyle.cpp.o

SHARED = carlastyle.so
STATIC = ../libs/theme.a

# --------------------------------------------------------------

all: $(STATIC)

clean:
	rm -f $(FILES) $(OBJS_shared) $(OBJS_static) $(SHARED) $(STATIC)

debug:
	$(MAKE) DEBUG=true

install: $(SHARED)
	# FIXME - find a way to get a proper install path
	install -m 644 $(SHARED) /usr/lib/x86_64-linux-gnu/qt4/plugins/styles/

# --------------------------------------------------------------

$(SHARED): $(FILES) $(OBJS_shared)
	$(CXX) $(OBJS_shared) $(LINK_FLAGS) -shared -o $@ && $(STRIP) $@

$(STATIC): $(FILES) $(OBJS_static)
	$(AR) rs $@ $(OBJS_static)

# --------------------------------------------------------------

%.cpp.o: %.cpp CarlaStyle.hpp moc_CarlaStyle.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

%.export.cpp.o: %.cpp CarlaStyle.hpp moc_CarlaStyle.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -DCARLA_EXPORT_STYLE -c -o $@

moc_%.cpp: %.hpp
	$(MOC) $< -o $@

resources.cpp: ../../resources/resources-theme.qrc
	$(RCC) $< -o $@
