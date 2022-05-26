/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2022 Filipe Coelho <falktx@falktx.com>
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

#if defined(DISTRHO_PLUGIN_TARGET_CARLA)
// nothing
#elif defined(DISTRHO_PLUGIN_TARGET_JACK)
// nothing
#elif defined(DISTRHO_PLUGIN_TARGET_DSSI)
# include "src/DistrhoUIDSSI.cpp"
#elif defined(DISTRHO_PLUGIN_TARGET_LV2)
# include "src/DistrhoUILV2.cpp"
#elif defined(DISTRHO_PLUGIN_TARGET_VST2)
// nothing
#elif defined(DISTRHO_PLUGIN_TARGET_VST3)
# include "src/DistrhoUIVST3.cpp"
#elif defined(DISTRHO_PLUGIN_TARGET_SHARED) || defined(DISTRHO_PLUGIN_TARGET_STATIC)
// nothing
#else
# error unsupported format
#endif

#if !DISTRHO_PLUGIN_WANT_DIRECT_ACCESS && !defined(DISTRHO_PLUGIN_TARGET_CARLA) && !defined(DISTRHO_PLUGIN_TARGET_JACK) && !defined(DISTRHO_PLUGIN_TARGET_VST2) && !defined(DISTRHO_PLUGIN_TARGET_VST3)
# ifdef DISTRHO_PLUGIN_TARGET_DSSI
#  define DISTRHO_IS_STANDALONE 1
# else
#  define DISTRHO_IS_STANDALONE 0
# endif
# include "src/DistrhoUtils.cpp"
#endif
