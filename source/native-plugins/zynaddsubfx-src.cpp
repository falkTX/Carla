/*
 * Carla Native Plugins
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaDefines.h"

#ifdef CARLA_OS_WIN
# include <cmath>
# define errx(...) {}
# define warnx(...) {}
# define rindex strrchr
using std::isnan;
#else
# include <err.h>
#endif

#define PLUGINVERSION
#define SOURCE_DIR "/usr/share/zynaddsubfx/examples"
#undef override

// base c-style headers
#include "zynaddsubfx/tlsf/tlsf.h"
#include "zynaddsubfx/rtosc/rtosc.h"

// C-code includes
extern "C" {
#include "zynaddsubfx/tlsf/tlsf.c"
#undef TLSF_64BIT
#undef tlsf_decl
#undef tlsf_fls_sizet
#undef tlsf_cast
#undef tlsf_min
#undef tlsf_max
#undef tlsf_assert
#undef _tlsf_glue2
#undef _tlsf_glue
#undef tlsf_static_assert
#undef tlsf_insist

#include "zynaddsubfx/rtosc/dispatch.c"
#include "zynaddsubfx/rtosc/rtosc.c"
}

// rtosc includes
#include "zynaddsubfx/rtosc/cpp/midimapper.cpp"
#include "zynaddsubfx/rtosc/cpp/miditable.cpp"
#undef RTOSC_INVALID_MIDI
#undef MAX_UNHANDLED_PATH
#include "zynaddsubfx/rtosc/cpp/ports.cpp"
#undef __builtin_expect
#include "zynaddsubfx/rtosc/cpp/subtree-serialize.cpp"
#include "zynaddsubfx/rtosc/cpp/thread-link.cpp"
#undef off_t
#include "zynaddsubfx/rtosc/cpp/undo-history.cpp"

// zynaddsubfx includes
#include "zynaddsubfx/Containers/MultiPseudoStack.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Containers/NotePool.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/DSP/AnalogFilter.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/DSP/FFTwrapper.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/DSP/Filter.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/DSP/FormantFilter.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/DSP/SVFilter.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/DSP/Unison.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Effects/Alienwah.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Effects/Chorus.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Effects/Distorsion.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Effects/DynamicFilter.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Effects/Echo.cpp"
#undef MAX_DELAY
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Effects/Effect.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Effects/EffectLFO.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Effects/EffectMgr.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Effects/EQ.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Effects/Phaser.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb
#undef PHASER_LFO_SHAPE
#undef ONE_
#undef ZERO_

#include "zynaddsubfx/Effects/Reverb.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Misc/Allocator.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Misc/Bank.cpp"
#undef INSTRUMENT_EXTENSION
#undef FORCE_BANK_DIR_FILE
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Misc/CallbackRepeater.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Misc/Config.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Misc/Master.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Misc/Microtonal.cpp"
#undef MAX_LINE_SIZE
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Misc/MiddleWare.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Misc/Part.cpp"
#undef CLONE
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Misc/PresetExtractor.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Misc/Recorder.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Misc/Util.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Misc/WavFile.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Misc/WaveShapeSmps.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Misc/XMLwrapper.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Params/ADnoteParameters.cpp"
#undef EXPAND
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Params/Controller.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Params/EnvelopeParams.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Params/FilterParams.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Params/LFOParams.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Params/PADnoteParameters.cpp"
#undef PC
#undef P_C
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Params/Presets.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Params/PresetsArray.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Params/PresetsStore.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Params/SUBnoteParameters.cpp"
#undef doPaste
#undef doPPaste
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Synth/ADnote.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Synth/Envelope.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Synth/LFO.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Synth/ModFilter.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Synth/OscilGen.cpp"
#undef PC
#undef DIFF
#undef PRESERVE
#undef RESTORE
#undef FUNC
#undef FILTER
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Synth/PADnote.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Synth/Resonance.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Synth/SUBnote.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/Synth/SynthNote.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/UI/ConnectionDummy.cpp"
#undef rObject
#undef rStdString
#undef rStdStringCb
#undef rChangeCb
#define rChangeCb

#include "zynaddsubfx/globals.cpp"

// Dummy variables and functions for linking purposes
class WavFile;
namespace Nio {
   void masterSwap(Master*){}
   bool setSource(std::string){return true;}
   bool setSink(std::string){return true;}
   std::set<std::string> getSources(void){return std::set<std::string>();}
   std::set<std::string> getSinks(void){return std::set<std::string>();}
   std::string getSource(void){return "";}
   std::string getSink(void){return "";}
   void waveNew(WavFile*){}
   void waveStart(){}
   void waveStop(){}
}
