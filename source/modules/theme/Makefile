#!/usr/bin/make -f
# Makefile for theme #
# ------------------ #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I. -I../../includes -I../../utils

QT5_CXX_FLAGS    = $(BUILD_CXX_FLAGS)
QT5_CXX_FLAGS   += $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Widgets)
QT5_LINK_FLAGS  += $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Widgets)

ifeq ($(HAVE_QT4),true)
QT4_CXX_FLAGS    = $(BUILD_CXX_FLAGS)
QT4_CXX_FLAGS   += $(shell pkg-config --cflags QtCore QtGui)
QT4_LINK_FLAGS  += $(shell pkg-config --libs QtCore QtGui)
endif

QT_STYLES_DIR    = $(shell pkg-config --variable=libdir Qt5Core)/qt5/plugins/styles

# --------------------------------------------------------------

FILES = \
	moc_CarlaStyle.cpp \
	moc_CarlaStyleAnimations.cpp \
	moc_CarlaStylePlugin.cpp \
	moc_CarlaStylePrivate.cpp \
	resources.cpp

FILES_qt4 = \
	moc_CarlaStyle.qt4.cpp \
	moc_CarlaStyleAnimations.qt4.cpp \
	moc_CarlaStylePlugin.qt4.cpp \
	moc_CarlaStylePrivate.qt4.cpp \
	resources.qt4.cpp

# --------------------------------------------------------------

OBJS = \
	CarlaStyle.cpp.o \
	moc_CarlaStyle.cpp.o \
	moc_CarlaStyleAnimations.cpp.o \
	moc_CarlaStylePrivate.cpp.o \
	resources.cpp.o

OBJS_shared = $(OBJS) \
	CarlaStylePlugin.cpp.o \
	moc_CarlaStylePlugin.cpp.o

# --------------------------------------------------------------

OBJS_qt4 = \
	CarlaStyle.cpp.qt4.o \
	moc_CarlaStyle.qt4.cpp.o \
	moc_CarlaStyleAnimations.qt4.cpp.o \
	moc_CarlaStylePrivate.qt4.cpp.o \
	resources.qt4.cpp.o

OBJS_posix32 = \
	CarlaStyle.cpp.posix32.o \
	moc_CarlaStyle.cpp.posix32.o \
	moc_CarlaStyleAnimations.cpp.posix32.o \
	moc_CarlaStylePrivate.cpp.posix32.o \
	resources.cpp.posix32.o

OBJS_posix64 = \
	CarlaStyle.cpp.posix64.o \
	moc_CarlaStyle.cpp.posix64.o \
	moc_CarlaStyleAnimations.cpp.posix64.o \
	moc_CarlaStylePrivate.cpp.posix64.o \
	resources.cpp.posix64.o

OBJS_win32 = \
	CarlaStyle.cpp.win32.o \
	moc_CarlaStyle.cpp.win32.o \
	moc_CarlaStyleAnimations.cpp.win32.o \
	moc_CarlaStylePrivate.cpp.win32.o \
	resources.cpp.win32.o

OBJS_win64 = \
	CarlaStyle.cpp.win64.o \
	moc_CarlaStyle.cpp.win64.o \
	moc_CarlaStyleAnimations.cpp.win64.o \
	moc_CarlaStylePrivate.cpp.win64.o \
	resources.cpp.win64.o

# --------------------------------------------------------------

ifeq ($(WIN32),true)
CARLASTYLE = styles/carlastyle.dll
else
ifeq ($(MACOS),true)
CARLASTYLE = styles/carlastyle.dylib
else
CARLASTYLE = styles/carlastyle.so
endif
endif

all: ../theme.a $(CARLASTYLE)

qt4:     ../theme.qt4.a
posix32: ../theme.posix32.a
posix64: ../theme.posix64.a
win32:   ../theme.win32.a
win64:   ../theme.win64.a

# --------------------------------------------------------------

../theme.a: $(FILES) $(OBJS)
	$(RM) $@
	$(AR) crs $@ $(OBJS)

../theme.qt4.a: $(FILES_qt4) $(OBJS_qt4)
	$(RM) $@
	$(AR) crs $@ $(OBJS_qt4)

../theme.posix32.a: $(FILES) $(OBJS_posix32)
	$(RM) $@
	$(AR) crs $@ $(OBJS_posix32)

../theme.posix64.a: $(FILES) $(OBJS_posix64)
	$(RM) $@
	$(AR) crs $@ $(OBJS_posix64)

../theme.win32.a: $(FILES) $(OBJS_win32)
	$(RM) $@
	$(AR) crs $@ $(OBJS_win32)

../theme.win64.a: $(FILES) $(OBJS_win64)
	$(RM) $@
	$(AR) crs $@ $(OBJS_win64)

# --------------------------------------------------------------

styles/carlastyle.dll: $(FILES) $(OBJS_shared)
	$(CXX) $(OBJS_shared) -shared $(QT5_LINK_FLAGS) -o $@

styles/carlastyle.dynlib: $(FILES) $(OBJS_shared)
	$(CXX) $(OBJS_shared) -dynamiclib $(QT5_LINK_FLAGS) -o $@

styles/carlastyle.so: $(FILES) $(OBJS_shared)
	$(CXX) $(OBJS_shared) -shared $(QT5_LINK_FLAGS) -o $@

# --------------------------------------------------------------

%.qt4.cpp.o: %.qt4.cpp CarlaStyle.hpp moc_CarlaStyle.qt4.cpp
	$(CXX) $< $(QT4_CXX_FLAGS) -c -o $@

%.cpp.qt4.o: %.cpp CarlaStyle.hpp moc_CarlaStyle.qt4.cpp
	$(CXX) $< $(QT4_CXX_FLAGS) -c -o $@

%.cpp.posix32.o: %.cpp CarlaStyle.hpp moc_CarlaStyle.cpp
	$(CXX) $< $(QT5_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

%.cpp.posix64.o: %.cpp CarlaStyle.hpp moc_CarlaStyle.cpp
	$(CXX) $< $(QT5_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

%.cpp.win32.o: %.cpp CarlaStyle.hpp moc_CarlaStyle.cpp
	$(CXX) $< $(QT5_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

%.cpp.win64.o: %.cpp CarlaStyle.hpp moc_CarlaStyle.cpp
	$(CXX) $< $(QT5_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

%.cpp.o: %.cpp CarlaStyle.hpp moc_CarlaStyle.cpp
	$(CXX) $< $(QT5_CXX_FLAGS) -c -o $@

moc_%.qt4.cpp: %.hpp
	$(MOC_QT4) $< -o $@

moc_%.cpp: %.hpp
	$(MOC) -Istyles $< -o $@

resources.cpp: ../../../resources/resources-theme.qrc
	$(RCC) $< -o $@

resources.qt4.cpp: ../../../resources/resources-theme.qrc
	$(RCC_QT4) $< -o $@

# --------------------------------------------------------------

clean:
	$(RM) *.o ../theme*.a $(CARLASTYLE) $(FILES) $(FILES_qt4)

debug:
	$(MAKE) DEBUG=true

install: $(CARLASTYLE) styles/carlastyle.json
	install -d $(QT_STYLES_DIR)
	install -m 644 $^ $(QT_STYLES_DIR)

# --------------------------------------------------------------
