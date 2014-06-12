# QtCreator project file

QT = core gui widgets

TARGET   = carla-frontend
TEMPLATE = app
VERSION  = 1.9.4

# -------------------------------------------------------

CONFIG   = debug
CONFIG  += qt resources warn_on

DEFINES  = DEBUG
DEFINES += REAL_BUILD

# -------------------------------------------------------

SOURCES = \
    carla_host.cpp \
    carla_shared.cpp \
    carla_style.cpp

HEADERS += \
    carla_shared.hpp \
    carla_style.hpp

RESOURCES = ../../resources/resources.qrc

# -------------------------------------------------------

HEADERS += \
    ../includes/CarlaDefines.h

HEADERS += \
    ../backend/CarlaBackend.h \
    ../backend/CarlaHost.h

# -------------------------------------------------------

INCLUDEPATH = \
    ../backend \
    ../includes \
    ../modules \
    ../utils

# -------------------------------------------------------

LIBS  = ../modules/theme.qt5.a
LIBS += -L../../bin -lcarla_standalone2

# -------------------------------------------------------

QMAKE_CXXFLAGS += -Wall -Wextra -pipe
QMAKE_CXXFLAGS += -Werror -Wcast-align -Wcast-qual -Wconversion -Wformat-security -Wredundant-decls -Wshadow -Wstrict-overflow -fstrict-overflow -Wundef -Wunsafe-loop-optimizations -Wwrite-strings
QMAKE_CXXFLAGS += -Wnon-virtual-dtor -Woverloaded-virtual
QMAKE_CXXFLAGS += -Wlogical-op -Wsign-conversion
# -Wmissing-declarations
QMAKE_CXXFLAGS += -O0 -g
QMAKE_CXXFLAGS += -std=c++0x -std=gnu++0x
QMAKE_CXXFLAGS += -isystem /opt/kxstudio/include -isystem /usr/include/qt5

# -------------------------------------------------------
