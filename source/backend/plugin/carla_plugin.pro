# QtCreator project file

CONFIG    = debug
CONFIG   += link_pkgconfig shared warn_on

DEFINES   = DEBUG
DEFINES  += QTCREATOR_TEST

# Plugins
DEFINES  += WANT_LADSPA
# WANT_DSSI WANT_LV2 WANT_VST

# Samplers
#DEFINES  += WANT_FLUIDSYNTH WANT_LINUXSAMPLER

# ZynAddSubFX
DEFINES  += WANT_ZYNADDSUBFX

# Misc
DEFINES  += WANT_SUIL

PKGCONFIG = liblo suil-0 fluidsynth linuxsampler

TARGET   = carla_plugin
TEMPLATE = lib
VERSION  = 0.5.0

SOURCES  = \
    carla_plugin.cpp \
    carla_plugin_thread.cpp \
    carla_bridge.cpp \
    native.cpp \
    ladspa.cpp \
    dssi.cpp \
    lv2.cpp \
    vst.cpp \
    fluidsynth.cpp \
    linuxsampler.cpp

HEADERS  = \
    carla_plugin_internal.hpp \
    carla_plugin_thread.hpp

HEADERS += \
    ../carla_backend.hpp \
    ../carla_engine.hpp \
    ../carla_native.h \
    ../carla_native.hpp \
    ../carla_plugin.hpp

HEADERS += \
    ../../utils/carla_juce_utils.hpp \
    ../../utils/carla_lib_utils.hpp \
    ../../utils/carla_osc_utils.hpp \
    ../../utils/carla_state_utils.hpp \
    ../../utils/carla_utils.hpp

INCLUDEPATH = . .. \
    ../../includes \
    ../../libs \
    ../../utils

# ---------------------------------------------------------------------------------------

PKGCONFIG += QtCore QtGui

# Fake includes
INCLUDEPATH += \
    /usr/include/qt4/ \
    /opt/kxstudio/include/

# System includes
QMAKE_CXXFLAGS += -isystem /usr/include/qt4/
QMAKE_CXXFLAGS += -isystem /opt/kxstudio/include/

WARN_FLAGS = \
    -ansi -pedantic -pedantic-errors -Wall -Wextra -Wunused-parameter -Wuninitialized \
    -Wlogical-op -Waggregate-return  -Wno-vla \
    -fipa-pure-const -Wsuggest-attribute=const #pure,const,noreturn
#-Wcast-qual -Wconversion -Wsign-conversion -Wformat=2

QMAKE_CFLAGS   += $${WARN_FLAGS} -std=c99 -Wc++-compat -Wunsuffixed-float-constants -Wwrite-strings
QMAKE_CXXFLAGS += $${WARN_FLAGS} -std=c++11 -Wzero-as-null-pointer-constant
