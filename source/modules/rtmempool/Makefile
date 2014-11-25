#!/usr/bin/make -f
# Makefile for rtmempool #
# ---------------------- #
# Created by falkTX
#

CWD=../..
MODULENAME=rtmempool
include ../Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_C_FLAGS += $(RTMEMPOOL_FLAGS)

# ----------------------------------------------------------------------------------------------------------------------------

OBJS         = $(OBJDIR)/$(MODULENAME).c.o
OBJS_posix32 = $(OBJDIR)/$(MODULENAME).c.posix32.o
OBJS_posix64 = $(OBJDIR)/$(MODULENAME).c.posix64.o
OBJS_win32   = $(OBJDIR)/$(MODULENAME).c.win32.o
OBJS_win64   = $(OBJDIR)/$(MODULENAME).c.win64.o

# ----------------------------------------------------------------------------------------------------------------------------

all:     $(MODULEDIR)/$(MODULENAME).a
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

$(OBJDIR)/$(MODULENAME).c.o: $(MODULENAME).c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CC) $< $(BUILD_C_FLAGS) -MMD -c -o $@

$(OBJDIR)/$(MODULENAME).c.%32.o: $(MODULENAME).c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (32bit)"
	@$(CC) $< $(BUILD_C_FLAGS) $(32BIT_FLAGS) -MMD -c -o $@

$(OBJDIR)/$(MODULENAME).c.%64.o: $(MODULENAME).c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (64bit)"
	@$(CC) $< $(BUILD_C_FLAGS) $(64BIT_FLAGS) -MMD -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------

-include $(OBJS:%.o=%.d)
-include $(OBJS_posix32:%.o=%.d)
-include $(OBJS_posix64:%.o=%.d)
-include $(OBJS_win32:%.o=%.d)
-include $(OBJS_win64:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
