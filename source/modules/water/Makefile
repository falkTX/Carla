#!/usr/bin/make -f
# Makefile for water #
# ----------------------------- #
# Created by falkTX
#

CWD=../..
MODULENAME=water
include ../Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += -I..

# ----------------------------------------------------------------------------------------------------------------------------

ifeq ($(MACOS),true)
OBJS         = $(OBJDIR)/$(MODULENAME).mm.o
OBJS_files   = $(OBJDIR)/$(MODULENAME).mm.files.o
OBJS_posix32 = $(OBJDIR)/$(MODULENAME).mm.posix32.o
OBJS_posix64 = $(OBJDIR)/$(MODULENAME).mm.posix64.o
else
OBJS         = $(OBJDIR)/$(MODULENAME).cpp.o
OBJS_files   = $(OBJDIR)/$(MODULENAME).cpp.files.o
OBJS_posix32 = $(OBJDIR)/$(MODULENAME).cpp.posix32.o
OBJS_posix64 = $(OBJDIR)/$(MODULENAME).cpp.posix64.o
endif
OBJS_win32   = $(OBJDIR)/$(MODULENAME).cpp.win32.o
OBJS_win64   = $(OBJDIR)/$(MODULENAME).cpp.win64.o

# ----------------------------------------------------------------------------------------------------------------------------

all:     $(MODULEDIR)/$(MODULENAME).a $(MODULEDIR)/$(MODULENAME).files.a
posix32: $(MODULEDIR)/$(MODULENAME).posix32.a
posix64: $(MODULEDIR)/$(MODULENAME).posix64.a
win32:   $(MODULEDIR)/$(MODULENAME).win32.a
win64:   $(MODULEDIR)/$(MODULENAME).win64.a

# ----------------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(MODULEDIR)/$(MODULENAME)*.a

debug:
	$(MAKE) DEBUG=true

# ----------------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME).a: $(OBJS)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).a"
	@rm -f $@
	@$(AR) crs $@ $^

$(MODULEDIR)/$(MODULENAME).files.a: $(OBJS_files)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).files.a"
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

$(OBJDIR)/$(MODULENAME).cpp.o: $(MODULENAME).cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(OBJDIR)/$(MODULENAME).cpp.files.o: $(MODULENAME).files.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(OBJDIR)/$(MODULENAME).cpp.%32.o: $(MODULENAME).cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (32bit)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -c -o $@

$(OBJDIR)/$(MODULENAME).cpp.%64.o: $(MODULENAME).cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (64bit)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/$(MODULENAME).mm.o: $(MODULENAME).mm $(MODULENAME).cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@

$(OBJDIR)/$(MODULENAME).mm.files.o: $(MODULENAME).files.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@

$(OBJDIR)/$(MODULENAME).mm.%32.o: $(MODULENAME).cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (32bit)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(32BIT_FLAGS) -ObjC++ -c -o $@

$(OBJDIR)/$(MODULENAME).mm.%64.o: $(MODULENAME).cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (64bit)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(64BIT_FLAGS) -ObjC++ -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------

-include $(OBJS:%.o=%.d)
-include $(OBJS_files:%.o=%.d)
-include $(OBJS_posix32:%.o=%.d)
-include $(OBJS_posix64:%.o=%.d)
-include $(OBJS_win32:%.o=%.d)
-include $(OBJS_win64:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
