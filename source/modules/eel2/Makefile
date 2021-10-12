#!/usr/bin/make -f
# Makefile for eel2 #
# ----------------- #
# Created by falkTX
#

CWD=../..
MODULENAME=eel2
include ../Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

BUILD_C_FLAGS += $(EEL2_FLAGS)
BUILD_C_FLAGS += -Wno-unused-parameter
BUILD_C_FLAGS += -Wno-sign-compare
BUILD_C_FLAGS += -Wno-missing-field-initializers
BUILD_C_FLAGS += -Wno-old-style-declaration
BUILD_C_FLAGS += -Wno-implicit-fallthrough
BUILD_C_FLAGS += -Wno-empty-body

ifneq ($(WIN32),true)
# NOTE: not compatible with MingGW, breaks win32_utf8
BUILD_C_FLAGS += -D_FILE_OFFSET_BITS=64
endif

# ---------------------------------------------------------------------------------------------------------------------

OBJS  = $(OBJDIR)/source/WDL/eel2/nseel-caltab.c.o
OBJS += $(OBJDIR)/source/WDL/eel2/nseel-cfunc.c.o
OBJS += $(OBJDIR)/source/WDL/eel2/nseel-compiler.c.o
OBJS += $(OBJDIR)/source/WDL/eel2/nseel-eval.c.o
OBJS += $(OBJDIR)/source/WDL/eel2/nseel-lextab.c.o
OBJS += $(OBJDIR)/source/WDL/eel2/nseel-ram.c.o
OBJS += $(OBJDIR)/source/WDL/eel2/nseel-yylex.c.o
OBJS += $(OBJDIR)/source/WDL/fft.c.o
ifeq ($(WIN32),true)
OBJS += $(OBJDIR)/source/WDL/win32_utf8.c.o
endif

ifeq ($(CPU_X86_64),true)
ifeq ($(WIN32),true)
OBJS += $(OBJDIR)/source/WDL/eel2/asm-nseel-x64-sse.asm.win64.o
else ifeq ($(MACOS),true)
OBJS += $(OBJDIR)/source/WDL/eel2/asm-nseel-x64-sse.asm.mac64.o
else
OBJS += $(OBJDIR)/source/WDL/eel2/asm-nseel-x64-sse.asm.posix64.o
endif
endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(MODULEDIR)/$(MODULENAME).a

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJDIR)/source/WDL/*.o $(OBJDIR)/source/WDL/eel2/*.o $(MODULEDIR)/$(MODULENAME)*.a

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

$(MODULEDIR)/$(MODULENAME).a: $(OBJS)
	-@mkdir -p $(MODULEDIR)
	@echo "Creating $(MODULENAME).a"
	@rm -f $@
	@$(AR) crs $@ $^

# ---------------------------------------------------------------------------------------------------------------------

$(OBJDIR)/%.c.o: %.c
	-@mkdir -p $(dir $@)
	@echo "Compiling $<"
	@$(CC) $< $(BUILD_C_FLAGS) -c -o $@

$(OBJDIR)/%.asm.win64.o: %.S #%.asm
	-@mkdir -p $(dir $@)
	@echo "Compiling $<"
	@$(CC) -c -o $@ $<
#	@$(NASM) -f win64 -o $@ $<

$(OBJDIR)/%.asm.mac64.o: %.S #%.asm
	-@mkdir -p $(dir $@)
	@echo "Compiling $<"
	@$(CC) -DAMD64ABI -DPREFIX=_ -c -o $@ $<
#	@$(NASM) -D AMD64ABI -f macho64 --prefix _ -o $@ $<

$(OBJDIR)/%.asm.posix64.o: %.S #%.asm
	-@mkdir -p $(dir $@)
	@echo "Compiling $<"
	@$(CC) -DAMD64ABI -c -o $@ $<
#	@$(NASM) -D AMD64ABI -f elf64 -o $@ $<

-include $(OBJS:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
