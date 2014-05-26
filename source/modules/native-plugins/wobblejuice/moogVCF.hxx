/*
 * Moog-like resonant LPF
 * Implemented by Andre Sklenar <andre.sklenar@gmail.com>, www.juicelab.cz .
 *
 * Filter design from http://www.musicdsp.com .
 */

#ifndef MOOG_VCF_HXX_INCLUDED
#define MOOG_VCF_HXX_INCLUDED

#include <cmath>
#include <cstdlib>

class MoogVCF
{
public:
    MoogVCF()
    {
        cutoff = 16000;
        res = 0.5;
        f=k=p=scale=r=0;
        y1=y2=y3=y4=oldIn=oldY1=oldY2=oldY3=0;
        in=oldIn=0;
        pureInput=drivenInput=processedInput=0;
    }

    void recalc(float cutoff, float reso, int sr, float nDrive)
    {
        f = 2*cutoff/sr;
        k=2*std::sin(f*M_PI/2)-1;
        p = (k+1)*0.5;
        scale = std::pow(2.71828, (1-p)*1.386249);
        r = reso*scale;
        drive = nDrive;
    }

    void process (long frames, const float* inputs, float* outputs)
    {
        //run the shit
        for (long i=0; i<frames; i++)
        {
            pureInput = inputs[i]; //clean signal
            drivenInput = std::tanh(pureInput*(drive*15+1)) * (drive); //a touch of waveshaping
            processedInput = pureInput*(1-drive) + drivenInput; // combine
            processedInput*=1-drive/3; //reduce gain a little

            /* filter */
            in = processedInput-r*y4;
            y1 = in*p + oldIn*p - k*y1;
            y2 = y1*p + oldY1*p - k*y2;
            y3 = y2*p + oldY2*p - k*y3;
            y4 = y3*p + oldY3*p - k*y4;

            oldIn = in;
            oldY1 = y1;
            oldY2 = y2;
            oldY3 = y3;

            /* output */
            outputs[i] = y4;
        }
    }

private:
    /* vcf filter */
    float cutoff; //freq in Hz
    float res; //resonance 0..1
    float drive; //drive 1...2;
    float f, k, p, scale, r;
    float y1, y2, y3, y4, oldY1, oldY2, oldY3;
    float in, oldIn;
    /* waveshaping vars */
    float pureInput, drivenInput, processedInput;
};

#endif // MOOG_VCF_HXX_INCLUDED
