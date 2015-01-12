#!/usr/bin/make -f
# Makefile for carla-widgets #
# -------------------------- #
# Created by falkTX
#

CWD=..
include $(CWD)/Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

BINDIR    := $(CWD)/../bin

ifeq ($(DEBUG),true)
OBJDIR    := $(CWD)/../build/widgets/Debug
MODULEDIR := $(CWD)/../build/modules/Debug
else
OBJDIR    := $(CWD)/../build/widgets/Release
MODULEDIR := $(CWD)/../build/modules/Release
endif

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += -I. -I$(CWD)/includes -I$(CWD)/modules -I$(CWD)/utils

ifeq ($(HAVE_QT4),true)
BUILD_CXX_FLAGS += $(shell pkg-config --cflags QtCore QtGui)
LINK_FLAGS      += $(shell pkg-config --libs QtCore QtGui)
else
BUILD_CXX_FLAGS += $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Widgets)
LINK_FLAGS      += $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Widgets)
endif

# ----------------------------------------------------------------------------------------------------------------------------

# FILES = \
# 	moc_paramspinbox.cpp \
# 	moc_pixmapkeyboard.cpp

OBJS  = \
	$(OBJDIR)/digitalpeakmeter.cpp.o

# 	ledbutton.cpp.o \
# 	paramspinbox.cpp.o \
# 	pixmapdial.cpp.o \
# 	pixmapkeyboard.cpp.o

# OBJS += \
# 	moc_paramspinbox.cpp.o \
# 	moc_pixmapkeyboard.cpp.o

TARGET = $(MODULEDIR)/widgets.a

# ----------------------------------------------------------------------------------------------------------------------------

all: $(TARGET)

# ----------------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(TARGET)

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/widgets.a: $(FILES) $(OBJS)
	$(RM) $@
	$(AR) crs $@ $(OBJS)

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

-include $(OBJS:%.o=%.d)

# moc_%.cpp.o: %.cpp
# 	$(CXX) moc_$< $(BUILD_CXX_FLAGS) -c -o $@
#
# moc_%.cpp: %.hpp
# 	$(MOC) $< -o $@

# ----------------------------------------------------------------------------------------------------------------------------
