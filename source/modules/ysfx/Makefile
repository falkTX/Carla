#!/usr/bin/make -f
# Makefile for eel2 #
# ----------------- #
# Created by falkTX
#

CWD=../..
MODULENAME=ysfx
include ../Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

BUILD_C_FLAGS += $(YSFX_FLAGS)
BUILD_C_FLAGS += -Iinclude -Isources
BUILD_C_FLAGS += -Ithirdparty/WDL/source -Ithirdparty/dr_libs -Ithirdparty/stb
BUILD_C_FLAGS += -DYSFX_NO_STANDARD_MUTEX
BUILD_C_FLAGS += -DEELSCRIPT_NO_NET
BUILD_C_FLAGS += -DEELSCRIPT_NO_LICE
BUILD_C_FLAGS += -DWDL_FFT_REALSIZE=8
BUILD_C_FLAGS += -DWDL_LINEPARSE_ATOF=ysfx_wdl_atof
BUILD_C_FLAGS += -DNSEEL_ATOF=ysfx_wdl_atof

BUILD_CXX_FLAGS += $(YSFX_FLAGS)
BUILD_CXX_FLAGS += -Iinclude -Isources
BUILD_CXX_FLAGS += -Ithirdparty/WDL/source -Ithirdparty/dr_libs -Ithirdparty/stb
BUILD_CXX_FLAGS += -DYSFX_NO_STANDARD_MUTEX
BUILD_CXX_FLAGS += -DEELSCRIPT_NO_NET
BUILD_CXX_FLAGS += -DEELSCRIPT_NO_LICE
BUILD_CXX_FLAGS += -DWDL_FFT_REALSIZE=8
BUILD_CXX_FLAGS += -DWDL_LINEPARSE_ATOF=ysfx_wdl_atof
BUILD_CXX_FLAGS += -DNSEEL_ATOF=ysfx_wdl_atof

ifneq ($(WIN32),true)
# NOTE: not compatible with MingGW, breaks win32_utf8
BUILD_C_FLAGS += -D_FILE_OFFSET_BITS=64
BUILD_CXX_FLAGS += -D_FILE_OFFSET_BITS=64
endif

ifeq ($(WIN32),true)
BUILD_C_FLAGS += -DNOMINMAX
BUILD_CXX_FLAGS += -DNOMINMAX
endif

ifeq ($(HAVE_YSFXGUI),true)
ifneq ($(MACOS_OR_WIN32),true)
BUILD_C_FLAGS += -DSWELL_LICE_GDI -DSWELL_FONTCONFIG -DSWELL_FREETYPE
BUILD_CXX_FLAGS += -DSWELL_LICE_GDI -DSWELL_FONTCONFIG -DSWELL_FREETYPE
endif
else
BUILD_CXX_FLAGS += -DYSFX_NO_GFX
endif

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
ifeq ($(WIN32),true)
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/win32_utf8.c.o
endif
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/lice/lice.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/lice/lice_arc.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/lice/lice_colorspace.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/lice/lice_image.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/lice/lice_line.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/lice/lice_palette.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/lice/lice_texgen.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/lice/lice_text.cpp.o
OBJS += $(OBJDIR)/thirdparty/WDL/source/WDL/lice/lice_textnew.cpp.o
OBJS += $(OBJDIR)/sources/lice_stb/lice_stb_loaders.cpp.o
OBJS += $(OBJDIR)/sources/lice_stb/lice_stb_bmp.cpp.o
OBJS += $(OBJDIR)/sources/lice_stb/lice_stb_gif.cpp.o
OBJS += $(OBJDIR)/sources/lice_stb/lice_stb_jpg.cpp.o
OBJS += $(OBJDIR)/sources/lice_stb/lice_stb_png.cpp.o
OBJS += $(OBJDIR)/sources/lice_stb/lice_stb_write.cpp.o

ifeq ($(HAVE_YSFXGUI),true)
ifneq ($(WIN32),true)
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
endif # !WIN32
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
	@$(CC) $< $(BUILD_ASM_FLAGS) -c -o $@

-include $(OBJS:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
