/*
  ==============================================================================

   Build options for juce_graphics static library

  ==============================================================================
*/

#ifndef CARLA_JUCE_GRAPHICS_APPCONFIG_H_INCLUDED
#define CARLA_JUCE_GRAPHICS_APPCONFIG_H_INCLUDED

#include "../juce_core/AppConfig.h"
#include "../juce_events/AppConfig.h"

//=============================================================================
/** Config: JUCE_USE_COREIMAGE_LOADER

    On OSX, enabling this flag means that the CoreImage codecs will be used to load
    PNG/JPEG/GIF files. It is enabled by default, but you may want to disable it if
    you'd rather use libpng, libjpeg, etc.
*/
#define JUCE_USE_COREIMAGE_LOADER 1

/** Config: JUCE_USE_DIRECTWRITE

    Enabling this flag means that DirectWrite will be used when available for font
    management and layout.
*/
#define JUCE_USE_DIRECTWRITE 1

#define JUCE_INCLUDE_PNGLIB_CODE 0

#define JUCE_INCLUDE_JPEGLIB_CODE 0

#define USE_COREGRAPHICS_RENDERING 1

#endif // CARLA_JUCE_GRAPHICS_APPCONFIG_H_INCLUDED
