#!/usr/bin/make -f
# Makefile for native-plugins #
# --------------------------- #
# Created by falkTX
#

ifeq ($(TESTBUILD),true)
ifeq ($(LINUX),true)
CXXFLAGS += -isystem /opt/kxstudio/include/ntk
endif
endif

ifeq ($(MACOS_OR_WIN32),true)
HAVE_DGL = true
SKIP_ZYN_SYNTH = true
else
HAVE_DGL = $(shell pkg-config --exists gl x11 && echo true)
endif

HAVE_NTK      = $(shell pkg-config --exists ntk ntk_images && echo true)
HAVE_PROJECTM = $(shell pkg-config --exists libprojectM && echo true)
ifneq ($(MACOS_OLD),true)
HAVE_ZYN_DEPS = $(shell pkg-config --exists liblo fftw3 mxml zlib && echo true)
endif

# ---------------------------------------------------------------------------------------------------------------------
# Check for optional libs (special non-pkgconfig unix tests)

ifeq ($(UNIX),true)

# fltk doesn't have a pkg-config file but has fltk-config instead.
# Also, don't try looking for it if we already have NTK.
ifneq ($(HAVE_NTK),true)
ifeq ($(shell which fltk-config 1>/dev/null 2>/dev/null && echo true),true)
ifeq ($(shell which fluid 1>/dev/null 2>/dev/null && echo true),true)
HAVE_FLTK = true
endif
endif
endif

endif

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(HAVE_FLTK),true)
HAVE_ZYN_UI_DEPS = true
endif
ifeq ($(HAVE_NTK),true)
HAVE_ZYN_UI_DEPS = true
endif

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(HAVE_DGL),true)
BASE_FLAGS += -DHAVE_DGL
endif

ifeq ($(HAVE_PROJECTM),true)
BASE_FLAGS += -DHAVE_PROJECTM
endif

ifeq ($(HAVE_ZYN_DEPS),true)
BASE_FLAGS += -DHAVE_ZYN_DEPS
ifeq ($(HAVE_ZYN_UI_DEPS),true)
BASE_FLAGS += -DHAVE_ZYN_UI_DEPS
endif
endif

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(HAVE_DGL),true)

ifeq ($(MACOS_OR_WIN32),true)
ifeq ($(MACOS),true)
DGL_LIBS   = -framework OpenGL -framework Cocoa
endif
ifeq ($(WIN32),true)
DGL_LIBS   = -lopengl32 -lgdi32
endif
else

DGL_FLAGS  = $(shell pkg-config --cflags gl x11)
DGL_LIBS   = $(shell pkg-config --libs gl x11)
endif

DGL_FLAGS += -DDGL_NAMESPACE=CarlaDGL -DDGL_FILE_BROWSER_DISABLED -DDGL_NO_SHARED_RESOURCES
endif

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(HAVE_PROJECTM),true)
PROJECTM_FLAGS = $(shell pkg-config --cflags libprojectM)
PROJECTM_LIBS  = $(shell pkg-config --libs libprojectM)
endif

# ---------------------------------------------------------------------------------------------------------------------
# Flags for DPF Plugins

DPF_FLAGS  = -I$(CWDE)/modules/distrho

ifeq ($(HAVE_DGL),true)
ifneq ($(MACOS_OR_WIN32),true)
DPF_FLAGS += $(shell pkg-config --cflags gl)
endif
DPF_FLAGS += -I$(CWDE)/modules/dgl -DDGL_NAMESPACE=CarlaDGL -DDGL_FILE_BROWSER_DISABLED -DDGL_NO_SHARED_RESOURCES
endif

# ---------------------------------------------------------------------------------------------------------------------
# Flags for ZynAddSubFX (DSP and UI separated)

ifeq ($(HAVE_ZYN_DEPS),true)

# Common flags
ZYN_BASE_FLAGS  = $(shell pkg-config --cflags liblo mxml)
ZYN_BASE_FLAGS += -Izynaddsubfx -Izynaddsubfx/rtosc
ifneq ($(WIN32),true)
ZYN_BASE_FLAGS += -DHAVE_ASYNC
endif

ZYN_BASE_LIBS   = $(shell pkg-config --libs liblo mxml) -lpthread
ZYN_BASE_LIBS  += $(LIBDL_LIBS)

# DSP flags
ZYN_DSP_FLAGS  = $(ZYN_BASE_FLAGS)
ZYN_DSP_FLAGS += $(shell pkg-config --cflags fftw3 zlib)
ZYN_DSP_LIBS   = $(ZYN_BASE_LIBS)
ZYN_DSP_LIBS  += $(shell pkg-config --libs fftw3 zlib)

ifeq ($(SKIP_ZYN_SYNTH),true)
ZYN_DSP_LIBS  += -DSKIP_ZYN_SYNTH
else
# UI flags
ifeq ($(HAVE_ZYN_UI_DEPS),true)

# Common UI flags
ZYN_UI_FLAGS  = $(ZYN_BASE_FLAGS)
ZYN_UI_LIBS   = $(ZYN_BASE_LIBS)

# NTK or FLTK UI flags
ifeq ($(HAVE_NTK),true)
FLUID          = ntk-fluid
ZYN_UI_FLAGS  += $(shell pkg-config --cflags ntk_images ntk) -DNTK_GUI
ZYN_UI_LIBS   += $(shell pkg-config --libs ntk_images ntk)
else # HAVE_NTK
FLUID          = fluid
ZYN_UI_FLAGS  += $(shell fltk-config --use-images --cxxflags) -DFLTK_GUI
ZYN_UI_LIBS   += $(shell fltk-config --use-images --ldflags)
endif # HAVE_NTK

# UI extra flags
ifeq ($(HAVE_X11),true)
ZYN_UI_FLAGS += $(shell pkg-config --cflags x11)
ZYN_UI_LIBS  += $(shell pkg-config --libs x11)
endif
ifeq ($(LINUX),true)
ZYN_UI_LIBS  += -lrt
endif

else  # HAVE_ZYN_UI_DEPS

ZYN_DSP_FLAGS += -DNO_UI

endif # SKIP_ZYN_SYNTH
endif # HAVE_ZYN_UI_DEPS
endif # HAVE_ZYN_DEPS

# ---------------------------------------------------------------------------------------------------------------------

NATIVE_PLUGINS_LIBS += $(DGL_LIBS)
NATIVE_PLUGINS_LIBS += $(PROJECTM_LIBS)
NATIVE_PLUGINS_LIBS += $(ZYN_DSP_LIBS)
NATIVE_PLUGINS_LIBS += $(ZITA_DSP_LIBS)

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(HAVE_DGL),true)
ALL_LIBS += $(MODULEDIR)/dgl.a
endif

# ---------------------------------------------------------------------------------------------------------------------

all:

install_external_plugins:
ifeq ($(HAVE_ZYN_DEPS),true)
ifeq ($(HAVE_ZYN_UI_DEPS),true)
	# Create directories (zynaddsubfx)
	install -d $(DESTDIR)$(DATADIR)/carla/resources/zynaddsubfx

	# Install resources (zynaddsubfx)
	install -m 644 \
		bin/resources/zynaddsubfx/*.png \
		$(DESTDIR)$(DATADIR)/carla/resources/zynaddsubfx

	install -m 755 \
		bin/resources/zynaddsubfx-ui \
		$(DESTDIR)$(DATADIR)/carla/resources
endif
endif

features_print_external_plugins:
	@printf -- "\n"
	@printf -- "$(tS)---> External plugins: $(tE)\n"
ifeq ($(HAVE_DGL),true)
	@printf -- "DPF Plugins:  $(ANS_YES) (with UI)\n"
ifeq ($(HAVE_PROJECTM),true)
	@printf -- "DPF ProM:     $(ANS_YES)\n"
else
	@printf -- "DPF ProM:     $(ANS_NO)  $(mS)missing libprojectM$(mE)\n"
endif
else
	@printf -- "DPF Plugins:  $(ANS_YES) (without UI)\n"
ifeq ($(HAVE_PROJECTM),true)
	@printf -- "DPF ProM:     $(ANS_NO)  $(mS)missing OpenGL$(mE)\n"
else
	@printf -- "DPF ProM:     $(ANS_NO)  $(mS)missing OpenGL and libprojectM$(mE)\n"
endif
endif
ifeq ($(HAVE_ZYN_DEPS),true)
ifeq ($(HAVE_ZYN_UI_DEPS),true)
ifeq ($(HAVE_NTK),true)
	@printf -- "ZynAddSubFX:  $(ANS_YES) (with NTK UI)\n"
else
	@printf -- "ZynAddSubFX:  $(ANS_YES) (with FLTK UI)\n"
endif
else
	@printf -- "ZynAddSubFX:  $(ANS_YES) (without UI) $(mS)FLTK or NTK missing$(mE)\n"
endif
else
	@printf -- "ZynAddSubFX:  $(ANS_NO)  $(mS)liblo, fftw3, mxml or zlib missing$(mE)\n"
endif

# ---------------------------------------------------------------------------------------------------------------------
