#!/usr/bin/make -f
# Makefile for lilv #
# ----------------- #
# Created by falkTX
#

CWD=../..
MODULENAME=lilv
include ../Makefile.mk

# ----------------------------------------------------------------------------------------------------------------------------

SERD_VERSION   = 0.21.1
SORD_VERSION   = 0.13.0
SRATOM_VERSION = 0.4.7
LILV_VERSION   = 0.21.3

# ----------------------------------------------------------------------------------------------------------------------------

BUILD_C_FLAGS += $(LILV_FLAGS) -Iconfig -w

# ----------------------------------------------------------------------------------------------------------------------------

OBJS         = $(OBJDIR)/serd.c.o         $(OBJDIR)/sord.c.o         $(OBJDIR)/sratom.c.o         $(OBJDIR)/lilv.c.o
OBJS_posix32 = $(OBJDIR)/serd.c.posix32.o $(OBJDIR)/sord.c.posix32.o $(OBJDIR)/sratom.c.posix32.o $(OBJDIR)/lilv.c.posix32.o
OBJS_posix64 = $(OBJDIR)/serd.c.posix64.o $(OBJDIR)/sord.c.posix64.o $(OBJDIR)/sratom.c.posix64.o $(OBJDIR)/lilv.c.posix64.o
OBJS_win32   = $(OBJDIR)/serd.c.win32.o   $(OBJDIR)/sord.c.win32.o   $(OBJDIR)/sratom.c.win32.o   $(OBJDIR)/lilv.c.win32.o
OBJS_win64   = $(OBJDIR)/serd.c.win64.o   $(OBJDIR)/sord.c.win64.o   $(OBJDIR)/sratom.c.win64.o   $(OBJDIR)/lilv.c.win64.o

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

$(OBJDIR)/serd.c.o: serd.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CC) $< $(BUILD_C_FLAGS) -Iserd-$(SERD_VERSION) -c -o $@

$(OBJDIR)/serd.c.%32.o: serd.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (32bit)"
	@$(CC) $< $(BUILD_C_FLAGS) -Iserd-$(SERD_VERSION) $(32BIT_FLAGS) -c -o $@

$(OBJDIR)/serd.c.%64.o: serd.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (64bit)"
	@$(CC) $< $(BUILD_C_FLAGS) -Iserd-$(SERD_VERSION) $(64BIT_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/sord.c.o: sord.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CC) $< $(BUILD_C_FLAGS) -Isord-$(SORD_VERSION) -Isord-$(SORD_VERSION)/src -c -o $@

$(OBJDIR)/sord.c.%32.o: sord.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (32bit)"
	@$(CC) $< $(BUILD_C_FLAGS) -Isord-$(SORD_VERSION) -Isord-$(SORD_VERSION)/src $(32BIT_FLAGS) -c -o $@

$(OBJDIR)/sord.c.%64.o: sord.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (64bit)"
	@$(CC) $< $(BUILD_C_FLAGS) -Isord-$(SORD_VERSION) -Isord-$(SORD_VERSION)/src $(64BIT_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/sratom.c.o: sratom.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CC) $< $(BUILD_C_FLAGS) -Isratom-$(SRATOM_VERSION) -c -o $@

$(OBJDIR)/sratom.c.%32.o: sratom.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (32bit)"
	@$(CC) $< $(BUILD_C_FLAGS) -Isratom-$(SRATOM_VERSION) $(32BIT_FLAGS) -c -o $@

$(OBJDIR)/sratom.c.%64.o: sratom.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (64bit)"
	@$(CC) $< $(BUILD_C_FLAGS) -Isratom-$(SRATOM_VERSION) $(64BIT_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/lilv.c.o: lilv.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	@$(CC) $< $(BUILD_C_FLAGS) -Ililv-$(LILV_VERSION) -Ililv-$(LILV_VERSION)/src -c -o $@

$(OBJDIR)/lilv.c.%32.o: lilv.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (32bit)"
	@$(CC) $< $(BUILD_C_FLAGS) -Ililv-$(LILV_VERSION) -Ililv-$(LILV_VERSION)/src $(32BIT_FLAGS) -c -o $@

$(OBJDIR)/lilv.c.%64.o: lilv.c
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $< (64bit)"
	@$(CC) $< $(BUILD_C_FLAGS) -Ililv-$(LILV_VERSION) -Ililv-$(LILV_VERSION)/src $(64BIT_FLAGS) -c -o $@

# ----------------------------------------------------------------------------------------------------------------------------

-include $(OBJS:%.o=%.d)
-include $(OBJS_posix32:%.o=%.d)
-include $(OBJS_posix64:%.o=%.d)
-include $(OBJS_win32:%.o=%.d)
-include $(OBJS_win64:%.o=%.d)

# ----------------------------------------------------------------------------------------------------------------------------
