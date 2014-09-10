# QtCreator project file

TARGET   = CarlaStandalone
TEMPLATE = app
VERSION  = 1.9

# -------------------------------------------------------

CONFIG  = debug
CONFIG += link_pkgconfig warn_on

# -------------------------------------------------------

DEFINES  = DEBUG REAL_BUILD
DEFINES += QTCREATOR_TEST

DEFINES += HAVE_FLUIDSYNTH
DEFINES += HAVE_LINUXSAMPLER
DEFINES += HAVE_X11
DEFINES += VESTIGE_HEADER
DEFINES += WANT_ZYNADDSUBFX
DEFINES += WANT_ZYNADDSUBFX_UI

# -------------------------------------------------------

PKGCONFIG  = liblo
PKGCONFIG += alsa
PKGCONFIG += fluidsynth
PKGCONFIG += linuxsampler
PKGCONFIG += x11
PKGCONFIG += fftw3
PKGCONFIG += mxml
PKGCONFIG += zlib
PKGCONFIG += ntk_images
PKGCONFIG += ntk

# -------------------------------------------------------

LIBS  = ../modules/jackbridge.a
LIBS += ../modules/juce_audio_basics.a
LIBS += ../modules/juce_audio_formats.a
LIBS += ../modules/juce_core.a
LIBS += ../modules/lilv.a
LIBS += ../modules/native-plugins.a
LIBS += ../modules/rtmempool.a
LIBS += ../modules/rtaudio.a
LIBS += ../modules/rtmidi.a
LIBS += -ldl -lm -lpthread -lrt -lmagic

# -------------------------------------------------------

SOURCES  = \
    CarlaStandalone.cpp

SOURCES += \
    engine/CarlaEngine.cpp \
    engine/CarlaEngineClient.cpp \
    engine/CarlaEngineData.cpp \
    engine/CarlaEngineGraph.cpp \
    engine/CarlaEngineInternal.cpp \
    engine/CarlaEngineOsc.cpp \
    engine/CarlaEngineOscSend.cpp \
    engine/CarlaEnginePorts.cpp \
    engine/CarlaEngineThread.cpp \
    engine/CarlaEngineJack.cpp \
    engine/CarlaEngineNative.cpp \
    engine/CarlaEngineRtAudio.cpp

SOURCES += \
    plugin/CarlaPlugin.cpp \
    plugin/CarlaPluginInternal.cpp \
    plugin/CarlaPluginThread.cpp \
    plugin/BridgePlugin.cpp \
    plugin/NativePlugin.cpp \
    plugin/LadspaPlugin.cpp \
    plugin/DssiPlugin.cpp \
    plugin/Lv2Plugin.cpp \
    plugin/VstPlugin.cpp \
    plugin/Vst3Plugin.cpp \
    plugin/AuPlugin.cpp \
    plugin/FluidSynthPlugin.cpp \
    plugin/LinuxSamplerPlugin.cpp

SOURCES += \
    ../tests/QtCreator.cpp

HEADERS  = \
    CarlaBackend.h \
    CarlaHost.h \
    CarlaEngine.hpp \
    CarlaPlugin.hpp

HEADERS += \
    engine/*.hpp \
    plugin/*.hpp

HEADERS += \
    ../includes/*.h \
    ../modules/*.h \
    ../modules/*.hpp \
    ../utils/*.cpp \
    ../utils/*.hpp

INCLUDEPATH = . \
    ../includes \
    ../modules \
    ../utils

# -------------------------------------------------------

QMAKE_CXXFLAGS *= -std=c++11

# -------------------------------------------------------
