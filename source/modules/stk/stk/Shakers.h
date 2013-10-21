#ifndef STK_SHAKERS_H
#define STK_SHAKERS_H

#include "Instrmnt.h"

namespace stk {

/***************************************************/
/*! \class Shakers
    \brief PhISEM and PhOLIES class.

    PhISEM (Physically Informed Stochastic Event
    Modeling) is an algorithmic approach for
    simulating collisions of multiple independent
    sound producing objects.  This class is a
    meta-model that can simulate a Maraca, Sekere,
    Cabasa, Bamboo Wind Chimes, Water Drops,
    Tambourine, Sleighbells, and a Guiro.

    PhOLIES (Physically-Oriented Library of
    Imitated Environmental Sounds) is a similar
    approach for the synthesis of environmental
    sounds.  This class implements simulations of
    breaking sticks, crunchy snow (or not), a
    wrench, sandpaper, and more.

    Control Change Numbers: 
      - Shake Energy = 2
      - System Decay = 4
      - Number Of Objects = 11
      - Resonance Frequency = 1
      - Shake Energy = 128
      - Instrument Selection = 1071
        - Maraca = 0
        - Cabasa = 1
        - Sekere = 2
        - Guiro = 3
        - Water Drops = 4
        - Bamboo Chimes = 5
        - Tambourine = 6
        - Sleigh Bells = 7
        - Sticks = 8
        - Crunch = 9
        - Wrench = 10
        - Sand Paper = 11
        - Coke Can = 12
        - Next Mug = 13
        - Penny + Mug = 14
        - Nickle + Mug = 15
        - Dime + Mug = 16
        - Quarter + Mug = 17
        - Franc + Mug = 18
        - Peso + Mug = 19
        - Big Rocks = 20
        - Little Rocks = 21
        - Tuned Bamboo Chimes = 22

    by Perry R. Cook, 1995-2011.
*/
/***************************************************/

const int MAX_FREQS = 8;
const int NUM_INSTR = 24;

class Shakers : public Instrmnt
{
 public:
  //! Class constructor.
  Shakers( void );

  //! Class destructor.
  ~Shakers( void );

  //! Start a note with the given instrument and amplitude.
  /*!
    Use the instrument numbers above, converted to frequency values
    as if MIDI note numbers, to select a particular instrument.
  */
  void noteOn( StkFloat instrument, StkFloat amplitude );

  //! Stop a note with the given amplitude (speed of decay).
  void noteOff( StkFloat amplitude );

  //! Perform the control change specified by \e number and \e value (0.0 - 128.0).
  void controlChange( int number, StkFloat value );

  //! Compute and return one output sample.
  StkFloat tick( unsigned int channel = 0 );

  //! Fill a channel of the StkFrames object with computed outputs.
  /*!
    The \c channel argument must be less than the number of
    channels in the StkFrames argument (the first channel is specified
    by 0).  However, range checking is only performed if _STK_DEBUG_
    is defined during compilation, in which case an out-of-range value
    will trigger an StkError exception.
  */
  StkFrames& tick( StkFrames& frames, unsigned int channel = 0 );

 protected:

  int setupName( char* instr );
  int setupNum( int inst );
  int setFreqAndReson( int which, StkFloat freq, StkFloat reson );
  void setDecays( StkFloat sndDecay, StkFloat sysDecay );
  void setFinalZs( StkFloat z0, StkFloat z1, StkFloat z2 );
  StkFloat wuter_tick( void );
  StkFloat tbamb_tick( void );
  StkFloat ratchet_tick( void );

  int instType_;
  int ratchetPos_, lastRatchetPos_;
  StkFloat shakeEnergy_;
  StkFloat inputs_[MAX_FREQS];
  StkFloat outputs_[MAX_FREQS][2];
  StkFloat coeffs_[MAX_FREQS][2];
  StkFloat sndLevel_;
  StkFloat baseGain_;
  StkFloat gains_[MAX_FREQS];
  int nFreqs_;
  StkFloat t_center_freqs_[MAX_FREQS];
  StkFloat center_freqs_[MAX_FREQS];
  StkFloat resons_[MAX_FREQS];
  StkFloat freq_rand_[MAX_FREQS];
  int freqalloc_[MAX_FREQS];
  StkFloat soundDecay_;
  StkFloat systemDecay_;
  StkFloat nObjects_;
  StkFloat totalEnergy_;
  StkFloat ratchet_, ratchetDelta_;
  StkFloat finalZ_[3];
  StkFloat finalZCoeffs_[3];
  StkFloat defObjs_[NUM_INSTR];
  StkFloat defDecays_[NUM_INSTR];
  StkFloat decayScale_[NUM_INSTR];

};

inline StkFrames& Shakers :: tick( StkFrames& frames, unsigned int channel )
{
  unsigned int nChannels = lastFrame_.channels();
#if defined(_STK_DEBUG_)
  if ( channel > frames.channels() - nChannels ) {
    oStream_ << "Shakers::tick(): channel and StkFrames arguments are incompatible!";
    handleError( StkError::FUNCTION_ARGUMENT );
  }
#endif

  StkFloat *samples = &frames[channel];
  unsigned int j, hop = frames.channels() - nChannels;
  if ( nChannels == 1 ) {
    for ( unsigned int i=0; i<frames.frames(); i++, samples += hop )
      *samples++ = tick();
  }
  else {
    for ( unsigned int i=0; i<frames.frames(); i++, samples += hop ) {
      *samples++ = tick();
      for ( j=1; j<nChannels; j++ )
        *samples++ = lastFrame_[j];
    }
  }

  return frames;
}

} // stk namespace

#endif
