#!/usr/bin/make -f
# Makefile for jackbridge #
# ----------------------- #
# Created by falkTX
#

CWD=..
MODULENAME=jackbridge
include ../modules/Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

WINECXX ?= wineg++

BUILD_CXX_FLAGS += $(JACKBRIDGE_FLAGS)
LINK_FLAGS      += $(JACKBRIDGE_LIBS)

ifeq ($(MACOS),true)
WINE_32BIT_FLAGS = $(32BIT_FLAGS)
WINE_64BIT_FLAGS = $(64BIT_FLAGS)
WINE_LINK_FLAGS  = $(LINK_FLAGS) -ldl -lpthread
else
WINE_32BIT_FLAGS = $(32BIT_FLAGS) -L/usr/lib32/wine -L/usr/lib/i386-linux-gnu/wine -L/opt/wine-staging/lib -L/opt/wine-staging/lib/wine/
WINE_64BIT_FLAGS = $(64BIT_FLAGS) -L/usr/lib64/wine -L/usr/lib/x86_64-linux-gnu/wine -L/opt/wine-staging/lib64 -L/opt/wine-staging/lib64/wine/
WINE_LINK_FLAGS  = $(LINK_FLAGS) -ldl -lrt -lpthread
endif

# ----------------------------------------------------------------------------------------------------------------------------

OBJS          = $(OBJDIR)/JackBridge1.cpp.o          $(OBJDIR)/JackBridge2.cpp.o
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

# ----------------------------------------------------------------------------------------------------------------------------

all:      $(MODULEDIR)/$(MODULENAME).a
posix32:  $(MODULEDIR)/$(MODULENAME).posix32.a
posix64:  $(MODULEDIR)/$(MODULENAME).posix64.a
posix32e: $(MODULEDIR)/$(MODULENAME).posix32e.a
posix64e: $(MODULEDIR)/$(MODULENAME).posix64e.a
win32:    $(MODULEDIR)/$(MODULENAME).win32.a
win64:    $(MODULEDIR)/$(MODULENAME).win64.a
win32e:   $(MODULEDIR)/$(MODULENAME).win32e.a
win64e:   $(MODULEDIR)/$(MODULENAME).win64e.a
wine32:   $(MODULEDIR)/$(MODULENAME)-wine32.dll$(LIB_EXT)
wine64:   $(MODULEDIR)/$(MODULENAME)-wine64.dll$(LIB_EXT)

# ----------------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(MODULEDIR)/$(MODULENAME)*.*

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME).a: $(OBJS)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).a"
	@rm -f $@
	@$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME).posix32.a: $(OBJS_posix32)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).posix32.a"
	@rm -f $@
	@$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME).posix64.a: $(OBJS_posix64)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).posix64.a"
	@rm -f $@
	@$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME).win32.a: $(OBJS_win32)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).win32.a"
	@rm -f $@
	@$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME).win64.a: $(OBJS_win64)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).win64.a"
	@rm -f $@
	@$(AR) crs $@ $^

# ----------------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME).posix32e.a: $(OBJS_posix32e)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).posix32e.a"
	@rm -f $@
	@$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME).posix64e.a: $(OBJS_posix64e)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).posix64e.a"
	@rm -f $@
	@$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME).win32e.a: $(OBJS_win32e)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).win32e.a"
	@rm -f $@
	@$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME).win64e.a: $(OBJS_win64e)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).win64e.a"
	@rm -f $@
	@$(AR) crs $@ $^

# ----------------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME)-wine32.dll$(LIB_EXT): $(OBJS_wine32) JackBridgeExport.def
	-@mkdir -p $(MODULEDIR)
	@echo "Linking $(MODULENAME)-wine32.dll$(LIB_EXT)"
	@$(WINECXX) $^ $(WINE_32BIT_FLAGS) $(WINE_LINK_FLAGS) $(SHARED) -o $@

$(MODULEDIR)/$(MODULENAME)-wine64.dll$(LIB_EXT): $(OBJS_wine64) JackBridgeExport.def
	-@mkdir -p $(MODULEDIR)
	@echo "Linking $(MODULENAME)-wine64.dll$(LIB_EXT)"
	@$(WINECXX) $^ $(WINE_64BIT_FLAGS) $(WINE_LINK_FLAGS) $(SHARED) -o $@

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/JackBridge1.cpp.o: JackBridge1.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling JackBridge1.cpp"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(OBJDIR)/JackBridge2.cpp.o: JackBridge2.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling JackBridge2.cpp"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/JackBridgeExport.cpp.%32e.o: JackBridgeExport.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -fpermissive -c -o $@

$(OBJDIR)/JackBridgeExport.cpp.%64e.o: JackBridgeExport.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -fpermissive -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.cpp.posix32.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (posix32)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.posix64.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (posix64)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.win32.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (win32)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.win64.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (win64)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.wine32.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (wine32)"
	@$(WINECXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

$(OBJDIR)/%.cpp.wine64.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (wine64)"
	@$(WINECXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------

-include $(OBJS:%.o=%.d)
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

# ----------------------------------------------------------------------------------------------------------------------------
