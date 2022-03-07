#!/usr/bin/make -f
# Makefile for rtmempool #
# ---------------------- #
# Created by falkTX
#

CWD=../..
MODULENAME=rtmempool
include ../Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

BUILD_C_FLAGS += $(RTMEMPOOL_FLAGS)

# ---------------------------------------------------------------------------------------------------------------------

OBJS         = $(OBJDIR)/$(MODULENAME).c.o
OBJS_arm32   = $(OBJDIR)/$(MODULENAME).c.arm32.o
OBJS_posix32 = $(OBJDIR)/$(MODULENAME).c.posix32.o
OBJS_posix64 = $(OBJDIR)/$(MODULENAME).c.posix64.o
OBJS_win32   = $(OBJDIR)/$(MODULENAME).c.win32.o
OBJS_win64   = $(OBJDIR)/$(MODULENAME).c.win64.o

# ---------------------------------------------------------------------------------------------------------------------

all:     $(MODULEDIR)/$(MODULENAME).a
arm32:   $(MODULEDIR)/$(MODULENAME).arm32.a
posix32: $(MODULEDIR)/$(MODULENAME).posix32.a
posix64: $(MODULEDIR)/$(MODULENAME).posix64.a
win32:   $(MODULEDIR)/$(MODULENAME).win32.a
win64:   $(MODULEDIR)/$(MODULENAME).win64.a

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/*.o $(MODULEDIR)/$(MODULENAME)*.a

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME).a: $(OBJS)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).a"
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

$(OBJDIR)/$(MODULENAME).c.o: $(MODULENAME).c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CC) $< $(BUILD_C_FLAGS) -c -o $@

$(OBJDIR)/$(MODULENAME).c.%32.o: $(MODULENAME).c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (32bit)"
	$(SILENT)$(CC) $< $(BUILD_C_FLAGS) $(32BIT_FLAGS) -c -o $@

$(OBJDIR)/$(MODULENAME).c.%64.o: $(MODULENAME).c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (64bit)"
	$(SILENT)$(CC) $< $(BUILD_C_FLAGS) $(64BIT_FLAGS) -c -o $@

$(OBJDIR)/$(MODULENAME).c.arm32.o: $(MODULENAME).c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (ARM32)"
	$(SILENT)$(CC) $< $(BUILD_C_FLAGS) $(ARM32_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJS:%.o=%.d)
-include $(OBJS_arm32:%.o=%.d)
-include $(OBJS_posix32:%.o=%.d)
-include $(OBJS_posix64:%.o=%.d)
-include $(OBJS_win32:%.o=%.d)
-include $(OBJS_win64:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
