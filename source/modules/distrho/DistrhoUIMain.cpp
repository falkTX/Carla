/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2025 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "src/DistrhoUI.cpp"

// we might be building a plugin with external UI, which works on most formats except VST2/3
#if ! DISTRHO_PLUGIN_HAS_UI && ! defined(DISTRHO_PLUGIN_VST_HPP_INCLUDED)
# error Trying to build UI without DISTRHO_PLUGIN_HAS_UI set to 1
#endif

#if DISTRHO_PLUGIN_HAS_UI

#if defined(DISTRHO_PLUGIN_TARGET_AU)
# define DISTRHO_PLUGIN_AND_UI_IN_SINGLE_OBJECT 1
# import "src/DistrhoUIAU.mm"
#elif defined(DISTRHO_PLUGIN_TARGET_CARLA)
# define DISTRHO_PLUGIN_AND_UI_IN_SINGLE_OBJECT 1
#elif defined(DISTRHO_PLUGIN_TARGET_CLAP)
# define DISTRHO_PLUGIN_AND_UI_IN_SINGLE_OBJECT 1
#elif defined(DISTRHO_PLUGIN_TARGET_JACK)
# define DISTRHO_PLUGIN_AND_UI_IN_SINGLE_OBJECT 1
#elif defined(DISTRHO_PLUGIN_TARGET_DSSI)
# define DISTRHO_PLUGIN_AND_UI_IN_SINGLE_OBJECT 0
# include "src/DistrhoUIDSSI.cpp"
#elif defined(DISTRHO_PLUGIN_TARGET_LV2)
# if defined(DISTRHO_PLUGIN_AND_UI_IN_SINGLE_OBJECT)
#  if ! DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
#   warning Using single/monolithic LV2 target while DISTRHO_PLUGIN_WANT_DIRECT_ACCESS is 0
#  endif
# else
#  define DISTRHO_PLUGIN_AND_UI_IN_SINGLE_OBJECT DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
# endif
# include "src/DistrhoUILV2.cpp"
#elif defined(DISTRHO_PLUGIN_TARGET_VST2)
# define DISTRHO_PLUGIN_AND_UI_IN_SINGLE_OBJECT 1
#elif defined(DISTRHO_PLUGIN_TARGET_VST3)
# define DISTRHO_PLUGIN_AND_UI_IN_SINGLE_OBJECT 1
# include "src/DistrhoUIVST3.cpp"
#elif defined(DISTRHO_PLUGIN_TARGET_EXPORT)
# define DISTRHO_PLUGIN_AND_UI_IN_SINGLE_OBJECT 1
#elif defined(DISTRHO_PLUGIN_TARGET_SHARED) || defined(DISTRHO_PLUGIN_TARGET_STATIC)
# define DISTRHO_PLUGIN_AND_UI_IN_SINGLE_OBJECT 1
#else
# error unsupported format
#endif

#if !DISTRHO_PLUGIN_AND_UI_IN_SINGLE_OBJECT
# ifdef DISTRHO_PLUGIN_TARGET_DSSI
#  define DISTRHO_IS_STANDALONE 1
# else
#  define DISTRHO_IS_STANDALONE 0
# endif
# include "src/DistrhoUtils.cpp"
#else
# ifdef DISTRHO_PLUGIN_TARGET_JACK
#  define DISTRHO_IS_STANDALONE 1
# else
#  define DISTRHO_IS_STANDALONE 0
# endif
#endif

#if defined(DISTRHO_UI_LINUX_WEBVIEW_START) && !DISTRHO_IS_STANDALONE
int main(int argc, char* argv[])
{
    return DISTRHO_NAMESPACE::dpf_webview_start(argc, argv);
}
#elif defined(DISTRHO_OS_LINUX) && defined(DGL_USE_WEB_VIEW) && !DISTRHO_IS_STANDALONE
int main()
{
    return 0;
}
#endif

#endif
