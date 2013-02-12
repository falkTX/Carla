# QtCreator project file

QT = core gui xml

CONFIG    = debug link_pkgconfig qt warn_on
PKGCONFIG = jack liblo

TARGET   = carla-bridge-qtcreator
TEMPLATE = app
VERSION  = 0.5.0

# -----------------------------------------------------------

SOURCES = \
    ../carla_bridge_client.cpp \
    ../carla_bridge_osc.cpp \
    ../carla_bridge_toolkit.cpp \
    ../carla_bridge_plugin.cpp

HEADERS = \
    ../carla_bridge.hpp \
    ../carla_bridge_client.hpp \
    ../carla_bridge_osc.hpp \
    ../carla_bridge_toolkit.hpp

# -----------------------------------------------------------

# carla-engine
SOURCES += \
    ../../backend/engine/carla_engine.cpp \
    ../../backend/engine/carla_engine_osc.cpp \
    ../../backend/engine/carla_engine_thread.cpp \
    ../../backend/engine/jack.cpp \
    ../../backend/engine/plugin.cpp \
    ../../backend/engine/rtaudio.cpp

# carla-plugin
SOURCES += \
    ../../backend/plugin/carla_plugin.cpp \
    ../../backend/plugin/carla_plugin_thread.cpp \
    ../../backend/plugin/native.cpp \
    ../../backend/plugin/ladspa.cpp \
    ../../backend/plugin/dssi.cpp \
    ../../backend/plugin/lv2.cpp \
    ../../backend/plugin/vst.cpp \
    ../../backend/plugin/fluidsynth.cpp \
    ../../backend/plugin/linuxsampler.cpp

# carla-standalone
SOURCES += \
    ../../backend/standalone/carla_standalone.cpp

# -----------------------------------------------------------

# common
HEADERS += \
    ../../backend/carla_backend.hpp \
    ../../backend/carla_engine.hpp \
    ../../backend/carla_native.h \
    ../../backend/carla_native.hpp \
    ../../backend/carla_plugin.hpp \
    ../../backend/carla_standalone.hpp

# engine
HEADERS += \
    ../../backend/engine/carla_engine_internal.hpp \
    ../../backend/engine/carla_engine_osc.hpp \
    ../../backend/engine/carla_engine_thread.hpp \
    ../../backend/engine/plugin/DistrhoPluginInfo.h

# plugin
HEADERS += \
    ../../backend/plugin/carla_plugin_internal.hpp \
    ../../backend/plugin/carla_plugin_thread.hpp

# includes
HEADERS += \
    ../../includes/carla_defines.hpp \
    ../../includes/carla_midi.h \
    ../../includes/ladspa_rdf.hpp \
    ../../includes/lv2_rdf.hpp

# utils
HEADERS += \
    ../../utils/carla_backend_utils.hpp \
    ../../utils/carla_juce_utils.hpp \
    ../../utils/carla_ladspa_utils.hpp \
    ../../utils/carla_lib_utils.hpp \
    ../../utils/carla_lv2_utils.hpp \
    ../../utils/carla_osc_utils.hpp \
    ../../utils/carla_state_utils.hpp \
    ../../utils/carla_vst_utils.hpp \
    ../../utils/carla_utils.hpp \
    ../../utils/lv2_atom_queue.hpp \
    ../../utils/rt_list.hpp

INCLUDEPATH = .. \
    ../../backend \
    ../../backend/engine \
    ../../backend/plugin \
    ../../backend/utils \
    ../../includes \
    ../../libs \
    ../../utils

# -----------------------------------------------------------

DEFINES  = QTCREATOR_TEST
DEFINES += DEBUG
#DEFINES += VESTIGE_HEADER
DEFINES += BUILD_BRIDGE BUILD_BRIDGE_PLUGIN BRIDGE_PLUGIN

DEFINES += WANT_JACK
DEFINES += WANT_LADSPA
# WANT_DSSI WANT_LV2 WANT_VST

LIBS     = -ldl \
    ../../libs/lilv.a \
    ../../libs/rtmempool.a

QMAKE_CXXFLAGS *= -std=c++0x
