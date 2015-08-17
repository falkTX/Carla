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
#undef static
#include "zynaddsubfx/rtosc/cpp/undo-history.cpp"

// zynaddsubfx includes
#include "zynaddsubfx/DSP/AnalogFilter.cpp"
#include "zynaddsubfx/DSP/FFTwrapper.cpp"
#include "zynaddsubfx/DSP/Filter.cpp"
#include "zynaddsubfx/DSP/FormantFilter.cpp"
#include "zynaddsubfx/DSP/SVFilter.cpp"
#include "zynaddsubfx/DSP/Unison.cpp"
#include "zynaddsubfx/Effects/Alienwah.cpp"
#include "zynaddsubfx/Effects/Chorus.cpp"
#include "zynaddsubfx/Effects/Distorsion.cpp"
#include "zynaddsubfx/Effects/DynamicFilter.cpp"
#include "zynaddsubfx/Effects/Echo.cpp"
#undef MAX_DELAY
#include "zynaddsubfx/Effects/Effect.cpp"
#include "zynaddsubfx/Effects/EffectLFO.cpp"
#include "zynaddsubfx/Effects/EffectMgr.cpp"
#undef rObject
#include "zynaddsubfx/Effects/EQ.cpp"
#include "zynaddsubfx/Effects/Phaser.cpp"
#undef PHASER_LFO_SHAPE
#undef ONE_
#undef ZERO_
#include "zynaddsubfx/Effects/Reverb.cpp"
#include "zynaddsubfx/Misc/Allocator.cpp"
#include "zynaddsubfx/Misc/Bank.cpp"
#undef INSTRUMENT_EXTENSION
#undef FORCE_BANK_DIR_FILE
#include "zynaddsubfx/Misc/Config.cpp"
#undef rStdString
#undef rStdStringCb
#undef rObject
#include "zynaddsubfx/Misc/Master.cpp"
#undef rObject
#include "zynaddsubfx/Misc/Microtonal.cpp"
#undef MAX_LINE_SIZE
#undef rObject
#include "zynaddsubfx/Misc/MiddleWare.cpp"
#include "zynaddsubfx/Misc/Part.cpp"
#undef rChangeCb
#define rChangeCb
#undef rObject
#undef CLONE
#include "zynaddsubfx/Misc/PresetExtractor.cpp"
#include "zynaddsubfx/Misc/Recorder.cpp"
//#include "zynaddsubfx/Misc/Stereo.cpp"
#include "zynaddsubfx/Misc/TmpFileMgr.cpp"
#include "zynaddsubfx/Misc/Util.cpp"
#include "zynaddsubfx/Misc/WavFile.cpp"
#include "zynaddsubfx/Misc/WaveShapeSmps.cpp"
#include "zynaddsubfx/Misc/XMLwrapper.cpp"
#include "zynaddsubfx/Params/ADnoteParameters.cpp"
#undef EXPAND
#undef rObject
#include "zynaddsubfx/Params/Controller.cpp"
#undef rObject
#include "zynaddsubfx/Params/EnvelopeParams.cpp"
#undef rObject
#include "zynaddsubfx/Params/FilterParams.cpp"
#undef rObject
#include "zynaddsubfx/Params/LFOParams.cpp"
#undef rObject
#include "zynaddsubfx/Params/PADnoteParameters.cpp"
#undef rObject
#undef PC
#undef P_C
#include "zynaddsubfx/Params/Presets.cpp"
#include "zynaddsubfx/Params/PresetsArray.cpp"
#include "zynaddsubfx/Params/PresetsStore.cpp"
#include "zynaddsubfx/Params/SUBnoteParameters.cpp"
#undef rObject
#undef doPaste
#undef doPPaste
#include "zynaddsubfx/Synth/ADnote.cpp"
#include "zynaddsubfx/Synth/Envelope.cpp"
#include "zynaddsubfx/Synth/LFO.cpp"
#include "zynaddsubfx/Synth/OscilGen.cpp"
#undef rObject
#undef PC
#undef DIFF
#undef PRESERVE
#undef RESTORE
#undef FUNC
#undef FILTER
#include "zynaddsubfx/Synth/PADnote.cpp"
#include "zynaddsubfx/Synth/Resonance.cpp"
#undef rObject
#include "zynaddsubfx/Synth/SUBnote.cpp"
#include "zynaddsubfx/Synth/SynthNote.cpp"
#include "zynaddsubfx/UI/ConnectionDummy.cpp"
#include "zynaddsubfx/globals.cpp"

// Dummy variables and functions for linking purposes
// const char* instance_name = nullptr;
// SYNTH_T* synth = nullptr;
class WavFile;
namespace Nio {
   void masterSwap(Master*){}
//    bool start(void){return 1;}
//    void stop(void){}
   bool setSource(std::string){return true;}
   bool setSink(std::string){return true;}
   std::set<std::string> getSources(void){return std::set<std::string>();}
   std::set<std::string> getSinks(void){return std::set<std::string>();}
   std::string getSource(void){return "";}
   std::string getSink(void){return "";}
   void waveNew(WavFile*){}
   void waveStart(){}
   void waveStop(){}
//    void waveEnd(void){}
}
