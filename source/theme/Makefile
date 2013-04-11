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
	CarlaStyle.cpp.o \
	moc_CarlaStyle.cpp.o \
	moc_CarlaStyleAnimations.cpp.o \
	moc_CarlaStylePrivate.cpp.o

TARGET = ../libs/theme.a

# --------------------------------------------------------------

all: $(TARGET)

clean:
	rm -f $(FILES) $(OBJS) $(TARGET)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

$(TARGET): $(FILES) $(OBJS)
	$(AR) rs $@ $(OBJS)

# --------------------------------------------------------------

%.cpp.o: %.cpp CarlaStyle.hpp moc_CarlaStyle.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

moc_%.cpp: %.hpp
	$(MOC) $< -o $@

resources.cpp: ../../resources/resources-theme.qrc
	$(RCC) $< -o $@
