#!/usr/bin/make -f
# Makefile for jackbridge #
# ----------------------- #
# Created by falkTX
#

CWD=..
MODULENAME=jackbridge
include ../modules/Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += $(JACKBRIDGE_FLAGS) -I../backend -I../modules
LINK_FLAGS      += $(JACKBRIDGE_LIBS)

ifeq ($(JACKBRIDGE_DIRECT),true)
BUILD_CXX_FLAGS += $(JACK_FLAGS) -DJACKBRIDGE_DIRECT
LINK_FLAGS      += $(JACK_LIBS)
endif

WINE_32BIT_FLAGS = $(32BIT_FLAGS) -fpermissive
WINE_64BIT_FLAGS = $(64BIT_FLAGS) -fpermissive
WINE_BUILD_FLAGS = $(filter-out -flto,$(BUILD_CXX_FLAGS))
WINE_LINK_FLAGS  = $(filter-out -flto -static-libgcc -static-libstdc++,$(LINK_FLAGS))
WINE_LINK_FLAGS += $(LIBDL_LIBS)
WINE_LINK_FLAGS += -pthread
WINE_LINK_FLAGS += -lpthread
WINE_LINK_FLAGS += -lstdc++

ifneq ($(MACOS),true)
WINE_32BIT_FLAGS += -I/usr/include/wine/wine/windows
WINE_32BIT_FLAGS += -I/usr/include/wine-development/windows
WINE_32BIT_FLAGS += -I/opt/wine-devel/include/wine/windows
WINE_32BIT_FLAGS += -L/usr/lib32/wine
WINE_32BIT_FLAGS += -L/usr/lib/wine
WINE_32BIT_FLAGS += -L/usr/lib/i386-linux-gnu/wine
WINE_32BIT_FLAGS += -L/usr/lib/i386-linux-gnu/wine-development
WINE_32BIT_FLAGS += -L/usr/lib/i386-linux-gnu/wine-development/i386-unix
WINE_32BIT_FLAGS += -L/opt/wine-stable/lib
WINE_32BIT_FLAGS += -L/opt/wine-stable/lib/wine
WINE_32BIT_FLAGS += -L/opt/wine-stable/lib/wine/i386-unix
WINE_32BIT_FLAGS += -L/opt/wine-staging/lib
WINE_32BIT_FLAGS += -L/opt/wine-staging/lib/wine
WINE_32BIT_FLAGS += -L/opt/wine-staging/lib/wine/i386-unix

WINE_64BIT_FLAGS += -I/usr/include/wine/wine/windows
WINE_64BIT_FLAGS += -I/usr/include/wine-development/windows
WINE_64BIT_FLAGS += -I/opt/wine-devel/include/wine/windows
WINE_64BIT_FLAGS += -L/usr/lib64/wine
WINE_64BIT_FLAGS += -L/usr/lib/x86_64-linux-gnu/wine
WINE_64BIT_FLAGS += -L/usr/lib/x86_64-linux-gnu/wine-development
WINE_64BIT_FLAGS += -L/usr/lib/x86_64-linux-gnu/wine-development/x86_64-unix
WINE_64BIT_FLAGS += -L/opt/wine-stable/lib64
WINE_64BIT_FLAGS += -L/opt/wine-stable/lib64/wine
WINE_64BIT_FLAGS += -L/opt/wine-stable/lib64/wine/x86_64-unix
WINE_64BIT_FLAGS += -L/opt/wine-staging/lib64
WINE_64BIT_FLAGS += -L/opt/wine-staging/lib64/wine
WINE_64BIT_FLAGS += -L/opt/wine-staging/lib64/wine/x86_64-unix

WINE_LINK_FLAGS  += -lrt
endif

# ---------------------------------------------------------------------------------------------------------------------

OBJS          = $(OBJDIR)/JackBridge1.cpp.o          $(OBJDIR)/JackBridge2.cpp.o
OBJS_min      = $(OBJDIR)/JackBridge2.cpp.o
OBJS_arm32    = $(OBJDIR)/JackBridge1.cpp.arm32.o    $(OBJDIR)/JackBridge2.cpp.arm32.o
OBJS_posix32  = $(OBJDIR)/JackBridge1.cpp.posix32.o  $(OBJDIR)/JackBridge2.cpp.posix32.o
OBJS_posix64  = $(OBJDIR)/JackBridge1.cpp.posix64.o  $(OBJDIR)/JackBridge2.cpp.posix64.o
OBJS_win32    = $(OBJDIR)/JackBridge1.cpp.win32.o    $(OBJDIR)/JackBridge2.cpp.win32.o
OBJS_win64    = $(OBJDIR)/JackBridge1.cpp.win64.o    $(OBJDIR)/JackBridge2.cpp.win64.o
OBJS_wine32   = $(OBJDIR)/JackBridge1.cpp.wine32.o   $(OBJDIR)/JackBridge2.cpp.wine32.o $(OBJDIR)/JackBridge3.cpp.wine32.o
OBJS_wine64   = $(OBJDIR)/JackBridge1.cpp.wine64.o   $(OBJDIR)/JackBridge2.cpp.wine64.o $(OBJDIR)/JackBridge3.cpp.wine64.o

OBJS_posix32e = $(OBJDIR)/JackBridgeExport.cpp.posix32e.o
OBJS_posix64e = $(OBJDIR)/JackBridgeExport.cpp.posix64e.o
OBJS_win64e   = $(OBJDIR)/JackBridgeExport.cpp.win64e.o
OBJS_win32e   = $(OBJDIR)/JackBridgeExport.cpp.win32e.o

# ---------------------------------------------------------------------------------------------------------------------

all: $(MODULEDIR)/$(MODULENAME).a
min: $(MODULEDIR)/$(MODULENAME).min.a

ifeq ($(WINDOWS),true)
posix32:
posix64:
posix32e:
posix64e:
win32:    $(MODULEDIR)/$(MODULENAME).win32.a
win64:    $(MODULEDIR)/$(MODULENAME).win64.a
win32e:   $(MODULEDIR)/$(MODULENAME).win32e.a
win64e:   $(MODULEDIR)/$(MODULENAME).win64e.a
wine32:
wine64:
else
arm32:    $(MODULEDIR)/$(MODULENAME).arm32.a
posix32:  $(MODULEDIR)/$(MODULENAME).posix32.a
posix64:  $(MODULEDIR)/$(MODULENAME).posix64.a
posix32e: $(MODULEDIR)/$(MODULENAME).posix32e.a
posix64e: $(MODULEDIR)/$(MODULENAME).posix64e.a
win32:
win64:
win32e:
win64e:
wine32:   $(MODULEDIR)/$(MODULENAME)-wine32.dll$(LIB_EXT)
wine64:   $(MODULEDIR)/$(MODULENAME)-wine64.dll$(LIB_EXT)
endif

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(MODULEDIR)/$(MODULENAME)*.*

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME).a: $(OBJS)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME).min.a: $(OBJS_min)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).min.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME).arm32.a: $(OBJS_arm32)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).arm32.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME).posix32.a: $(OBJS_posix32)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).posix32.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME).posix64.a: $(OBJS_posix64)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).posix64.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME).win32.a: $(OBJS_win32)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).win32.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME).win64.a: $(OBJS_win64)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).win64.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

# ---------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME).posix32e.a: $(OBJS_posix32e)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).posix32e.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME).posix64e.a: $(OBJS_posix64e)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).posix64e.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME).win32e.a: $(OBJS_win32e)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).win32e.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME).win64e.a: $(OBJS_win64e)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).win64e.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

# ---------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME)-wine32.dll$(LIB_EXT): $(OBJS_wine32) JackBridgeExport.def
	-@mkdir -p $(MODULEDIR)
	@echo "Linking $(MODULENAME)-wine32.dll$(LIB_EXT)"
	$(SILENT)$(WINECC) $^ $(WINE_32BIT_FLAGS) $(WINE_LINK_FLAGS) $(SHARED) -o $@

$(MODULEDIR)/$(MODULENAME)-wine64.dll$(LIB_EXT): $(OBJS_wine64) JackBridgeExport.def
	-@mkdir -p $(MODULEDIR)
	@echo "Linking $(MODULENAME)-wine64.dll$(LIB_EXT)"
	$(SILENT)$(WINECC) $^ $(WINE_64BIT_FLAGS) $(WINE_LINK_FLAGS) $(SHARED) -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/JackBridge1.cpp.o: JackBridge1.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling JackBridge1.cpp"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(OBJDIR)/JackBridge2.cpp.o: JackBridge2.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling JackBridge2.cpp"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/JackBridgeExport.cpp.%32e.o: JackBridgeExport.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -fpermissive -c -o $@

$(OBJDIR)/JackBridgeExport.cpp.%64e.o: JackBridgeExport.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -fpermissive -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.cpp.arm32.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (arm32)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(ARM32_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.posix32.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (posix32)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.posix64.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (posix64)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.win32.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (win32)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.win64.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (win64)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.wine32.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (wine32)"
	$(SILENT)$(WINECC) $< $(WINE_32BIT_FLAGS) $(WINE_BUILD_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.wine64.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (wine64)"
	$(SILENT)$(WINECC) $< $(WINE_64BIT_FLAGS) $(WINE_BUILD_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJS:%.o=%.d)
-include $(OBJS_arm32:%.o=%.d)
-include $(OBJS_posix32:%.o=%.d)
-include $(OBJS_posix32e:%.o=%.d)
-include $(OBJS_posix64:%.o=%.d)
-include $(OBJS_posix64e:%.o=%.d)
-include $(OBJS_win32:%.o=%.d)
-include $(OBJS_win32e:%.o=%.d)
-include $(OBJS_win64:%.o=%.d)
-include $(OBJS_win64e:%.o=%.d)
-include $(OBJS_wine32:%.o=%.d)
-include $(OBJS_wine64:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
