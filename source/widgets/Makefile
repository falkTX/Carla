#!/usr/bin/make -f
# Makefile for widgets #
# -------------------- #
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

FILES = \
	moc_paramspinbox.cpp \
	moc_pixmapkeyboard.cpp

OBJS = \
	digitalpeakmeter.cpp.o \
	ledbutton.cpp.o \
	paramspinbox.cpp.o \
	pixmapdial.cpp.o \
	pixmapkeyboard.cpp.o \
	moc_paramspinbox.cpp.o \
	moc_pixmapkeyboard.cpp.o

TARGET = ../libs/widgets.a

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

%.cpp.o: %.cpp %.hpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

moc_%.cpp.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

moc_%.cpp: %.hpp
	$(MOC) $< -o $@
