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

#if defined (__STK_STKHEADER__) && ! JUCE_AMALGAMATED_INCLUDE
 /* When you add this cpp file to your project, you mustn't include it in a file where you've
    already included any other headers - just put it inside a file on its own, possibly with your config
    flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
    header files that the compiler may be using.
 */
 #error "Incorrect use of JUCE cpp file"
#endif

#include "stk.h"

// stops a warning with clang
#ifdef __clang__
 #pragma clang diagnostic ignored "-Wtautological-compare"
#endif

#if JUCE_MSVC
 #pragma warning (push)
 #pragma warning (disable: 4127 4702 4244 4305 4100 4996 4309)
#endif

#include "stk/ADSR.cpp"
#include "stk/Asymp.cpp"
#include "stk/BandedWG.cpp"
#include "stk/BeeThree.cpp"
#include "stk/BiQuad.cpp"
#include "stk/Blit.cpp"
#include "stk/BlitSaw.cpp"
#include "stk/BlitSquare.cpp"
#include "stk/BlowBotl.cpp"
#include "stk/BlowHole.cpp"
#include "stk/Bowed.cpp"
#include "stk/Brass.cpp"
#include "stk/Chorus.cpp"
#include "stk/Clarinet.cpp"
#include "stk/Delay.cpp"
#include "stk/DelayA.cpp"
#include "stk/DelayL.cpp"
#include "stk/Drummer.cpp"
#include "stk/Echo.cpp"
#include "stk/Envelope.cpp"
#include "stk/FileLoop.cpp"
#include "stk/FileRead.cpp"
#include "stk/FileWrite.cpp"
#include "stk/FileWvIn.cpp"
#include "stk/FileWvOut.cpp"
#include "stk/Fir.cpp"
#include "stk/Flute.cpp"
#include "stk/FM.cpp"
#include "stk/FMVoices.cpp"
#include "stk/FormSwep.cpp"
#include "stk/Granulate.cpp"
#include "stk/HevyMetl.cpp"
#include "stk/Iir.cpp"
#include "stk/JCRev.cpp"
#include "stk/LentPitShift.cpp"
#include "stk/Mandolin.cpp"
#include "stk/Mesh2D.cpp"
#include "stk/MidiFileIn.cpp"
#include "stk/Modal.cpp"
#include "stk/ModalBar.cpp"
#include "stk/Modulate.cpp"
#include "stk/Moog.cpp"
#include "stk/Noise.cpp"
#include "stk/NRev.cpp"
#include "stk/OnePole.cpp"
#include "stk/OneZero.cpp"
#include "stk/PercFlut.cpp"
#include "stk/Phonemes.cpp"
#include "stk/PitShift.cpp"
#include "stk/Plucked.cpp"
#include "stk/PoleZero.cpp"
#include "stk/PRCRev.cpp"
#include "stk/Resonate.cpp"
#include "stk/Rhodey.cpp"
#include "stk/Sampler.cpp"
#include "stk/Saxofony.cpp"
#include "stk/Shakers.cpp"
#include "stk/Simple.cpp"
#include "stk/SineWave.cpp"
#include "stk/SingWave.cpp"
#include "stk/Sitar.cpp"
#include "stk/Skini.cpp"
#include "stk/Sphere.cpp"
#include "stk/StifKarp.cpp"
#include "stk/Stk.cpp"
#include "stk/TapDelay.cpp"
#include "stk/TubeBell.cpp"
#include "stk/Twang.cpp"
#include "stk/TwoPole.cpp"
#include "stk/TwoZero.cpp"
#include "stk/Voicer.cpp"
#include "stk/VoicForm.cpp"
#include "stk/Whistle.cpp"
#include "stk/Wurley.cpp"

#if JUCE_MSVC
 #pragma warning (pop)
 #pragma warning (disable: 4127 4702 4244 4305 4100 4996 4309)
#endif

#ifdef __clang__
 #pragma pop // -Wtautological-compare
#endif
