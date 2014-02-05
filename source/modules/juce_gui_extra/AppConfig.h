/*
  ==============================================================================

   Build options for juce_gui_extra static library

  ==============================================================================
*/

#ifndef CARLA_JUCE_GUI_EXTRA_APPCONFIG_H_INCLUDED
#define CARLA_JUCE_GUI_EXTRA_APPCONFIG_H_INCLUDED

#include "../juce_gui_basics/AppConfig.h"

//=============================================================================
/** Config: JUCE_WEB_BROWSER
    This lets you disable the WebBrowserComponent class (Mac and Windows).
    If you're not using any embedded web-pages, turning this off may reduce your code size.
*/
#define JUCE_WEB_BROWSER 0

/** Config: JUCE_ENABLE_LIVE_CONSTANT_EDITOR
    This lets you turn on the JUCE_ENABLE_LIVE_CONSTANT_EDITOR support. See the documentation
    for that macro for more details.
*/
#define JUCE_ENABLE_LIVE_CONSTANT_EDITOR 0

#endif // CARLA_JUCE_GUI_EXTRA_APPCONFIG_H_INCLUDED
