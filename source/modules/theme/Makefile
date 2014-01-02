#!/usr/bin/make -f
# Makefile for theme #
# ------------------ #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I. -I../../includes -I../../utils -w

ifeq ($(HAVE_QT4),true)
QT4_CXX_FLAGS    = $(BUILD_CXX_FLAGS)
QT4_CXX_FLAGS   += $(shell pkg-config --cflags QtCore QtGui)
QT4_LINK_FLAGS  += $(shell pkg-config --libs QtCore QtGui)
QT4_STYLES_DIR   = $(shell pkg-config --variable=libdir QtCore)/qt4/plugins/styles
endif

ifeq ($(HAVE_QT5),true)
QT5_CXX_FLAGS    = $(BUILD_CXX_FLAGS)
QT5_CXX_FLAGS   += $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Widgets)
QT5_LINK_FLAGS  += $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Widgets)
QT5_STYLES_DIR   = $(shell pkg-config --variable=libdir Qt5Core)/qt5/plugins/styles
endif

ifeq ($(HAVE_QT4),true)
SHARED_LINK_FLAGS = $(QT4_LINK_FLAGS)
else
SHARED_LINK_FLAGS = $(QT5_LINK_FLAGS)
endif

# --------------------------------------------------------------

FILES_qt4 = \
	moc_CarlaStyle.qt4.cpp \
	moc_CarlaStyleAnimations.qt4.cpp \
	moc_CarlaStylePlugin.qt4.cpp \
	moc_CarlaStylePrivate.qt4.cpp \
	resources.qt4.cpp

FILES_qt5 = \
	moc_CarlaStyle.qt5.cpp \
	moc_CarlaStyleAnimations.qt5.cpp \
	moc_CarlaStylePlugin.qt5.cpp \
	moc_CarlaStylePrivate.qt5.cpp \
	resources.qt5.cpp

ifeq ($(HAVE_QT4),true)
FILES_shared = $(FILES_qt4)
else
FILES_shared = $(FILES_qt5)
endif

# --------------------------------------------------------------

OBJS_qt4 = \
	CarlaStyle.cpp.qt4.o \
	moc_CarlaStyle.qt4.cpp.o \
	moc_CarlaStyleAnimations.qt4.cpp.o \
	moc_CarlaStylePrivate.qt4.cpp.o \
	resources.qt4.cpp.o

OBJS_qt5 = \
	CarlaStyle.cpp.qt5.o \
	moc_CarlaStyle.qt5.cpp.o \
	moc_CarlaStyleAnimations.qt5.cpp.o \
	moc_CarlaStylePrivate.qt5.cpp.o \
	resources.qt5.cpp.o

ifeq ($(HAVE_QT4),true)
OBJS_shared += $(OBJS_qt4) \
	CarlaStylePlugin.cpp.qt4.o \
	moc_CarlaStylePlugin.qt4.cpp.o
else
OBJS_shared += $(OBJS_qt5) \
	CarlaStylePlugin.cpp.qt5.o \
	moc_CarlaStylePlugin.qt5.cpp.o
endif

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

all: $(CARLASTYLE)

qt4: ../theme.qt4.a
qt5: ../theme.qt5.a

# --------------------------------------------------------------

../theme.qt4.a: $(FILES_qt4) $(OBJS_qt4)
	$(RM) $@
	$(AR) crs $@ $(OBJS_qt4)

../theme.qt5.a: $(FILES_qt5) $(OBJS_qt5)
	$(RM) $@
	$(AR) crs $@ $(OBJS_qt5)

# --------------------------------------------------------------

styles/carlastyle.dll: $(FILES_shared) $(OBJS_shared)
	$(CXX) $(OBJS_shared) $(SHARED_LINK_FLAGS) -shared -o $@

styles/carlastyle.dynlib: $(FILES_shared) $(OBJS_shared)
	$(CXX) $(OBJS_shared) $(SHARED_LINK_FLAGS) -dynamiclib -o $@

styles/carlastyle.so: $(FILES_shared) $(OBJS_shared)
	$(CXX) $(OBJS_shared) $(SHARED_LINK_FLAGS) -shared -o $@

# --------------------------------------------------------------

%.qt4.cpp.o: %.qt4.cpp CarlaStyle.hpp moc_CarlaStyle.qt4.cpp
	$(CXX) $< $(QT4_CXX_FLAGS) -c -o $@

%.qt5.cpp.o: %.qt5.cpp CarlaStyle.hpp moc_CarlaStyle.qt5.cpp
	$(CXX) $< $(QT5_CXX_FLAGS) -c -o $@

%.cpp.qt4.o: %.cpp CarlaStyle.hpp moc_CarlaStyle.qt4.cpp
	$(CXX) $< $(QT4_CXX_FLAGS) -c -o $@

%.cpp.qt5.o: %.cpp CarlaStyle.hpp moc_CarlaStyle.qt5.cpp
	$(CXX) $< $(QT5_CXX_FLAGS) -c -o $@

moc_%.qt4.cpp: %.hpp
	$(MOC_QT4) $< -o $@

moc_%.qt5.cpp: %.hpp
	$(MOC_QT5) -Istyles $< -o $@

resources.qt4.cpp: ../../../resources/resources-theme.qrc
	$(RCC_QT4) $< -o $@

resources.qt5.cpp: ../../../resources/resources-theme.qrc
	$(RCC_QT5) $< -o $@

# --------------------------------------------------------------

clean:
	$(RM) *.o ../theme*.a $(CARLASTYLE) $(FILES_qt4) $(FILES_qt5)

debug:
	$(MAKE) DEBUG=true

install: $(CARLASTYLE) styles/carlastyle.json
	install -d $(QT4_STYLES_DIR)
	install -m 644 $^ $(QT4_STYLES_DIR)

install-main: $(CARLASTYLE) styles/carlastyle.json
	install -d $(STYLES_DIR)
	install -m 644 $^ $(STYLES_DIR)

# --------------------------------------------------------------
