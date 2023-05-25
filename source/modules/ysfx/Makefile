#!/usr/bin/make -f
# Makefile for eel2 #
# ----------------- #
# Created by falkTX
#

CWD=../..
MODULENAME=ysfx
include ../Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

YSFX_FLAGS += -fsigned-char
YSFX_FLAGS += -Iinclude -Isources
YSFX_FLAGS += -Ithirdparty/WDL/source -Ithirdparty/dr_libs -Ithirdparty/stb
YSFX_FLAGS += -DEELSCRIPT_NO_NET
YSFX_FLAGS += -DEELSCRIPT_NO_LICE
YSFX_FLAGS += -DNSEEL_ATOF=ysfx_wdl_atof
YSFX_FLAGS += -DWDL_FFT_REALSIZE=8
YSFX_FLAGS += -DWDL_LINEPARSE_ATOF=ysfx_wdl_atof
YSFX_FLAGS += -DWDL_WIN32_UTF8_NO_UI_IMPL
YSFX_FLAGS += -DYSFX_API=
YSFX_FLAGS += -DYSFX_NO_STANDARD_MUTEX

# FIXME so many warnings..
# YSFX_FLAGS += -Wno-missing-field-initializers
YSFX_FLAGS += -Wno-ignored-attributes
YSFX_FLAGS += -Wno-sign-compare
YSFX_FLAGS += -Wno-unused-function
YSFX_FLAGS += -Wno-unused-parameter

ifeq ($(CPU_I386_OR_X86_64),true)
ifneq ($(WASM),true)
YSFX_NATIVELY_OPTIMIZED = true
endif
else ifeq ($(CPU_ARM64),true)
YSFX_NATIVELY_OPTIMIZED = true
endif

ifeq ($(YSFX_FTS_LACKS_LFS_SUPPORT),true)
YSFX_FLAGS += -DYSFX_FTS_LACKS_LFS_SUPPORT
endif

ifneq ($(YSFX_NATIVELY_OPTIMIZED),true)
# missing optimized assembly code or not running properly yet, use portable slower approach
YSFX_FLAGS += -DEEL_TARGET_PORTABLE
endif

ifeq ($(WINDOWS),true)
YSFX_FLAGS += -DNOMINMAX -UUNICODE -U_UNICODE
else
# Not compatible with MingGW
YSFX_FLAGS += -D_FILE_OFFSET_BITS=64
endif

ifeq ($(HAVE_YSFXGUI),true)
ifneq ($(MACOS_OR_WINDOWS),true)
YSFX_FLAGS += -DSWELL_LICE_GDI -DSWELL_FONTCONFIG -DSWELL_FREETYPE
endif
else
YSFX_FLAGS += -DYSFX_NO_GFX
endif

ifneq ($(HAIKU),true)
ifneq ($(WASM),true)
YSFX_FLAGS += -pthread
endif
endif

BUILD_C_FLAGS += $(YSFX_FLAGS)
BUILD_CXX_FLAGS += $(YSFX_FLAGS)

# ---------------------------------------------------------------------------------------------------------------------

OBJS  = $(OBJDIR)/sources/ysfx.cpp.o
OBJS += $(OBJDIR)/sources/ysfx_api_eel.cpp.o
OBJS += $(OBJDIR)/sources/ysfx_api_file.cpp.o
OBJS += $(OBJDIR)/sources/ysfx_api_gfx.cpp.o
OBJS += $(OBJDIR)/sources/ysfx_api_reaper.cpp.o
OBJS += $(OBJDIR)/sources/ysfx_audio_flac.cpp.o
OBJS += $(OBJDIR)/sources/ysfx_audio_wav.cpp.o
OBJS += $(OBJDIR)/sources/ysfx_config.cpp.o
OBJS += $(OBJDIR)/sources/ysfx_eel_utils.cpp.o
OBJS += $(OBJDIR)/sources/ysfx_midi.cpp.o
OBJS += $(OBJDIR)/sources/ysfx_parse.cpp.o
OBJS += $(OBJDIR)/sources/ysfx_reader.cpp.o
OBJS += $(OBJDIR)/sources/ysfx_utils.cpp.o
OBJS += $(OBJDIR)/sources/ysfx_utils_fts.cpp.o
OBJS += $(OBJDIR)/sources/eel2-gas/sources/asm-nseel-x64-sse.S.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/eel2/nseel-caltab.c.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/eel2/nseel-cfunc.c.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/eel2/nseel-compiler.c.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/eel2/nseel-eval.c.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/eel2/nseel-lextab.c.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/eel2/nseel-ram.c.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/eel2/nseel-yylex.c.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/fft.c.o
ifeq ($(WINDOWS),true)
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/win32_utf8.c.o
endif

ifeq ($(HAVE_YSFXGUI),true)
OBJS += $(OBJDIR)/sources/lice_stb/lice_stb_loaders.cpp.o
OBJS += $(OBJDIR)/sources/lice_stb/lice_stb_bmp.cpp.o
OBJS += $(OBJDIR)/sources/lice_stb/lice_stb_gif.cpp.o
OBJS += $(OBJDIR)/sources/lice_stb/lice_stb_jpg.cpp.o
OBJS += $(OBJDIR)/sources/lice_stb/lice_stb_png.cpp.o
OBJS += $(OBJDIR)/sources/lice_stb/lice_stb_write.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/lice/lice.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/lice/lice_arc.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/lice/lice_colorspace.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/lice/lice_image.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/lice/lice_line.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/lice/lice_palette.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/lice/lice_texgen.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/lice/lice_text.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/lice/lice_textnew.cpp.o
ifneq ($(WINDOWS),true)
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-ini.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell.cpp.o
ifeq ($(MACOS),true)
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-appstub.mm.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-dlg.mm.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-gdi.mm.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-kb.mm.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-menu.mm.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-misc.mm.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-miscdlg.mm.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-modstub.mm.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-wnd.mm.o
else # MACOS
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-appstub-generic.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-dlg-generic.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-gdi-generic.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-gdi-lice.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-generic-gdk.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-generic-headless.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-kb-generic.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-menu-generic.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-misc-generic.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-miscdlg-generic.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-modstub-generic.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/swell/swell-wnd-generic.cpp.o
endif # !WINDOWS
endif
endif # HAVE_YSFXGUI

# ---------------------------------------------------------------------------------------------------------------------

all: $(MODULEDIR)/$(MODULENAME).a

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJS) $(MODULEDIR)/$(MODULENAME)*.a

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

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(dir $@)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(OBJDIR)/%.mm.o: %.mm
	-@mkdir -p $(dir $@)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@

$(OBJDIR)/%.S.o: %.S
	-@mkdir -p $(dir $@)
	@echo "Compiling $<"
	@$(CC) $< $(BUILD_C_FLAGS) -c -o $@

-include $(OBJS:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
