/*
  ==============================================================================

  The Synthesis ToolKit in C++ (STK) is a set of open source audio
  signal processing and algorithmic synthesis classes written in the
  C++ programming language. STK was designed to facilitate rapid
  development of music synthesis and audio processing software, with
  an emphasis on cross-platform functionality, realtime control,
  ease of use, and educational example code.  STK currently runs
  with realtime support (audio and MIDI) on Linux, Macintosh OS X,
  and Windows computer platforms. Generic, non-realtime support has
  been tested under NeXTStep, Sun, and other platforms and should
  work with any standard C++ compiler.

  STK WWW site: http://ccrma.stanford.edu/software/stk/

  The Synthesis ToolKit in C++ (STK)
  Copyright (c) 1995-2011 Perry R. Cook and Gary P. Scavone

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  Any person wishing to distribute modifications to the Software is
  asked to send the modifications to the original developer so that
  they can be incorporated into the canonical version.  This is,
  however, not a binding provision of this license.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
  ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
  CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  ==============================================================================
*/

#ifndef __STK_STKHEADER__
#define __STK_STKHEADER__

#include "../juce_core.h"

#if JUCE_LITTLE_ENDIAN && ! defined (__LITTLE_ENDIAN__)
 #define __LITTLE_ENDIAN__
#endif

#if JUCE_MAC
 #define __MACOSX_CORE__
#endif

#if JUCE_MSVC
 #pragma warning (push)
 #pragma warning (disable: 4127 4702 4244 4305 4100 4996 4309)
#endif

//=============================================================================
#include "stk/ADSR.h"
#include "stk/Asymp.h"
#include "stk/BandedWG.h"
#include "stk/BeeThree.h"
#include "stk/BiQuad.h"
#include "stk/Blit.h"
#include "stk/BlitSaw.h"
#include "stk/BlitSquare.h"
#include "stk/BlowBotl.h"
#include "stk/BlowHole.h"
#include "stk/Bowed.h"
#include "stk/BowTable.h"
#include "stk/Brass.h"
#include "stk/Chorus.h"
#include "stk/Clarinet.h"
#include "stk/Cubic.h"
#include "stk/Delay.h"
#include "stk/DelayA.h"
#include "stk/DelayL.h"
#include "stk/Drummer.h"
#include "stk/Echo.h"
#include "stk/Effect.h"
#include "stk/Envelope.h"
#include "stk/FileLoop.h"
#include "stk/FileRead.h"
#include "stk/FileWrite.h"
#include "stk/FileWvIn.h"
#include "stk/FileWvOut.h"
#include "stk/Filter.h"
#include "stk/Fir.h"
#include "stk/Flute.h"
#include "stk/FM.h"
#include "stk/FMVoices.h"
#include "stk/FormSwep.h"
#include "stk/Function.h"
#include "stk/Generator.h"
#include "stk/Granulate.h"
#include "stk/HevyMetl.h"
#include "stk/Iir.h"
#include "stk/Instrmnt.h"
#include "stk/JCRev.h"
#include "stk/JetTable.h"
#include "stk/LentPitShift.h"
#include "stk/Mandolin.h"
#include "stk/Mesh2D.h"
#include "stk/MidiFileIn.h"
#include "stk/Modal.h"
#include "stk/ModalBar.h"
#include "stk/Modulate.h"
#include "stk/Moog.h"
#include "stk/Noise.h"
#include "stk/NRev.h"
#include "stk/OnePole.h"
#include "stk/OneZero.h"
#include "stk/PercFlut.h"
#include "stk/Phonemes.h"
#include "stk/PitShift.h"
#include "stk/Plucked.h"
#include "stk/PoleZero.h"
#include "stk/PRCRev.h"
#include "stk/ReedTable.h"
#include "stk/Resonate.h"
#include "stk/Rhodey.h"
#include "stk/Sampler.h"
#include "stk/Saxofony.h"
#include "stk/Shakers.h"
#include "stk/Simple.h"
#include "stk/SineWave.h"
#include "stk/SingWave.h"
#include "stk/Sitar.h"
#include "stk/Skini.h"
#include "stk/Sphere.h"
#include "stk/StifKarp.h"
#include "stk/Stk.h"
#include "stk/TapDelay.h"
#include "stk/TubeBell.h"
#include "stk/Twang.h"
#include "stk/TwoPole.h"
#include "stk/TwoZero.h"
#include "stk/Vector3D.h"
#include "stk/Voicer.h"
#include "stk/VoicForm.h"
#include "stk/Whistle.h"
#include "stk/Wurley.h"
#include "stk/WvIn.h"
#include "stk/WvOut.h"

#if JUCE_MSVC
 #pragma warning (pop)
#endif

#endif   // __STK_STKHEADER__
