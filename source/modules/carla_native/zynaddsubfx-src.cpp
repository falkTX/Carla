/*
 * Carla Native Plugins
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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
#include "zynaddsubfx/Effects/Effect.cpp"
#include "zynaddsubfx/Effects/EffectLFO.cpp"
#include "zynaddsubfx/Effects/EffectMgr.cpp"
#include "zynaddsubfx/Effects/EQ.cpp"
#include "zynaddsubfx/Effects/Phaser.cpp"
#include "zynaddsubfx/Effects/Reverb.cpp"
#include "zynaddsubfx/Misc/Bank.cpp"
#include "zynaddsubfx/Misc/Config.cpp"
#include "zynaddsubfx/Misc/Dump.cpp"
#include "zynaddsubfx/Misc/Master.cpp"
#include "zynaddsubfx/Misc/Microtonal.cpp"
#include "zynaddsubfx/Misc/Part.cpp"
#include "zynaddsubfx/Misc/Recorder.cpp"
//#include "zynaddsubfx/Misc/Stereo.cpp"
#include "zynaddsubfx/Misc/Util.cpp"
#include "zynaddsubfx/Misc/WavFile.cpp"
#include "zynaddsubfx/Misc/WaveShapeSmps.cpp"
#include "zynaddsubfx/Misc/XMLwrapper.cpp"
#include "zynaddsubfx/Params/ADnoteParameters.cpp"
#include "zynaddsubfx/Params/Controller.cpp"
#include "zynaddsubfx/Params/EnvelopeParams.cpp"
#include "zynaddsubfx/Params/FilterParams.cpp"
#include "zynaddsubfx/Params/LFOParams.cpp"
#include "zynaddsubfx/Params/PADnoteParameters.cpp"
#include "zynaddsubfx/Params/Presets.cpp"
#include "zynaddsubfx/Params/PresetsArray.cpp"
#include "zynaddsubfx/Params/PresetsStore.cpp"
#include "zynaddsubfx/Params/SUBnoteParameters.cpp"
#include "zynaddsubfx/Synth/ADnote.cpp"
#include "zynaddsubfx/Synth/Envelope.cpp"
#include "zynaddsubfx/Synth/LFO.cpp"
#include "zynaddsubfx/Synth/OscilGen.cpp"
#include "zynaddsubfx/Synth/PADnote.cpp"
#include "zynaddsubfx/Synth/Resonance.cpp"
#include "zynaddsubfx/Synth/SUBnote.cpp"
#include "zynaddsubfx/Synth/SynthNote.cpp"

// Dummy variables and functions for linking purposes
const char* instance_name = nullptr;
SYNTH_T* synth = nullptr;
class WavFile;
namespace Nio {
   bool start(void){return 1;}
   void stop(void){}
   bool setSource(std::string){return true;}
   bool setSink(std::string){return true;}
   std::set<std::string> getSources(void){return std::set<std::string>();}
   std::set<std::string> getSinks(void){return std::set<std::string>();}
   std::string getSource(void){return "";}
   std::string getSink(void){return "";}
   void waveNew(WavFile*){}
   void waveStart(void){}
   void waveStop(void){}
   void waveEnd(void){}
}
