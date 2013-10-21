/***************************************************/
/*! \class DelayL
    \brief STK linear interpolating delay line class.

    This class implements a fractional-length digital delay-line using
    first-order linear interpolation.  If the delay and maximum length
    are not specified during instantiation, a fixed maximum length of
    4095 and a delay of zero is set.

    Linear interpolation is an efficient technique for achieving
    fractional delay lengths, though it does introduce high-frequency
    signal attenuation to varying degrees depending on the fractional
    delay setting.  The use of higher order Lagrange interpolators can
    typically improve (minimize) this attenuation characteristic.

    by Perry R. Cook and Gary P. Scavone, 1995-2011.
*/
/***************************************************/

#include "DelayL.h"

namespace stk {

DelayL :: DelayL( StkFloat delay, unsigned long maxDelay )
{
  if ( delay < 0.0 ) {
    oStream_ << "DelayL::DelayL: delay must be >= 0.0!";
    handleError( StkError::FUNCTION_ARGUMENT );
  }

  if ( delay > (StkFloat) maxDelay ) {
    oStream_ << "DelayL::DelayL: maxDelay must be > than delay argument!";
    handleError( StkError::FUNCTION_ARGUMENT );
  }

  // Writing before reading allows delays from 0 to length-1. 
  if ( maxDelay + 1 > inputs_.size() )
    inputs_.resize( maxDelay + 1, 1, 0.0 );

  inPoint_ = 0;
  this->setDelay( delay );
  doNextOut_ = true;
}

DelayL :: ~DelayL()
{
}

void DelayL :: setMaximumDelay( unsigned long delay )
{
  if ( delay < inputs_.size() ) return;
  inputs_.resize( delay + 1 );
}

void DelayL :: setDelay( StkFloat delay )
{
  if ( delay + 1 > inputs_.size() ) { // The value is too big.
    oStream_ << "DelayL::setDelay: argument (" << delay << ") greater than  maximum!";
    handleError( StkError::WARNING ); return;
  }

  if (delay < 0 ) {
    oStream_ << "DelayL::setDelay: argument (" << delay << ") less than zero!";
    handleError( StkError::WARNING ); return;
  }

  StkFloat outPointer = inPoint_ - delay;  // read chases write
  delay_ = delay;

  while ( outPointer < 0 )
    outPointer += inputs_.size(); // modulo maximum length

  outPoint_ = (long) outPointer;   // integer part
  if ( outPoint_ == inputs_.size() ) outPoint_ = 0;
  alpha_ = outPointer - outPoint_; // fractional part
  omAlpha_ = (StkFloat) 1.0 - alpha_;
}

StkFloat DelayL :: tapOut( unsigned long tapDelay )
{
  long tap = inPoint_ - tapDelay - 1;
  while ( tap < 0 ) // Check for wraparound.
    tap += inputs_.size();

  return inputs_[tap];
}

void DelayL :: tapIn( StkFloat value, unsigned long tapDelay )
{
  long tap = inPoint_ - tapDelay - 1;
  while ( tap < 0 ) // Check for wraparound.
    tap += inputs_.size();

  inputs_[tap] = value;
}

} // stk namespace
