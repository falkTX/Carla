# QtCreator project file

TARGET   = CarlaPlugin
TEMPLATE = lib
VERSION  = 1.0

# -------------------------------------------------------

QT = core gui xml

CONFIG     = debug
CONFIG    += link_pkgconfig qt shared warn_on

DEFINES    = DEBUG
DEFINES   += HAVE_CPP11_SUPPORT
DEFINES   += QTCREATOR_TEST MOC_PARSING

# Shared
DEFINES   += WANT_NATIVE
DEFINES   += WANT_LADSPA
DEFINES   += WANT_DSSI
DEFINES   += WANT_LV2
DEFINES   += WANT_VST
DEFINES   += WANT_PLUGIN
DEFINES   += WANT_FLUIDSYNTH
#DEFINES   += WANT_LINUXSAMPLER
DEFINES   += WANT_OPENGL
DEFINES   += WANT_AUDIOFILE
DEFINES   += WANT_MIDIFILE
DEFINES   += WANT_ZYNADDSUBFX
DEFINES   += WANT_ZYNADDSUBFX_UI

# Plugin
PKGCONFIG += liblo

# FluidSynth
PKGCONFIG += fluidsynth

# LinuxSampler
#PKGCONFIG += linuxsampler

# -------------------------------------------------------

SOURCES  = \
    CarlaPlugin.cpp \
    CarlaPluginGui.cpp \
    CarlaPluginThread.cpp \
    BridgePlugin.cpp \
    NativePlugin.cpp \
    LadspaPlugin.cpp \
    DssiPlugin.cpp \
    Lv2Plugin.cpp \
    VstPlugin.cpp \
    Vst3Plugin.cpp \
    JucePlugin.cpp \
    FluidSynthPlugin.cpp \
    LinuxSamplerPlugin.cpp

HEADERS  = \
    CarlaPluginInternal.hpp \
    CarlaPluginGui.hpp \
    CarlaPluginThread.hpp

HEADERS += \
    ../CarlaBackend.hpp \
    ../CarlaEngine.hpp \
    ../CarlaNative.h \
    ../CarlaPlugin.hpp

HEADERS += \
    ../../includes/CarlaDefines.hpp \
    ../../includes/CarlaMIDI.h \
    ../../includes/ladspa_rdf.hpp \
    ../../includes/lv2_rdf.hpp

HEADERS += \
    ../../utils/CarlaMutex.hpp \
    ../../utils/CarlaRingBuffer.hpp \
    ../../utils/CarlaString.hpp \
    ../../utils/CarlaUtils.hpp \
    ../../utils/CarlaBackendUtils.hpp \
    ../../utils/CarlaBridgeUtils.hpp \
    ../../utils/CarlaJuceUtils.hpp \
    ../../utils/CarlaLibUtils.hpp \
    ../../utils/CarlaOscUtils.hpp \
    ../../utils/CarlaStateUtils.hpp \
    ../../utils/Lv2AtomQueue.hpp \
    ../../utils/RtList.hpp

INCLUDEPATH = . .. \
    ../../includes \
    ../../modules \
    ../../utils

QMAKE_CXXFLAGS += -std=c++0x
