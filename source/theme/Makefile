#!/usr/bin/make -f
# Makefile for theme #
# ------------------ #
# Created by falkTX
#

CWD=..
MODULENAME=theme
include ../modules/Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

ifeq ($(HAVE_QT4),true)
QT4_CXX_FLAGS  = $(BUILD_CXX_FLAGS) $(shell pkg-config --cflags QtCore QtGui)
QT4_LINK_FLAGS = $(LINK_FLAGS) $(shell pkg-config --libs QtCore QtGui)
QT4_STYLES_DIR = $(shell pkg-config --variable=libdir QtCore)/qt4/plugins/styles
endif

ifeq ($(HAVE_QT5),true)
QT5_CXX_FLAGS  = $(BUILD_CXX_FLAGS) $(shell pkg-config --cflags Qt5Core Qt5Gui Qt5Widgets)
QT5_LINK_FLAGS = $(LINK_FLAGS) $(shell pkg-config --libs Qt5Core Qt5Gui Qt5Widgets)
QT5_STYLES_DIR = $(shell pkg-config --variable=libdir Qt5Core)/qt5/plugins/styles
endif

# ----------------------------------------------------------------------------------------------------------------------------

ifeq ($(HAVE_QT4),true)
FILES_qt4 = \
	$(OBJDIR)/moc_CarlaStyle.qt4.cpp \
	$(OBJDIR)/moc_CarlaStyleAnimations.qt4.cpp \
	$(OBJDIR)/moc_CarlaStylePlugin.qt4.cpp \
	$(OBJDIR)/moc_CarlaStylePrivate.qt4.cpp \
	$(OBJDIR)/resources.qt4.cpp
endif

ifeq ($(HAVE_QT5),true)
FILES_qt5 = \
	$(OBJDIR)/moc_CarlaStyle.qt5.cpp \
	$(OBJDIR)/moc_CarlaStyleAnimations.qt5.cpp \
	$(OBJDIR)/moc_CarlaStylePlugin.qt5.cpp \
	$(OBJDIR)/moc_CarlaStylePrivate.qt5.cpp \
	$(OBJDIR)/resources.qt5.cpp
endif

# ----------------------------------------------------------------------------------------------------------------------------

ifeq ($(HAVE_QT4),true)
OBJS_qt4 = \
	$(OBJDIR)/CarlaStyle.cpp.qt4.o \
	$(OBJDIR)/moc_CarlaStyle.qt4.cpp.o \
	$(OBJDIR)/moc_CarlaStyleAnimations.qt4.cpp.o \
	$(OBJDIR)/moc_CarlaStylePrivate.qt4.cpp.o \
	$(OBJDIR)/resources.qt4.cpp.o

OBJS_qt4_shared = $(OBJS_qt4) \
	$(OBJDIR)/CarlaStylePlugin.cpp.qt4.o \
	$(OBJDIR)/moc_CarlaStylePlugin.qt4.cpp.o
endif

ifeq ($(HAVE_QT5),true)
OBJS_qt5 = \
	$(OBJDIR)/CarlaStyle.cpp.qt5.o \
	$(OBJDIR)/moc_CarlaStyle.qt5.cpp.o \
	$(OBJDIR)/moc_CarlaStyleAnimations.qt5.cpp.o \
	$(OBJDIR)/moc_CarlaStylePrivate.qt5.cpp.o \
	$(OBJDIR)/resources.qt5.cpp.o

OBJS_qt5_shared = $(OBJS_qt5) \
	$(OBJDIR)/CarlaStylePlugin.cpp.qt5.o \
	$(OBJDIR)/moc_CarlaStylePlugin.qt5.cpp.o
endif

# ----------------------------------------------------------------------------------------------------------------------------

ifeq ($(HAVE_QT5),true)
all: $(BINDIR)/styles/carlastyle$(LIB_EXT) $(BINDIR)/styles/carlastyle.json
else
all:
endif

qt4: $(MODULEDIR)/$(MODULENAME).qt4.a
qt5: $(MODULEDIR)/$(MODULENAME).qt5.a

# ----------------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(MODULEDIR)/$(MODULENAME)*.a $(BINDIR)/styles/carlastyle$(LIB_EXT) $(FILES_qt4) $(FILES_qt5)

debug:
	$(MAKE) DEBUG=true

install: $(BINDIR)/styles/carlastyle$(LIB_EXT) $(BINDIR)/styles/carlastyle.json
	install -d $(QT5_STYLES_DIR)
	install -m 644 $^ $(QT5_STYLES_DIR)

# ----------------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME).qt4.a: $(FILES_qt4) $(OBJS_qt4)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).qt4.a"
	@rm -f $@
	@$(AR) crs $@ $(OBJS_qt4)

$(MODULEDIR)/$(MODULENAME).qt5.a: $(FILES_qt5) $(OBJS_qt5)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).qt5.a"
	@rm -f $@
	@$(AR) crs $@ $(OBJS_qt5)

# ----------------------------------------------------------------------------------------------------------------------------

$(BINDIR)/styles/carlastyle.dll: $(FILES_qt5) $(OBJS_qt5_shared)
	-@mkdir -p $(BINDIR)/styles
	@echo "Linking carlastyle.dll"
	@$(CXX) $(OBJS_qt5_shared) $(QT5_LINK_FLAGS) -luuid -lwinspool -lgdi32 -limm32 -lole32 -lshell32 -lws2_32 -shared -o $@

$(BINDIR)/styles/carlastyle.dylib: $(FILES_qt5) $(OBJS_qt5_shared)
	-@mkdir -p $(BINDIR)/styles
	@echo "Linking carlastyle.dylib"
	@$(CXX) $(OBJS_qt5_shared) $(QT5_LINK_FLAGS) -dynamiclib -o $@

$(BINDIR)/styles/carlastyle.so: $(FILES_qt5) $(OBJS_qt5_shared)
	-@mkdir -p $(BINDIR)/styles
	@echo "Linking carlastyle.so"
	@$(CXX) $(OBJS_qt5_shared) $(QT5_LINK_FLAGS) -shared -o $@

$(BINDIR)/styles/carlastyle.json: styles/carlastyle.json
	-@mkdir -p $(BINDIR)/styles
	@cp -f $< $@

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/moc_%.qt4.cpp: %.hpp
	-@mkdir -p $(OBJDIR)
	@echo "Generating moc_$*.qt4.cpp"
	@$(MOC_QT4) $< -o $@

$(OBJDIR)/moc_%.qt5.cpp: %.hpp
	-@mkdir -p $(OBJDIR)
	@echo "Generating moc_$*.qt5.cpp"
	@$(MOC_QT5) -Istyles $< -o $@

$(OBJDIR)/resources.qt4.cpp: $(CWD)/../resources/resources-theme.qrc
	-@mkdir -p $(OBJDIR)
	@echo "Generating resources.qt4.cpp"
	@$(RCC_QT4) $< -o $@

$(OBJDIR)/resources.qt5.cpp: $(CWD)/../resources/resources-theme.qrc
	-@mkdir -p $(OBJDIR)
	@echo "Generating resources.qt5.cpp"
	@$(RCC_QT5) $< -o $@

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.qt4.cpp.o: $(OBJDIR)/%.qt4.cpp $(OBJDIR)/moc_CarlaStyle.qt4.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $*.cpp (Qt4)"
	@$(CXX) $< $(QT4_CXX_FLAGS) -Wno-unused-variable -c -o $@

$(OBJDIR)/%.qt5.cpp.o: $(OBJDIR)/%.qt5.cpp $(OBJDIR)/moc_CarlaStyle.qt5.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $*.cpp (Qt5)"
	@$(CXX) $< $(QT5_CXX_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.qt4.o: %.cpp $(OBJDIR)/moc_CarlaStyle.qt4.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (Qt4)"
	@$(CXX) $< $(QT4_CXX_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.qt5.o: %.cpp $(OBJDIR)/moc_CarlaStyle.qt5.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (Qt5)"
	@$(CXX) $< $(QT5_CXX_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------

-include $(OBJS_qt4_shared:%.o=%.d)
-include $(OBJS_qt5_shared:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
