#!/usr/bin/make -f
# Makefile for theme #
# ------------------ #
# Created by falkTX
#

CWD=..
MODULENAME=theme
include ../modules/Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(WINDOWS),true)
NON_STATIC_LINK_FLAGS = $(shell echo $(LINK_FLAGS) | awk 'sub(" -static","")') -static-libgcc
else
NON_STATIC_LINK_FLAGS = $(LINK_FLAGS)
endif

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(HAVE_QT4),true)
FILES_qt4 = \
	$(OBJDIR)/moc_CarlaStyle.qt4.cpp \
	$(OBJDIR)/moc_CarlaStyleAnimations.qt4.cpp \
	$(OBJDIR)/moc_CarlaStylePlugin.qt4.cpp \
	$(OBJDIR)/moc_CarlaStylePrivate.qt4.cpp \
	$(OBJDIR)/resources.qt4.cpp

OBJS_qt4 = \
	$(OBJDIR)/CarlaStyle.cpp.qt4.o \
	$(OBJDIR)/moc_CarlaStyle.qt4.cpp.o \
	$(OBJDIR)/moc_CarlaStyleAnimations.qt4.cpp.o \
	$(OBJDIR)/moc_CarlaStylePrivate.qt4.cpp.o \
	$(OBJDIR)/resources.qt4.cpp.o
endif

ifneq (,$(findstring true,$(HAVE_QT5)$(HAVE_QT5PKG)$(HAVE_QT5BREW)))
FILES_qt5 = \
	$(OBJDIR)/moc_CarlaStyle.qt5.cpp \
	$(OBJDIR)/moc_CarlaStyleAnimations.qt5.cpp \
	$(OBJDIR)/moc_CarlaStylePlugin.qt5.cpp \
	$(OBJDIR)/moc_CarlaStylePrivate.qt5.cpp \
	$(OBJDIR)/resources.qt5.cpp

OBJS_qt5 = \
	$(OBJDIR)/CarlaStyle.cpp.qt5.o \
	$(OBJDIR)/moc_CarlaStyle.qt5.cpp.o \
	$(OBJDIR)/moc_CarlaStyleAnimations.qt5.cpp.o \
	$(OBJDIR)/moc_CarlaStylePrivate.qt5.cpp.o \
	$(OBJDIR)/resources.qt5.cpp.o
endif

ifneq (,$(findstring true,$(HAVE_QT6)$(HAVE_QT6BREW)))
FILES_qt6 = \
	$(OBJDIR)/moc_CarlaStyle.qt6.cpp \
	$(OBJDIR)/moc_CarlaStyleAnimations.qt6.cpp \
	$(OBJDIR)/moc_CarlaStylePlugin.qt6.cpp \
	$(OBJDIR)/moc_CarlaStylePrivate.qt6.cpp \
	$(OBJDIR)/resources.qt6.cpp

OBJS_qt6 = \
	$(OBJDIR)/CarlaStyle.cpp.qt6.o \
	$(OBJDIR)/moc_CarlaStyle.qt6.cpp.o \
	$(OBJDIR)/moc_CarlaStyleAnimations.qt6.cpp.o \
	$(OBJDIR)/moc_CarlaStylePrivate.qt6.cpp.o \
	$(OBJDIR)/resources.qt6.cpp.o
endif

# ---------------------------------------------------------------------------------------------------------------------

OBJS_qt_shared = $(OBJS_qt$(FRONTEND_TYPE)) \
	$(OBJDIR)/CarlaStylePlugin.cpp.qt$(FRONTEND_TYPE).o \
	$(OBJDIR)/moc_CarlaStylePlugin.qt$(FRONTEND_TYPE).cpp.o

# ---------------------------------------------------------------------------------------------------------------------

ifneq (,$(FRONTEND_TYPE))
all: $(BINDIR)/styles/carlastyle$(LIB_EXT) $(BINDIR)/styles/carlastyle.json
else
all:
endif

qt4: $(MODULEDIR)/$(MODULENAME).qt4.a
qt5: $(MODULEDIR)/$(MODULENAME).qt5.a
qt6: $(MODULEDIR)/$(MODULENAME).qt6.a

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(MODULEDIR)/$(MODULENAME)*.a
	rm -f $(BINDIR)/styles/carlastyle$(LIB_EXT) $(FILES_qt4) $(FILES_qt5) $(FILES_qt6)

debug:
	$(MAKE) DEBUG=true

install: $(BINDIR)/styles/carlastyle$(LIB_EXT) $(BINDIR)/styles/carlastyle.json
	install -d $(QT5_STYLES_DIR)
	install -m 644 $^ $(QT5_STYLES_DIR)

# ---------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME).qt4.a: $(FILES_qt4) $(OBJS_qt4)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).qt4.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $(OBJS_qt4)

$(MODULEDIR)/$(MODULENAME).qt5.a: $(FILES_qt5) $(OBJS_qt5)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).qt5.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $(OBJS_qt5)

$(MODULEDIR)/$(MODULENAME).qt6.a: $(FILES_qt6) $(OBJS_qt6)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).qt6.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $(OBJS_qt6)

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/styles/carlastyle.dll: $(FILES_qt5) $(OBJS_qt_shared)
	-@mkdir -p $(BINDIR)/styles
	@echo "Linking carlastyle.dll"
	$(SILENT)$(CXX) $(OBJS_qt_shared) $(NON_STATIC_LINK_FLAGS) $(QT_LINK_FLAGS) -shared -o $@

$(BINDIR)/styles/carlastyle.dylib: $(FILES_qt5) $(OBJS_qt_shared)
	-@mkdir -p $(BINDIR)/styles
	@echo "Linking carlastyle.dylib"
	$(SILENT)$(CXX) $(OBJS_qt_shared) $(NON_STATIC_LINK_FLAGS) $(QT_LINK_FLAGS) -dynamiclib -o $@

$(BINDIR)/styles/carlastyle.so: $(FILES_qt5) $(OBJS_qt_shared)
	-@mkdir -p $(BINDIR)/styles
	@echo "Linking carlastyle.so"
	$(SILENT)$(CXX) $(OBJS_qt_shared) $(NON_STATIC_LINK_FLAGS) $(QT_LINK_FLAGS) -shared -o $@

$(BINDIR)/styles/carlastyle.json: styles/carlastyle.json
	-@mkdir -p $(BINDIR)/styles
	$(SILENT)cp -f $< $@

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/moc_%.qt4.cpp: %.hpp
	-@mkdir -p $(OBJDIR)
	@echo "Generating moc_$*.qt4.cpp"
	$(SILENT)$(MOC_QT4) $< -o $@

$(OBJDIR)/moc_%.qt5.cpp: %.hpp
	-@mkdir -p $(OBJDIR)
	@echo "Generating moc_$*.qt5.cpp"
	$(SILENT)$(MOC_QT5) -Istyles $< -o $@

$(OBJDIR)/moc_%.qt6.cpp: %.hpp
	-@mkdir -p $(OBJDIR)
	@echo "Generating moc_$*.qt6.cpp"
	$(SILENT)$(MOC_QT6) -Istyles $< -o $@

$(OBJDIR)/resources.qt4.cpp: $(CWD)/../resources/resources-theme.qrc
	-@mkdir -p $(OBJDIR)
	@echo "Generating resources.qt4.cpp"
	$(SILENT)$(RCC_QT4) $< -o $@

$(OBJDIR)/resources.qt5.cpp: $(CWD)/../resources/resources-theme.qrc
	-@mkdir -p $(OBJDIR)
	@echo "Generating resources.qt5.cpp"
	$(SILENT)$(RCC_QT5) $< -o $@

$(OBJDIR)/resources.qt6.cpp: $(CWD)/../resources/resources-theme.qrc
	-@mkdir -p $(OBJDIR)
	@echo "Generating resources.qt6.cpp"
	$(SILENT)$(RCC_QT6) $< -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.qt4.cpp.o: $(OBJDIR)/%.qt4.cpp $(OBJDIR)/moc_CarlaStyle.qt4.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $*.cpp (Qt4)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(QT4_CXX_FLAGS) -Wno-unused-variable -c -o $@

$(OBJDIR)/%.qt5.cpp.o: $(OBJDIR)/%.qt5.cpp $(OBJDIR)/moc_CarlaStyle.qt5.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $*.cpp (Qt5)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(QT5_CXX_FLAGS) -c -o $@

$(OBJDIR)/%.qt6.cpp.o: $(OBJDIR)/%.qt6.cpp $(OBJDIR)/moc_CarlaStyle.qt6.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $*.cpp (Qt6)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(QT6_CXX_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.qt4.o: %.cpp $(OBJDIR)/moc_CarlaStyle.qt4.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (Qt4)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(QT4_CXX_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.qt5.o: %.cpp $(OBJDIR)/moc_CarlaStyle.qt5.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (Qt5)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(QT5_CXX_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.qt6.o: %.cpp $(OBJDIR)/moc_CarlaStyle.qt6.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (Qt6)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(QT6_CXX_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJS_qt4:%.o=%.d)
-include $(OBJS_qt5:%.o=%.d)
-include $(OBJS_qt6:%.o=%.d)
-include $(OBJS_qt_shared:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
