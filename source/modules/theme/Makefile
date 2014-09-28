#!/usr/bin/make -f
# Makefile for theme #
# ------------------ #
# Created by falkTX
#

include ../../Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I. -I../../includes -I../../utils

ifeq ($(HAVE_QT4),true)
QT4_CXX_FLAGS    = $(BUILD_CXX_FLAGS)
QT4_CXX_FLAGS   += $(shell pkg-config --cflags QtCore QtGui)
QT4_LINK_FLAGS   = $(LINK_FLAGS)
QT4_LINK_FLAGS  += $(shell pkg-config --libs QtCore QtGui)
QT4_STYLES_DIR   = $(shell pkg-config --variable=libdir QtCore)/qt4/plugins/styles
endif

ifeq ($(HAVE_QT5),true)
QT5_CXX_FLAGS    = $(BUILD_CXX_FLAGS)
QT5_CXX_FLAGS   += $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Widgets)
QT5_LINK_FLAGS   = $(LINK_FLAGS)
QT5_LINK_FLAGS  += $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Widgets)
QT5_STYLES_DIR   = $(shell pkg-config --variable=libdir Qt5Core)/qt5/plugins/styles
endif

ifeq ($(DEFAULT_QT),4)
QT_STYLES_DIR     = $(QT4_STYLES_DIR)
SHARED_LINK_FLAGS = $(QT4_LINK_FLAGS)
else
QT_STYLES_DIR     = $(QT5_STYLES_DIR)
SHARED_LINK_FLAGS = $(QT5_LINK_FLAGS)
endif

# --------------------------------------------------------------

ifeq ($(HAVE_QT4),true)
FILES_qt4 = \
	moc_CarlaStyle.qt4.cpp \
	moc_CarlaStyleAnimations.qt4.cpp \
	moc_CarlaStylePlugin.qt4.cpp \
	moc_CarlaStylePrivate.qt4.cpp \
	resources.qt4.cpp
endif

ifeq ($(HAVE_QT5),true)
FILES_qt5 = \
	moc_CarlaStyle.qt5.cpp \
	moc_CarlaStyleAnimations.qt5.cpp \
	moc_CarlaStylePlugin.qt5.cpp \
	moc_CarlaStylePrivate.qt5.cpp \
	resources.qt5.cpp
endif

ifeq ($(DEFAULT_QT),4)
FILES_shared = $(FILES_qt4)
else
FILES_shared = $(FILES_qt5)
endif

# --------------------------------------------------------------

ifeq ($(HAVE_QT4),true)
OBJS_qt4 = \
	CarlaStyle.cpp.qt4.o \
	moc_CarlaStyle.qt4.cpp.o \
	moc_CarlaStyleAnimations.qt4.cpp.o \
	moc_CarlaStylePrivate.qt4.cpp.o \
	resources.qt4.cpp.o

OBJS_qt4_shared = $(OBJS_qt4) \
	CarlaStylePlugin.cpp.qt4.o \
	moc_CarlaStylePlugin.qt4.cpp.o
endif

ifeq ($(HAVE_QT5),true)
OBJS_qt5 = \
	CarlaStyle.cpp.qt5.o \
	moc_CarlaStyle.qt5.cpp.o \
	moc_CarlaStyleAnimations.qt5.cpp.o \
	moc_CarlaStylePrivate.qt5.cpp.o \
	resources.qt5.cpp.o

OBJS_qt5_shared = $(OBJS_qt5) \
	CarlaStylePlugin.cpp.qt5.o \
	moc_CarlaStylePlugin.qt5.cpp.o
endif

ifeq ($(DEFAULT_QT),4)
OBJS_shared += $(OBJS_qt4_shared)
else
OBJS_shared += $(OBJS_qt5_shared)
endif

# --------------------------------------------------------------

all:

ifeq ($(DEFAULT_QT),4)
ifeq ($(HAVE_QT4),true)
all: styles/carlastyle$(LIB_EXT)
endif
else # DEFAULT_QT
ifeq ($(HAVE_QT5),true)
all: styles/carlastyle$(LIB_EXT)
endif
endif

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
	$(CXX) $(OBJS_shared) $(SHARED_LINK_FLAGS) -luuid -lwinspool -lgdi32 -limm32 -lole32 -lshell32 -lws2_32 -shared -o $@
# 	$(CXX) $(OBJS_shared) \
# 	-L/home/falktx/.winepy3/drive_c/Python33/Lib/site-packages/PyQt4/ -lQtGui4 -lQtCore4 \
# 	-shared -o $@

styles/carlastyle.dylib: $(FILES_shared) $(OBJS_shared)
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
	$(RM) *.o ../theme*.a styles/carlastyle$(LIB_EXT) $(FILES_qt4) $(FILES_qt5)

debug:
	$(MAKE) DEBUG=true

install: styles/carlastyle$(LIB_EXT) styles/carlastyle.json
	install -d $(QT_STYLES_DIR)
	install -m 644 $^ $(QT_STYLES_DIR)

install-main: styles/carlastyle$(LIB_EXT) styles/carlastyle.json
	install -d $(STYLES_DIR)
	install -m 644 $^ $(STYLES_DIR)

# --------------------------------------------------------------
