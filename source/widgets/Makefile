#!/usr/bin/make -f
# Makefile for widgets #
# -------------------- #
# Created by falkTX
#

include ../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -fvisibility=hidden -fPIC -I.
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore)
LINK_FLAGS      += $(shell pkg-config --libs QtCore)

FILES = \
	moc_pixmapkeyboard.cpp

OBJS = \
	digitalpeakmeter.cpp.o \
	ledbutton.cpp.o \
	paramspinbox.cpp.o \
	pixmapdial.cpp.o \
	pixmapkeyboard.cpp.o \
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

%.cpp.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

moc_%.cpp: %.hpp
	$(MOC) $< -o $@
