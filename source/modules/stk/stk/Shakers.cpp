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

#include "Shakers.h"
#include "SKINI.msg"
#include <cstring>
#include <cmath>
#include <cstdlib>

namespace stk {

int my_random( int max ) //  Return Random Int Between 0 and max
{
  int temp = (int) ((float)max * rand() / (RAND_MAX + 1.0) );
  return temp;
}

StkFloat float_random( StkFloat max ) // Return random float between 0.0 and max
{	
  StkFloat temp = (StkFloat) (max * rand() / (RAND_MAX + 1.0) );
  return temp;
}

StkFloat noise_tick( void ) //  Return random StkFloat float between -1.0 and 1.0
{
  StkFloat temp = (StkFloat) (2.0 * rand() / (RAND_MAX + 1.0) );
  temp -= 1.0;
  return temp;
}

// Maraca
const StkFloat MARA_SOUND_DECAY = 0.95;
const StkFloat MARA_SYSTEM_DECAY = 0.999;
const StkFloat MARA_GAIN = 20.0;
const StkFloat MARA_NUM_BEANS = 25;
const StkFloat MARA_CENTER_FREQ = 3200.0;
const StkFloat MARA_RESON = 0.96;

// Sekere
const StkFloat SEKE_SOUND_DECAY = 0.96;
const StkFloat SEKE_SYSTEM_DECAY = 0.999;
const StkFloat SEKE_GAIN = 20.0;
const StkFloat SEKE_NUM_BEANS = 64;
const StkFloat SEKE_CENTER_FREQ = 5500.0;
const StkFloat SEKE_RESON = 0.6;

// Sandpaper
const StkFloat SANDPAPR_SOUND_DECAY = 0.999;
const StkFloat SANDPAPR_SYSTEM_DECAY = 0.999;
const StkFloat SANDPAPR_GAIN = 0.5;
const StkFloat SANDPAPR_NUM_GRAINS = 128;
const StkFloat SANDPAPR_CENTER_FREQ = 4500.0;
const StkFloat SANDPAPR_RESON = 0.6;

// Cabasa
const StkFloat CABA_SOUND_DECAY = 0.96;
const StkFloat CABA_SYSTEM_DECAY = 0.997;
const StkFloat CABA_GAIN = 40.0;
const StkFloat CABA_NUM_BEADS = 512;
const StkFloat CABA_CENTER_FREQ = 3000.0;
const StkFloat CABA_RESON = 0.7;

// Bamboo Wind Chimes
const StkFloat BAMB_SOUND_DECAY = 0.95;
const StkFloat BAMB_SYSTEM_DECAY = 0.9999;
const StkFloat BAMB_GAIN = 2.0;
const StkFloat BAMB_NUM_TUBES = 1.25;
const StkFloat BAMB_CENTER_FREQ0 = 2800.0;
const StkFloat BAMB_CENTER_FREQ1 = 0.8 * 2800.0;
const StkFloat BAMB_CENTER_FREQ2 = 1.2 * 2800.0;
const StkFloat BAMB_RESON	= 0.995;

// Tuned Bamboo Wind Chimes (Anklung)
const StkFloat TBAMB_SOUND_DECAY = 0.95;
const StkFloat TBAMB_SYSTEM_DECAY = 0.9999;
const StkFloat TBAMB_GAIN = 1.0;
const StkFloat TBAMB_NUM_TUBES = 1.25;
const StkFloat TBAMB_CENTER_FREQ0 = 1046.6;
const StkFloat TBAMB_CENTER_FREQ1 = 1174.8;
const StkFloat TBAMB_CENTER_FREQ2 = 1397.0;
const StkFloat TBAMB_CENTER_FREQ3 = 1568.0;
const StkFloat TBAMB_CENTER_FREQ4 = 1760.0;
const StkFloat TBAMB_CENTER_FREQ5 = 2093.3;
const StkFloat TBAMB_CENTER_FREQ6 = 2350.0;
const StkFloat TBAMB_RESON = 0.996;

// Water Drops
const StkFloat WUTR_SOUND_DECAY = 0.95;
const StkFloat WUTR_SYSTEM_DECAY = 0.996;
const StkFloat WUTR_GAIN = 1.0;
const StkFloat WUTR_NUM_SOURCES = 10;
const StkFloat WUTR_CENTER_FREQ0 = 450.0;
const StkFloat WUTR_CENTER_FREQ1 = 600.0;
const StkFloat WUTR_CENTER_FREQ2 = 750.0;
const StkFloat WUTR_RESON = 0.9985;
const StkFloat WUTR_FREQ_SWEEP = 1.0001;

// Tambourine
const StkFloat TAMB_SOUND_DECAY = 0.95;
const StkFloat TAMB_SYSTEM_DECAY = 0.9985;
const StkFloat TAMB_GAIN = 5.0;
const StkFloat TAMB_NUM_TIMBRELS = 32;
const StkFloat TAMB_SHELL_FREQ = 2300;
const StkFloat TAMB_SHELL_GAIN = 0.1;
const StkFloat TAMB_SHELL_RESON = 0.96;
const StkFloat TAMB_CYMB_FREQ1 = 5600;
const StkFloat TAMB_CYMB_FREQ2 = 8100;
const StkFloat TAMB_CYMB_RESON = 0.99;

// Sleighbells
const StkFloat SLEI_SOUND_DECAY = 0.97;
const StkFloat SLEI_SYSTEM_DECAY = 0.9994;
const StkFloat SLEI_GAIN = 1.0;
const StkFloat SLEI_NUM_BELLS = 32;
const StkFloat SLEI_CYMB_FREQ0 = 2500;
const StkFloat SLEI_CYMB_FREQ1 = 5300;
const StkFloat SLEI_CYMB_FREQ2 = 6500;
const StkFloat SLEI_CYMB_FREQ3 = 8300;
const StkFloat SLEI_CYMB_FREQ4 = 9800;
const StkFloat SLEI_CYMB_RESON = 0.99;

// Guiro
const StkFloat GUIR_SOUND_DECAY = 0.95;
const StkFloat GUIR_GAIN = 10.0;
const StkFloat GUIR_NUM_PARTS = 128;
const StkFloat GUIR_GOURD_FREQ = 2500.0;
const StkFloat GUIR_GOURD_RESON = 0.97;
const StkFloat GUIR_GOURD_FREQ2 = 4000.0;
const StkFloat GUIR_GOURD_RESON2 = 0.97;

// Wrench
const StkFloat WRENCH_SOUND_DECAY = 0.95;
const StkFloat WRENCH_GAIN = 5;
const StkFloat WRENCH_NUM_PARTS = 128;
const StkFloat WRENCH_FREQ = 3200.0;
const StkFloat WRENCH_RESON = 0.99;
const StkFloat WRENCH_FREQ2 = 8000.0;
const StkFloat WRENCH_RESON2 = 0.992;

// Cokecan
const StkFloat COKECAN_SOUND_DECAY = 0.97;
const StkFloat COKECAN_SYSTEM_DECAY = 0.999;
const StkFloat COKECAN_GAIN = 0.8;
const StkFloat COKECAN_NUM_PARTS = 48;
const StkFloat COKECAN_HELMFREQ = 370;
const StkFloat COKECAN_HELM_RES = 0.99;
const StkFloat COKECAN_METLFREQ0 = 1025;
const StkFloat COKECAN_METLFREQ1 = 1424;
const StkFloat COKECAN_METLFREQ2 = 2149;
const StkFloat COKECAN_METLFREQ3 = 3596;
const StkFloat COKECAN_METL_RES = 0.992;

// PhOLIES (Physically-Oriented Library of Imitated Environmental
// Sounds), Perry Cook, 1997-8

// Stix1
const StkFloat STIX1_SOUND_DECAY = 0.96;
const StkFloat STIX1_SYSTEM_DECAY = 0.998;
const StkFloat STIX1_GAIN = 30.0;
const StkFloat STIX1_NUM_BEANS = 2;
const StkFloat STIX1_CENTER_FREQ = 5500.0;
const StkFloat STIX1_RESON = 0.6;

// Crunch1
const StkFloat CRUNCH1_SOUND_DECAY = 0.95;
const StkFloat CRUNCH1_SYSTEM_DECAY = 0.99806;
const StkFloat CRUNCH1_GAIN = 20.0;
const StkFloat CRUNCH1_NUM_BEADS = 7;
const StkFloat CRUNCH1_CENTER_FREQ = 800.0;
const StkFloat CRUNCH1_RESON = 0.95;

// Nextmug
const StkFloat NEXTMUG_SOUND_DECAY = 0.97;
const StkFloat NEXTMUG_SYSTEM_DECAY = 0.9995;
const StkFloat NEXTMUG_GAIN = 0.8;
const StkFloat NEXTMUG_NUM_PARTS = 3;
const StkFloat NEXTMUG_FREQ0 = 2123;
const StkFloat NEXTMUG_FREQ1 = 4518;
const StkFloat NEXTMUG_FREQ2 = 8856;
const StkFloat NEXTMUG_FREQ3 = 10753;
const StkFloat NEXTMUG_RES = 0.997;

const StkFloat PENNY_FREQ0 = 11000;
const StkFloat PENNY_FREQ1 = 5200;
const StkFloat PENNY_FREQ2 = 3835;
const StkFloat PENNY_RES   = 0.999;

const StkFloat NICKEL_FREQ0 = 5583;
const StkFloat NICKEL_FREQ1 = 9255;
const StkFloat NICKEL_FREQ2 = 9805;
const StkFloat NICKEL_RES = 0.9992;

const StkFloat DIME_FREQ0 = 4450;
const StkFloat DIME_FREQ1 = 4974;
const StkFloat DIME_FREQ2 = 9945;
const StkFloat DIME_RES = 0.9993;

const StkFloat QUARTER_FREQ0 = 1708;
const StkFloat QUARTER_FREQ1 = 8863;
const StkFloat QUARTER_FREQ2 = 9045;
const StkFloat QUARTER_RES = 0.9995;

const StkFloat FRANC_FREQ0 = 5583;
const StkFloat FRANC_FREQ1 = 11010;
const StkFloat FRANC_FREQ2 = 1917;
const StkFloat FRANC_RES = 0.9995;

const StkFloat PESO_FREQ0 = 7250;
const StkFloat PESO_FREQ1 = 8150;
const StkFloat PESO_FREQ2 = 10060;
const StkFloat PESO_RES = 0.9996;

// Big Gravel
const StkFloat BIGROCKS_SOUND_DECAY = 0.98;
const StkFloat BIGROCKS_SYSTEM_DECAY = 0.9965;
const StkFloat BIGROCKS_GAIN = 20.0;
const StkFloat BIGROCKS_NUM_PARTS = 23;
const StkFloat BIGROCKS_FREQ = 6460;
const StkFloat BIGROCKS_RES = 0.932;

// Little Gravel
const StkFloat LITLROCKS_SOUND_DECAY = 0.98;
const StkFloat LITLROCKS_SYSTEM_DECAY = 0.99586;
const StkFloat LITLROCKS_GAIN = 20.0;
const StkFloat LITLROCKS_NUM_PARTS = 1600;
const StkFloat LITLROCKS_FREQ = 9000;
const StkFloat LITLROCKS_RES = 0.843;

// Finally ... the class code!

Shakers :: Shakers( void )
{
  int i;

  instType_ = 0;
  shakeEnergy_ = 0.0;
  nFreqs_ = 0;
  sndLevel_ = 0.0;

  for ( i=0; i<MAX_FREQS; i++ )	{
    inputs_[i] = 0.0;
    outputs_[i][0] = 0.0;
    outputs_[i][1] = 0.0;
    coeffs_[i][0] = 0.0;
    coeffs_[i][1] = 0.0;
    gains_[i] = 0.0;
    center_freqs_[i] = 0.0;
    resons_[i] =  0.0;
    freq_rand_[i] = 0.0;
    freqalloc_[i] = 0;
  }

  soundDecay_ = 0.0;
  systemDecay_ = 0.0;
  nObjects_ = 0.0;
  totalEnergy_ = 0.0;
  ratchet_ = 0.0;
  ratchetDelta_ = 0.0005;
  lastRatchetPos_ = 0;
  finalZ_[0] = 0.0;
  finalZ_[1] = 0.0;
  finalZ_[2] = 0.0;
  finalZCoeffs_[0] = 1.0;
  finalZCoeffs_[1] = 0.0;
  finalZCoeffs_[2] = 0.0;

  this->setupNum(instType_);
}

Shakers :: ~Shakers( void )
{
}

const StkFloat MAX_SHAKE = 2000.0;

char instrs[NUM_INSTR][10] = {
  "Maraca", "Cabasa", "Sekere", "Guiro",
  "Waterdrp", "Bamboo", "Tambourn", "Sleighbl", 
  "Stix1", "Crunch1", "Wrench", "SandPapr",
  "CokeCan", "NextMug", "PennyMug", "NicklMug",
  "DimeMug", "QuartMug", "FrancMug", "PesoMug",
  "BigRocks", "LitlRoks", "TBamboo"
};

int Shakers :: setupName( char* instr )
{
  int which = 0;

  for ( int i=0; i<NUM_INSTR; i++ )	{
    if ( !strcmp(instr,instrs[i]) )
	    which = i;
  }

  return this->setupNum( which );
}

void Shakers :: setFinalZs( StkFloat z0, StkFloat z1, StkFloat z2 )
{
  finalZCoeffs_[0] = z0;
  finalZCoeffs_[1] = z1;
  finalZCoeffs_[2] = z2;
}

void Shakers :: setDecays( StkFloat sndDecay, StkFloat sysDecay )
{
  soundDecay_ = sndDecay;
  systemDecay_ = sysDecay;
}

int Shakers :: setFreqAndReson( int which, StkFloat freq, StkFloat reson )
{
  if ( which < MAX_FREQS )	{
    resons_[which] = reson;
    center_freqs_[which] = freq;
    t_center_freqs_[which] = freq;
    coeffs_[which][1] = reson * reson;
    coeffs_[which][0] = -reson * 2.0 * cos(freq * TWO_PI / Stk::sampleRate());
    return 1;
  }
  else return 0;
}

int Shakers :: setupNum( int inst )
{
  int i, rv = 0;
  StkFloat temp;

  if (inst == 1) { // Cabasa
    rv = inst;
    nObjects_ = CABA_NUM_BEADS;
    defObjs_[inst] = CABA_NUM_BEADS;
    setDecays(CABA_SOUND_DECAY, CABA_SYSTEM_DECAY);
    defDecays_[inst] = CABA_SYSTEM_DECAY;
    decayScale_[inst] = 0.97;
    nFreqs_ = 1;
    baseGain_ = CABA_GAIN;
    temp = log(nObjects_) * baseGain_ / (StkFloat) nObjects_;
    gains_[0] = temp;
    freqalloc_[0] = 0;
    setFreqAndReson(0,CABA_CENTER_FREQ,CABA_RESON);
    setFinalZs(1.0,-1.0,0.0);
  }
  else if (inst == 2) { // Sekere
    rv = inst;
    nObjects_ = SEKE_NUM_BEANS;
    defObjs_[inst] = SEKE_NUM_BEANS;
    this->setDecays(SEKE_SOUND_DECAY,SEKE_SYSTEM_DECAY);
    defDecays_[inst] = SEKE_SYSTEM_DECAY;
    decayScale_[inst] = 0.94;
    nFreqs_ = 1;
    baseGain_ = SEKE_GAIN;
    temp = log(nObjects_) * baseGain_ / (StkFloat) nObjects_;
    gains_[0] = temp;
    freqalloc_[0] = 0;
    this->setFreqAndReson(0,SEKE_CENTER_FREQ,SEKE_RESON);
    this->setFinalZs(1.0, 0.0, -1.0);
  }
  else if (inst == 3) { //  Guiro
    rv = inst;
    nObjects_ = GUIR_NUM_PARTS;
    defObjs_[inst] = GUIR_NUM_PARTS;
    setDecays(GUIR_SOUND_DECAY,1.0);
    defDecays_[inst] = 0.9999;
    decayScale_[inst] = 1.0;
    nFreqs_ = 2;
    baseGain_ = GUIR_GAIN;
    temp = log(nObjects_) * baseGain_ / (StkFloat) nObjects_;
    gains_[0]=temp;
    gains_[1]=temp;
    freqalloc_[0] = 0;
    freqalloc_[1] = 0;
    freq_rand_[0] = 0.0;
    freq_rand_[1] = 0.0;
    setFreqAndReson(0,GUIR_GOURD_FREQ,GUIR_GOURD_RESON);
    setFreqAndReson(1,GUIR_GOURD_FREQ2,GUIR_GOURD_RESON2);
    ratchet_ = 0;
    ratchetPos_ = 10;
  }
  else if (inst == 4) { //  Water Drops
    rv = inst;
    nObjects_ = WUTR_NUM_SOURCES;
    defObjs_[inst] = WUTR_NUM_SOURCES;
    setDecays(WUTR_SOUND_DECAY,WUTR_SYSTEM_DECAY);
    defDecays_[inst] = WUTR_SYSTEM_DECAY;
    decayScale_[inst] = 0.8;
    nFreqs_ = 3;
    baseGain_ = WUTR_GAIN;
    temp = log(nObjects_) * baseGain_ / (StkFloat) nObjects_;
    gains_[0]=temp;
    gains_[1]=temp;
    gains_[2]=temp;
    freqalloc_[0] = 1;
    freqalloc_[1] = 1;
    freqalloc_[2] = 1;
    freq_rand_[0] = 0.2;
    freq_rand_[1] = 0.2;
    freq_rand_[2] = 0.2;
    setFreqAndReson(0,WUTR_CENTER_FREQ0,WUTR_RESON);
    setFreqAndReson(1,WUTR_CENTER_FREQ0,WUTR_RESON);
    setFreqAndReson(2,WUTR_CENTER_FREQ0,WUTR_RESON);
    setFinalZs(1.0,0.0,0.0);
  }
  else if (inst == 5) { // Bamboo
    rv = inst;
    nObjects_ = BAMB_NUM_TUBES;
    defObjs_[inst] = BAMB_NUM_TUBES;
    setDecays(BAMB_SOUND_DECAY, BAMB_SYSTEM_DECAY);
    defDecays_[inst] = BAMB_SYSTEM_DECAY;
    decayScale_[inst] = 0.7;
    nFreqs_ = 3;
    baseGain_ = BAMB_GAIN;
    temp = log(nObjects_) * baseGain_ / (StkFloat) nObjects_;
    gains_[0]=temp;
    gains_[1]=temp;
    gains_[2]=temp;
    freqalloc_[0] = 1;
    freqalloc_[1] = 1;
    freqalloc_[2] = 1;
    freq_rand_[0] = 0.2;
    freq_rand_[1] = 0.2;
    freq_rand_[2] = 0.2;
    setFreqAndReson(0,BAMB_CENTER_FREQ0,BAMB_RESON);
    setFreqAndReson(1,BAMB_CENTER_FREQ1,BAMB_RESON);
    setFreqAndReson(2,BAMB_CENTER_FREQ2,BAMB_RESON);
    setFinalZs(1.0,0.0,0.0);
  }
  else if (inst == 6) { // Tambourine
    rv = inst;
    nObjects_ = TAMB_NUM_TIMBRELS;
    defObjs_[inst] = TAMB_NUM_TIMBRELS;
    setDecays(TAMB_SOUND_DECAY,TAMB_SYSTEM_DECAY);
    defDecays_[inst] = TAMB_SYSTEM_DECAY;
    decayScale_[inst] = 0.95;
    nFreqs_ = 3;
    baseGain_ = TAMB_GAIN;
    temp = log(nObjects_) * baseGain_ / (StkFloat) nObjects_;
    gains_[0]=temp*TAMB_SHELL_GAIN;
    gains_[1]=temp*0.8;
    gains_[2]=temp;
    freqalloc_[0] = 0;
    freqalloc_[1] = 1;
    freqalloc_[2] = 1;
    freq_rand_[0] = 0.0;
    freq_rand_[1] = 0.05;
    freq_rand_[2] = 0.05;
    setFreqAndReson(0,TAMB_SHELL_FREQ,TAMB_SHELL_RESON);
    setFreqAndReson(1,TAMB_CYMB_FREQ1,TAMB_CYMB_RESON);
    setFreqAndReson(2,TAMB_CYMB_FREQ2,TAMB_CYMB_RESON);
    setFinalZs(1.0,0.0,-1.0);
  }
  else if (inst == 7) { // Sleighbell
    rv = inst;
    nObjects_ = SLEI_NUM_BELLS;
    defObjs_[inst] = SLEI_NUM_BELLS;
    setDecays(SLEI_SOUND_DECAY,SLEI_SYSTEM_DECAY);
    defDecays_[inst] = SLEI_SYSTEM_DECAY;
    decayScale_[inst] = 0.9;
    nFreqs_ = 5;
    baseGain_ = SLEI_GAIN;
    temp = log(nObjects_) * baseGain_ / (StkFloat) nObjects_;
    gains_[0]=temp;
    gains_[1]=temp;
    gains_[2]=temp;
    gains_[3]=temp*0.5;
    gains_[4]=temp*0.3;
    for (i=0;i<nFreqs_;i++)	{
	    freqalloc_[i] = 1;
	    freq_rand_[i] = 0.03;
    }
    setFreqAndReson(0,SLEI_CYMB_FREQ0,SLEI_CYMB_RESON);
    setFreqAndReson(1,SLEI_CYMB_FREQ1,SLEI_CYMB_RESON);
    setFreqAndReson(2,SLEI_CYMB_FREQ2,SLEI_CYMB_RESON);
    setFreqAndReson(3,SLEI_CYMB_FREQ3,SLEI_CYMB_RESON);
    setFreqAndReson(4,SLEI_CYMB_FREQ4,SLEI_CYMB_RESON);
    setFinalZs(1.0,0.0,-1.0);
  }
  else if (inst == 8) { // Stix1
    rv = inst;
    nObjects_ = STIX1_NUM_BEANS;
    defObjs_[inst] = STIX1_NUM_BEANS;
    setDecays(STIX1_SOUND_DECAY,STIX1_SYSTEM_DECAY);
    defDecays_[inst] = STIX1_SYSTEM_DECAY;

    decayScale_[inst] = 0.96;
    nFreqs_ = 1;
    baseGain_ = STIX1_GAIN;
    temp = log(nObjects_) * baseGain_ / (StkFloat) nObjects_;
    gains_[0]=temp;
    freqalloc_[0] = 0;
    setFreqAndReson(0,STIX1_CENTER_FREQ,STIX1_RESON);
    setFinalZs(1.0,0.0,-1.0);
  }
  else if (inst == 9) { // Crunch1
    rv = inst;
    nObjects_ = CRUNCH1_NUM_BEADS;
    defObjs_[inst] = CRUNCH1_NUM_BEADS;
    setDecays(CRUNCH1_SOUND_DECAY,CRUNCH1_SYSTEM_DECAY);
    defDecays_[inst] = CRUNCH1_SYSTEM_DECAY;
    decayScale_[inst] = 0.96;
    nFreqs_ = 1;
    baseGain_ = CRUNCH1_GAIN;
    temp = log(nObjects_) * baseGain_ / (StkFloat) nObjects_;
    gains_[0]=temp;
    freqalloc_[0] = 0;
    setFreqAndReson(0,CRUNCH1_CENTER_FREQ,CRUNCH1_RESON);
    setFinalZs(1.0,-1.0,0.0);
  }
  else if (inst == 10) { // Wrench
    rv = inst;
    nObjects_ = WRENCH_NUM_PARTS;
    defObjs_[inst] = WRENCH_NUM_PARTS;
    setDecays(WRENCH_SOUND_DECAY,1.0);
    defDecays_[inst] = 0.9999;
    decayScale_[inst] = 0.98;
    nFreqs_ = 2;
    baseGain_ = WRENCH_GAIN;
    temp = log(nObjects_) * baseGain_ / (StkFloat) nObjects_;
    gains_[0]=temp;
    gains_[1]=temp;
    freqalloc_[0] = 0;
    freqalloc_[1] = 0;
    freq_rand_[0] = 0.0;
    freq_rand_[1] = 0.0;
    setFreqAndReson(0,WRENCH_FREQ,WRENCH_RESON);
    setFreqAndReson(1,WRENCH_FREQ2,WRENCH_RESON2);
    ratchet_ = 0;
    ratchetPos_ = 10;
  }
  else if (inst == 11) { // Sandpapr
    rv = inst;
    nObjects_ = SANDPAPR_NUM_GRAINS;
    defObjs_[inst] = SANDPAPR_NUM_GRAINS;
    this->setDecays(SANDPAPR_SOUND_DECAY,SANDPAPR_SYSTEM_DECAY);
    defDecays_[inst] = SANDPAPR_SYSTEM_DECAY;
    decayScale_[inst] = 0.97;
    nFreqs_ = 1;
    baseGain_ = SANDPAPR_GAIN;
    temp = log(nObjects_) * baseGain_ / (StkFloat) nObjects_;
    gains_[0] = temp;
    freqalloc_[0] = 0;
    this->setFreqAndReson(0,SANDPAPR_CENTER_FREQ,SANDPAPR_RESON);
    this->setFinalZs(1.0, 0.0, -1.0);
  }
  else if (inst == 12) { // Cokecan
    rv = inst;
    nObjects_ = COKECAN_NUM_PARTS;
    defObjs_[inst] = COKECAN_NUM_PARTS;
    setDecays(COKECAN_SOUND_DECAY,COKECAN_SYSTEM_DECAY);
    defDecays_[inst] = COKECAN_SYSTEM_DECAY;
    decayScale_[inst] = 0.95;
    nFreqs_ = 5;
    baseGain_ = COKECAN_GAIN;
    temp = log(nObjects_) * baseGain_ / (StkFloat) nObjects_;
    gains_[0]=temp;
    gains_[1]=temp*1.8;
    gains_[2]=temp*1.8;
    gains_[3]=temp*1.8;
    gains_[4]=temp*1.8;
    freqalloc_[0] = 0;
    freqalloc_[1] = 0;
    freqalloc_[2] = 0;
    freqalloc_[3] = 0;
    freqalloc_[4] = 0;
    setFreqAndReson(0,COKECAN_HELMFREQ,COKECAN_HELM_RES);
    setFreqAndReson(1,COKECAN_METLFREQ0,COKECAN_METL_RES);
    setFreqAndReson(2,COKECAN_METLFREQ1,COKECAN_METL_RES);
    setFreqAndReson(3,COKECAN_METLFREQ2,COKECAN_METL_RES);
    setFreqAndReson(4,COKECAN_METLFREQ3,COKECAN_METL_RES);
    setFinalZs(1.0,0.0,-1.0);
  }
  else if (inst>12 && inst<20) { // Nextmug
    rv = inst;
    nObjects_ = NEXTMUG_NUM_PARTS;
    defObjs_[inst] = NEXTMUG_NUM_PARTS;
    setDecays(NEXTMUG_SOUND_DECAY,NEXTMUG_SYSTEM_DECAY);
    defDecays_[inst] = NEXTMUG_SYSTEM_DECAY;
    decayScale_[inst] = 0.95;
    nFreqs_ = 4;
    baseGain_ = NEXTMUG_GAIN;
    temp = log(nObjects_) * baseGain_ / (StkFloat) nObjects_;
    gains_[0]=temp;
    gains_[1]=temp*0.8;
    gains_[2]=temp*0.6;
    gains_[3]=temp*0.4;
    freqalloc_[0] = 0;
    freqalloc_[1] = 0;
    freqalloc_[2] = 0;
    freqalloc_[3] = 0;
    freqalloc_[4] = 0;
    freqalloc_[5] = 0;
    setFreqAndReson(0,NEXTMUG_FREQ0,NEXTMUG_RES);
    setFreqAndReson(1,NEXTMUG_FREQ1,NEXTMUG_RES);
    setFreqAndReson(2,NEXTMUG_FREQ2,NEXTMUG_RES);
    setFreqAndReson(3,NEXTMUG_FREQ3,NEXTMUG_RES);
    setFinalZs(1.0,0.0,-1.0);

    if (inst == 14) { // Mug + Penny
      nFreqs_ = 7;
      gains_[4] = temp;
      gains_[5] = temp*0.8;
      gains_[6] = temp*0.5;
      setFreqAndReson(4,PENNY_FREQ0,PENNY_RES);
      setFreqAndReson(5,PENNY_FREQ1,PENNY_RES);
      setFreqAndReson(6,PENNY_FREQ2,PENNY_RES);
    }
    else if (inst == 15) { // Mug + Nickel
      nFreqs_ = 6;
      gains_[4] = temp;
      gains_[5] = temp*0.8;
      gains_[6] = temp*0.5;
      setFreqAndReson(4,NICKEL_FREQ0,NICKEL_RES);
      setFreqAndReson(5,NICKEL_FREQ1,NICKEL_RES);
      setFreqAndReson(6,NICKEL_FREQ2,NICKEL_RES);
    }
    else if (inst == 16) { // Mug + Dime
      nFreqs_ = 6;
      gains_[4] = temp;
      gains_[5] = temp*0.8;
      gains_[6] = temp*0.5;
      setFreqAndReson(4,DIME_FREQ0,DIME_RES);
      setFreqAndReson(5,DIME_FREQ1,DIME_RES);
      setFreqAndReson(6,DIME_FREQ2,DIME_RES);
    }
    else if (inst == 17) { // Mug + Quarter
      nFreqs_ = 6;
      gains_[4] = temp*1.3;
      gains_[5] = temp*1.0;
      gains_[6] = temp*0.8;
      setFreqAndReson(4,QUARTER_FREQ0,QUARTER_RES);
      setFreqAndReson(5,QUARTER_FREQ1,QUARTER_RES);
      setFreqAndReson(6,QUARTER_FREQ2,QUARTER_RES);
    }
    else if (inst == 18) { // Mug + Franc
      nFreqs_ = 6;
      gains_[4] = temp*0.7;
      gains_[5] = temp*0.4;
      gains_[6] = temp*0.3;
      setFreqAndReson(4,FRANC_FREQ0,FRANC_RES);
      setFreqAndReson(5,FRANC_FREQ1,FRANC_RES);
      setFreqAndReson(6,FRANC_FREQ2,FRANC_RES);
    }
    else if (inst == 19) { // Mug + Peso
      nFreqs_ = 6;
      gains_[4] = temp;
      gains_[5] = temp*1.2;
      gains_[6] = temp*0.7;
      setFreqAndReson(4,PESO_FREQ0,PESO_RES);
      setFreqAndReson(5,PESO_FREQ1,PESO_RES);
      setFreqAndReson(6,PESO_FREQ2,PESO_RES);
    }
  }
  else if (inst == 20) { // Big Rocks
    nFreqs_ = 1;
    rv = inst;
    nObjects_ = BIGROCKS_NUM_PARTS;
    defObjs_[inst] = BIGROCKS_NUM_PARTS;
    setDecays(BIGROCKS_SOUND_DECAY,BIGROCKS_SYSTEM_DECAY);
    defDecays_[inst] = BIGROCKS_SYSTEM_DECAY;
    decayScale_[inst] = 0.95;
    baseGain_ = BIGROCKS_GAIN;
    temp = log(nObjects_) * baseGain_ / (StkFloat) nObjects_;
    gains_[0]=temp;
    freqalloc_[0] = 1;
    freq_rand_[0] = 0.11;
    setFreqAndReson(0,BIGROCKS_FREQ,BIGROCKS_RES);
    setFinalZs(1.0,0.0,-1.0);
  }
  else if (inst == 21) { // Little Rocks
    nFreqs_ = 1;
    rv = inst;
    nObjects_ = LITLROCKS_NUM_PARTS;
    defObjs_[inst] = LITLROCKS_NUM_PARTS;
    setDecays(LITLROCKS_SOUND_DECAY,LITLROCKS_SYSTEM_DECAY);
    defDecays_[inst] = LITLROCKS_SYSTEM_DECAY;
    decayScale_[inst] = 0.95;
    baseGain_ = LITLROCKS_GAIN;
    temp = log(nObjects_) * baseGain_ / (StkFloat) nObjects_;
    gains_[0]=temp;
    freqalloc_[0] = 1;
    freq_rand_[0] = 0.18;
    setFreqAndReson(0,LITLROCKS_FREQ,LITLROCKS_RES);
    setFinalZs(1.0,0.0,-1.0);
  }
  else if (inst == 22) { // Tuned Bamboo
    rv = inst;
    nObjects_ = TBAMB_NUM_TUBES;
    defObjs_[inst] = TBAMB_NUM_TUBES;
    setDecays(TBAMB_SOUND_DECAY, TBAMB_SYSTEM_DECAY);
    defDecays_[inst] = TBAMB_SYSTEM_DECAY;
    decayScale_[inst] = 0.7;
    nFreqs_ = 7;
    baseGain_ = TBAMB_GAIN;
    temp = log(nObjects_) * baseGain_ / (StkFloat) nObjects_;
    gains_[0]=temp;
    gains_[1]=temp;
    gains_[2]=temp;
    gains_[3]=temp;
    gains_[4]=temp;
    gains_[5]=temp;
    gains_[6]=temp;
    freqalloc_[0] = 0;
    freqalloc_[1] = 0;
    freqalloc_[2] = 0;
    freqalloc_[3] = 0;
    freqalloc_[4] = 0;
    freqalloc_[5] = 0;
    freqalloc_[6] = 0;
    freq_rand_[0] = 0.0;
    freq_rand_[1] = 0.0;
    freq_rand_[2] = 0.0;
    freq_rand_[3] = 0.0;
    freq_rand_[4] = 0.0;
    freq_rand_[5] = 0.0;
    freq_rand_[6] = 0.0;
    setFreqAndReson(0,TBAMB_CENTER_FREQ0,TBAMB_RESON);
    setFreqAndReson(1,TBAMB_CENTER_FREQ1,TBAMB_RESON);
    setFreqAndReson(2,TBAMB_CENTER_FREQ2,TBAMB_RESON);
    setFreqAndReson(3,TBAMB_CENTER_FREQ3,TBAMB_RESON);
    setFreqAndReson(4,TBAMB_CENTER_FREQ4,TBAMB_RESON);
    setFreqAndReson(5,TBAMB_CENTER_FREQ5,TBAMB_RESON);
    setFreqAndReson(6,TBAMB_CENTER_FREQ6,TBAMB_RESON);
    setFinalZs(1.0,0.0,-1.0);
  }
  else { // Maraca (inst == 0) or default
    rv = 0;
    nObjects_ = MARA_NUM_BEANS;
    defObjs_[0] = MARA_NUM_BEANS;
    setDecays(MARA_SOUND_DECAY,MARA_SYSTEM_DECAY);
    defDecays_[0] = MARA_SYSTEM_DECAY;
    decayScale_[inst] = 0.9;
    nFreqs_ = 1;
    baseGain_ = MARA_GAIN;
    temp = log(nObjects_) * baseGain_ / (StkFloat) nObjects_;
    gains_[0]=temp;
    freqalloc_[0] = 0;
    setFreqAndReson(0,MARA_CENTER_FREQ,MARA_RESON);
    setFinalZs(1.0,-1.0,0.0);
  }
  return rv;
}

void Shakers :: noteOn( StkFloat frequency, StkFloat amplitude )
{
  // Yep ... pretty kludgey, but it works!
  int noteNum = (int) ((12*log(frequency/220.0)/log(2.0)) + 57.01) % 32;
  if (instType_ !=  noteNum) instType_ = this->setupNum(noteNum);

  shakeEnergy_ += amplitude * MAX_SHAKE * 0.1;
  if (shakeEnergy_ > MAX_SHAKE) shakeEnergy_ = MAX_SHAKE;
  if (instType_==10 || instType_==3) ratchetPos_ += 1;
}

void Shakers :: noteOff( StkFloat amplitude )
{
  shakeEnergy_ = 0.0;
  if (instType_==10 || instType_==3) ratchetPos_ = 0;
}

const StkFloat MIN_ENERGY = 0.3;

StkFloat Shakers :: tick( unsigned int )
{
  StkFloat data;
  StkFloat temp_rand;
  int i;

  if (instType_ == 4) {
  	if (shakeEnergy_ > MIN_ENERGY)	{
      lastFrame_[0] = wuter_tick();
      lastFrame_[0] *= 0.0001;
    }
    else {
      lastFrame_[0] = 0.0;
    }
  }
  else if (instType_ == 22) {
    lastFrame_[0] = tbamb_tick();
  }
  else if (instType_ == 10 || instType_ == 3) {
    if (ratchetPos_ > 0) {
      ratchet_ -= (ratchetDelta_ + (0.002*totalEnergy_));
      if (ratchet_ < 0.0) {
        ratchet_ = 1.0;
        ratchetPos_ -= 1;
	    }
      totalEnergy_ = ratchet_;
      lastFrame_[0] = ratchet_tick();
      lastFrame_[0] *= 0.0001;
    }
    else lastFrame_[0] = 0.0;
  }
  else  { // generic_tick()
    if (shakeEnergy_ > MIN_ENERGY) {
      shakeEnergy_ *= systemDecay_;               // Exponential system decay
      if (float_random(1024.0) < nObjects_) {
        sndLevel_ += shakeEnergy_;   
        for (i=0;i<nFreqs_;i++) {
          if (freqalloc_[i])	{
            temp_rand = t_center_freqs_[i] * (1.0 + (freq_rand_[i] * noise_tick()));
            coeffs_[i][0] = -resons_[i] * 2.0 * cos(temp_rand * TWO_PI / Stk::sampleRate());
          }
        }
    	}
      inputs_[0] = sndLevel_ * noise_tick();      // Actual Sound is Random
      for (i=1; i<nFreqs_; i++)	{
        inputs_[i] = inputs_[0];
      }
      sndLevel_ *= soundDecay_;                   // Exponential Sound decay 
      finalZ_[2] = finalZ_[1];
      finalZ_[1] = finalZ_[0];
      finalZ_[0] = 0;
      for (i=0;i<nFreqs_;i++)	{
        inputs_[i] -= outputs_[i][0]*coeffs_[i][0];  // Do
        inputs_[i] -= outputs_[i][1]*coeffs_[i][1];  // resonant
        outputs_[i][1] = outputs_[i][0];            // filter
        outputs_[i][0] = inputs_[i];                // calculations
        finalZ_[0] += gains_[i] * outputs_[i][1];
      }
      data = finalZCoeffs_[0] * finalZ_[0];     // Extra zero(s) for shape
      data += finalZCoeffs_[1] * finalZ_[1];    // Extra zero(s) for shape
      data += finalZCoeffs_[2] * finalZ_[2];    // Extra zero(s) for shape
      if (data > 10000.0)	data = 10000.0;
      if (data < -10000.0) data = -10000.0;
      lastFrame_[0] = data * 0.0001;
    }
    else lastFrame_[0] = 0.0;
  }

  return lastFrame_[0];
}

void Shakers :: controlChange( int number, StkFloat value )
{
#if defined(_STK_DEBUG_)
  if ( Stk::inRange( value, 0.0, 128.0 ) == false ) {
    oStream_ << "Shakers::controlChange: value (" << value << ") is out of range!";
    handleError( StkError::WARNING ); return;
  }
#endif

  int i;
  StkFloat temp;
  StkFloat normalizedValue = value * ONE_OVER_128;
  if (number == __SK_Breath_) { // 2 ... energy
    shakeEnergy_ += normalizedValue * MAX_SHAKE * 0.1;
    if (shakeEnergy_ > MAX_SHAKE) shakeEnergy_ = MAX_SHAKE;
    if (instType_==10 || instType_==3) {
	    ratchetPos_ = (int) fabs(value - lastRatchetPos_);
	    ratchetDelta_ = 0.0002 * ratchetPos_;
	    lastRatchetPos_ = (int) value;
    }
  }
  else if (number == __SK_ModFrequency_) { // 4 ... decay
    if (instType_ != 3 && instType_ != 10) {
      systemDecay_ = defDecays_[instType_] + ((value - 64.0) *
                                           decayScale_[instType_] *
                                           (1.0 - defDecays_[instType_]) / 64.0 );
      gains_[0] = log(nObjects_) * baseGain_ / (StkFloat) nObjects_;
      for (i=1;i<nFreqs_;i++) gains_[i] = gains_[0];
      if (instType_ == 6) { // tambourine
        gains_[0] *= TAMB_SHELL_GAIN;
        gains_[1] *= 0.8;
      }
      else if (instType_ == 7) { // sleighbell
        gains_[3] *= 0.5;
        gains_[4] *= 0.3;
      }
      else if (instType_ == 12) { // cokecan
        for (i=1;i<nFreqs_;i++) gains_[i] *= 1.8;
      }
      for (i=0;i<nFreqs_;i++) gains_[i] *= ((128-value)/100.0 + 0.36);
    }
  }
  else if (number == __SK_FootControl_) { // 11 ... number of objects
    if (instType_ == 5) // bamboo
      nObjects_ = (StkFloat) (value * defObjs_[instType_] / 64.0) + 0.3;
    else
      nObjects_ = (StkFloat) (value * defObjs_[instType_] / 64.0) + 1.1;
    gains_[0] = log(nObjects_) * baseGain_ / (StkFloat) nObjects_;
    for (i=1;i<nFreqs_;i++) gains_[i] = gains_[0];
    if (instType_ == 6) { // tambourine
      gains_[0] *= TAMB_SHELL_GAIN;
      gains_[1] *= 0.8;
    }
    else if (instType_ == 7) { // sleighbell
      gains_[3] *= 0.5;
      gains_[4] *= 0.3;
    }
    else if (instType_ == 12) { // cokecan
      for (i=1;i<nFreqs_;i++) gains_[i] *= 1.8;
    }
    if (instType_ != 3 && instType_ != 10) {
      // reverse calculate decay setting
      double temp = (double) (64.0 * (systemDecay_-defDecays_[instType_])/(decayScale_[instType_]*(1-defDecays_[instType_])) + 64.0);
      // scale gains_ by decay setting
      for (i=0;i<nFreqs_;i++) gains_[i] *= ((128-temp)/100.0 + 0.36);
    }
  }
  else if (number == __SK_ModWheel_) { // 1 ... resonance frequency
    for (i=0; i<nFreqs_; i++)	{
      if (instType_ == 6 || instType_ == 2 || instType_ == 7) // limit range a bit for tambourine
        temp = center_freqs_[i] * pow (1.008,value-64);
      else
        temp = center_freqs_[i] * pow (1.015,value-64);
      t_center_freqs_[i] = temp;

      coeffs_[i][0] = -resons_[i] * 2.0 * cos(temp * TWO_PI / Stk::sampleRate());
      coeffs_[i][1] = resons_[i]*resons_[i];
    }
  }
  else if (number == __SK_AfterTouch_Cont_) { // 128
    shakeEnergy_ += normalizedValue * MAX_SHAKE * 0.1;
    if (shakeEnergy_ > MAX_SHAKE) shakeEnergy_ = MAX_SHAKE;
    if (instType_==10 || instType_==3)	{
	    ratchetPos_ = (int) fabs(value - lastRatchetPos_);
	    ratchetDelta_ = 0.0002 * ratchetPos_;
	    lastRatchetPos_ = (int) value;
    }
  }
  else  if (number == __SK_ShakerInst_) { // 1071
    instType_ = (int) (value + 0.5);	//  Just to be safe
    this->setupNum(instType_);
  }
#if defined(_STK_DEBUG_)
  else {
    oStream_ << "Shakers::controlChange: undefined control number (" << number << ")!";
    handleError( StkError::WARNING );
  }
#endif
}

// KLUDGE-O-MATIC-O-RAMA

StkFloat Shakers :: wuter_tick( void ) {
  StkFloat data;
  int j;
  shakeEnergy_ *= systemDecay_;               // Exponential system decay
  if (my_random(32767) < nObjects_) {     
    sndLevel_ = shakeEnergy_;   
    j = my_random(3);
	  if (j == 0)   {
      center_freqs_[0] = WUTR_CENTER_FREQ1 * (0.75 + (0.25 * noise_tick()));
	    gains_[0] = fabs(noise_tick());
	  }
	  else if (j == 1)      {
      center_freqs_[1] = WUTR_CENTER_FREQ1 * (1.0 + (0.25 * noise_tick()));
	    gains_[1] = fabs(noise_tick());
	  }
	  else  {
      center_freqs_[2] = WUTR_CENTER_FREQ1 * (1.25 + (0.25 * noise_tick()));
	    gains_[2] = fabs(noise_tick());
	  }
	}
	
  gains_[0] *= resons_[0];
  if (gains_[0] >  0.001) {
    center_freqs_[0]  *= WUTR_FREQ_SWEEP;
    coeffs_[0][0] = -resons_[0] * 2.0 * 
      cos(center_freqs_[0] * TWO_PI / Stk::sampleRate());
  }
  gains_[1] *= resons_[1];
  if (gains_[1] > 0.001) {
    center_freqs_[1] *= WUTR_FREQ_SWEEP;
    coeffs_[1][0] = -resons_[1] * 2.0 * 
      cos(center_freqs_[1] * TWO_PI / Stk::sampleRate());
  }
  gains_[2] *= resons_[2];
  if (gains_[2] > 0.001) {
    center_freqs_[2] *= WUTR_FREQ_SWEEP;
    coeffs_[2][0] = -resons_[2] * 2.0 * 
      cos(center_freqs_[2] * TWO_PI / Stk::sampleRate());
  }
	
  sndLevel_ *= soundDecay_;        // Each (all) event(s) 
                                   // decay(s) exponentially 
  inputs_[0] = sndLevel_;
  inputs_[0] *= noise_tick();     // Actual Sound is Random
  inputs_[1] = inputs_[0] * gains_[1];
  inputs_[2] = inputs_[0] * gains_[2];
  inputs_[0] *= gains_[0];
  inputs_[0] -= outputs_[0][0]*coeffs_[0][0];
  inputs_[0] -= outputs_[0][1]*coeffs_[0][1];
  outputs_[0][1] = outputs_[0][0];
  outputs_[0][0] = inputs_[0];
  data = gains_[0]*outputs_[0][0];
  inputs_[1] -= outputs_[1][0]*coeffs_[1][0];
  inputs_[1] -= outputs_[1][1]*coeffs_[1][1];
  outputs_[1][1] = outputs_[1][0];
  outputs_[1][0] = inputs_[1];
  data += gains_[1]*outputs_[1][0];
  inputs_[2] -= outputs_[2][0]*coeffs_[2][0];
  inputs_[2] -= outputs_[2][1]*coeffs_[2][1];
  outputs_[2][1] = outputs_[2][0];
  outputs_[2][0] = inputs_[2];
  data += gains_[2]*outputs_[2][0];
 
  finalZ_[2] = finalZ_[1];
  finalZ_[1] = finalZ_[0];
  finalZ_[0] = data * 4;

  data = finalZ_[2] - finalZ_[0];
  return data;
}

StkFloat Shakers :: ratchet_tick( void ) {
  StkFloat data;
  if (my_random(1024) < nObjects_) {
    sndLevel_ += 512 * ratchet_ * totalEnergy_;
  }
  inputs_[0] = sndLevel_;
  inputs_[0] *= noise_tick() * ratchet_;
  sndLevel_ *= soundDecay_;
		 
  inputs_[1] = inputs_[0];
  inputs_[0] -= outputs_[0][0]*coeffs_[0][0];
  inputs_[0] -= outputs_[0][1]*coeffs_[0][1];
  outputs_[0][1] = outputs_[0][0];
  outputs_[0][0] = inputs_[0];
  inputs_[1] -= outputs_[1][0]*coeffs_[1][0];
  inputs_[1] -= outputs_[1][1]*coeffs_[1][1];
  outputs_[1][1] = outputs_[1][0];
  outputs_[1][0] = inputs_[1];
     
  finalZ_[2] = finalZ_[1];
  finalZ_[1] = finalZ_[0];
  finalZ_[0] = gains_[0]*outputs_[0][1] + gains_[1]*outputs_[1][1];
  data = finalZ_[0] - finalZ_[2];
  return data;
}

StkFloat Shakers :: tbamb_tick( void ) {
  StkFloat data, temp;
  static int which = 0;
  int i;

  if (shakeEnergy_ > MIN_ENERGY)	{
      shakeEnergy_ *= systemDecay_;    // Exponential system decay
      if (float_random(1024.0) < nObjects_) {
	    sndLevel_ += shakeEnergy_;
	    which = my_random(7);
	  }  
      temp = sndLevel_ * noise_tick();      // Actual Sound is Random
	  for (i=0;i<nFreqs_;i++)	inputs_[i] = 0;
	  inputs_[which] = temp;
      sndLevel_ *= soundDecay_;                   // Exponential Sound decay 
      finalZ_[2] = finalZ_[1];
      finalZ_[1] = finalZ_[0];
      finalZ_[0] = 0;
      for (i=0;i<nFreqs_;i++)	{
        inputs_[i] -= outputs_[i][0]*coeffs_[i][0];  // Do
        inputs_[i] -= outputs_[i][1]*coeffs_[i][1];  // resonant
        outputs_[i][1] = outputs_[i][0];            // filter
        outputs_[i][0] = inputs_[i];                // calculations
        finalZ_[0] += gains_[i] * outputs_[i][1];
      }
      data = finalZCoeffs_[0] * finalZ_[0];     // Extra zero(s) for shape
      data += finalZCoeffs_[1] * finalZ_[1];    // Extra zero(s) for shape
      data += finalZCoeffs_[2] * finalZ_[2];    // Extra zero(s) for shape
      if (data > 10000.0)	data = 10000.0;
      if (data < -10000.0) data = -10000.0;
      data = data * 0.0001;
  }
  else data = 0.0;
  return data;
}

} // stk namespace
