/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2008 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2008 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU Lesser General Public License, as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================

   @author  rockhardbuns
   @tweaker Lucio Asnaghi

 ==============================================================================
*/

#ifndef __JUCETICE_VEXCSYNTMODULE_HEADER__
#define __JUCETICE_VEXCSYNTMODULE_HEADER__

#ifdef CARLA_EXPORT
 #include "JuceHeader.h"
#else
 #include "../StandardHeader.h"
#endif

#include "cVoice.h"

class VexSyntModule
{
public:
    VexSyntModule(const float* const p)
        : parameters(p),
          sampleRate(44100),
          benchwarmer(0),
          playCount(1),
          part1(false),
          part2(false),
          part3(false)
    {
        for (int i = 0; i < kNumVoices; ++i)
        {
            vo1[i] = new VexVoice(p,  0, wr1);
            vo2[i] = new VexVoice(p, 24, wr2);
            vo3[i] = new VexVoice(p, 48, wr3);
        }
    }

    ~VexSyntModule()
    {
        for (int i = 0; i < kNumVoices; ++i)
        {
            delete vo1[i];
            delete vo2[i];
            delete vo3[i];
        }
    }

#ifdef CARLA_EXPORT
    void doProcess(float* const outPtrL, float* const outPtrR, const int numSamples)
    {
#else
    void doProcess(AudioSampleBuffer& assbf, AudioSampleBuffer& obf, AudioSampleBuffer& ebf1, AudioSampleBuffer& ebf2, AudioSampleBuffer& ebf3)
    {
        const int numSamples = obf.getNumSamples();
        float* const outPtrL = assbf.getSampleData(0,0);
        float* const outPtrR = assbf.getSampleData(1,0);
#endif

        if (part1)
        {
#ifndef CARLA_EXPORT
            float right = parameters[86] * parameters[83];
            float left  = parameters[86] * (1.0f - parameters[83]);
#endif

            for (int i = 0; i < kNumVoices; ++i)
            {
                if (vo1[i]->getIsOn())
                {
                    vo1[i]->doProcess(outPtrL, outPtrR, numSamples);
#ifndef CARLA_EXPORT
                    obf.addFrom(0, 0,  assbf, 0, 0, numSamples, left);
                    obf.addFrom(1, 0,  assbf, 1, 0, numSamples, right);
                    ebf1.addFrom(0, 0, assbf, 0, 0, numSamples, parameters[22] * left);
                    ebf1.addFrom(1, 0, assbf, 1, 0, numSamples, parameters[22] * right);
                    ebf2.addFrom(0, 0, assbf, 0, 0, numSamples, parameters[23] * left);
                    ebf2.addFrom(1, 0, assbf, 1, 0, numSamples, parameters[23] * right);
                    ebf3.addFrom(0, 0, assbf, 0, 0, numSamples, parameters[24] * left);
                    ebf3.addFrom(1, 0, assbf, 1, 0, numSamples, parameters[24] * right);
#endif
                }
            }
        }

        if (part2)
        {
#ifndef CARLA_EXPORT
            float right = parameters[87] * parameters[84];
            float left  = parameters[87] * (1.0f - parameters[84]);
#endif

            for (int i = 0; i < kNumVoices; ++i)
            {
                if (vo2[i]->getIsOn())
                {
                    vo2[i]->doProcess(outPtrL, outPtrR, numSamples);
#ifndef CARLA_EXPORT
                    obf.addFrom(0, 0,  assbf, 0, 0, numSamples, left);
                    obf.addFrom(1, 0,  assbf, 1, 0, numSamples, right);
                    ebf1.addFrom(0, 0, assbf, 0, 0, numSamples, parameters[22 + 24] * left);
                    ebf1.addFrom(1, 0, assbf, 1, 0, numSamples, parameters[22 + 24] * right);
                    ebf2.addFrom(0, 0, assbf, 0, 0, numSamples, parameters[23 + 24] * left);
                    ebf2.addFrom(1, 0, assbf, 1, 0, numSamples, parameters[23 + 24] * right);
                    ebf3.addFrom(0, 0, assbf, 0, 0, numSamples, parameters[24 + 24] * left);
                    ebf3.addFrom(1, 0, assbf, 1, 0, numSamples, parameters[24 + 24] * right);
#endif
                }
            }
        }

        if (part3)
        {
#ifndef CARLA_EXPORT
            float right = parameters[88] * parameters[85];
            float left  = parameters[88] * (1.0f - parameters[85]);
#endif

            for (int i = 0; i < kNumVoices; ++i)
            {
                if (vo3[i]->getIsOn())
                {
                    vo3[i]->doProcess(outPtrL, outPtrR, numSamples);
#ifndef CARLA_EXPORT
                    obf.addFrom(0, 0,  assbf, 0, 0, numSamples, left);
                    obf.addFrom(1, 0,  assbf, 1, 0, numSamples, right);
                    ebf1.addFrom(0, 0, assbf, 0, 0, numSamples, parameters[22 + 48] * left);
                    ebf1.addFrom(1, 0, assbf, 1, 0, numSamples, parameters[22 + 48] * right);
                    ebf2.addFrom(0, 0, assbf, 0, 0, numSamples, parameters[23 + 48] * left);
                    ebf2.addFrom(1, 0, assbf, 1, 0, numSamples, parameters[23 + 48] * right);
                    ebf3.addFrom(0, 0, assbf, 0, 0, numSamples, parameters[24 + 48] * left);
                    ebf3.addFrom(1, 0, assbf, 1, 0, numSamples, parameters[24 + 48] * right);
#endif
                }
            }
        }
    }

    void setSampleRate(const double s)
    {
        sampleRate = s;
    }

    void updateParameterPtr(const float* const p)
    {
        parameters = p;

        for (int i = 0; i < kNumVoices; ++i)
        {
            vo1[i]->updateParameterPtr(parameters);
            vo2[i]->updateParameterPtr(parameters);
            vo3[i]->updateParameterPtr(parameters);
        }
    }

    void playNote(const int n, const int vel, const int preroll, const int part)
    {
        VexVoice** v   = nullptr;
        const int note = n + 12;

        switch (part)
        {
        case 1:
            if (!part1) return;
            v = vo1;
            break;
        case 2:
            if (!part2) return;
            v = vo2;
            break;
        case 3:
            if (!part3) return;
            v = vo3;
            break;
        }

        if (v == nullptr)
            return;

        int OldestOn = kNumVoices-1;
        int OldestOff = kNumVoices-1;
        int OldestReleased = kNumVoices-1;

        int tmpOn = 100000000;
        //int tmpOff = 100000000;
        int tmpReleased = 100000000;

        for (int i = 0; i < kNumVoices; ++i)
        {
            if (i == benchwarmer)
                continue;

            if (! v[i]->getIsOn())
            {
                OldestOff = i;
                break;
            }

            if (vo1[i]->getIsReleased())
            {
                OldestReleased = (v[i]->getOrdinal() < tmpReleased) ? i : OldestReleased;
                tmpReleased = v[OldestReleased]->getOrdinal();
                continue;
            }

            OldestOn = (v[i]->getOrdinal() < tmpOn) ? i : OldestOn;
            tmpOn = v[OldestOn]->getOrdinal();
        }

        float noteInHertz = (float)MidiMessage::getMidiNoteInHertz(note);
        playCount++;

        if (OldestOff < kNumVoices)
        {
            v[OldestOff]->start(noteInHertz, float(vel)/127.0f, note, preroll, sampleRate, playCount);
            return;
        }

        if (OldestReleased < kNumVoices)
        {
            v[benchwarmer]->start(noteInHertz, float(vel)/127.0f, note, preroll, sampleRate, playCount);
            benchwarmer = OldestReleased;
            v[OldestReleased]->quickRelease();
            return;
        }

        if (OldestOn < kNumVoices)
        {
            v[benchwarmer]->start(noteInHertz, float(vel)/127.0f, note, preroll, sampleRate, playCount);
            benchwarmer = OldestOn;
            v[OldestReleased]->quickRelease();
            return;
        }
    }

    void releaseNote(const int n, const int preroll, const int part)
    {
        VexVoice** v   = nullptr;
        const int note = n + 12;

        switch (part)
        {
        case 1:
            if (!part1) return;
            v = vo1;
            break;
        case 2:
            if (!part2) return;
            v = vo2;
            break;
        case 3:
            if (!part3) return;
            v = vo3;
            break;
        }

        if (v == nullptr)
            return;

        for (int i = 0; i < kNumVoices; ++i)
        {
            if (v[i]->getNote() == note)
                v[i]->release(preroll);
        }
    }

    void releaseAll(const int p)
    {
        for (int i = 0; i < kNumVoices; ++i)
        {
            vo1[i]->release(p);
            vo2[i]->release(p);
            vo3[i]->release(p);
        }
    }

    void kill(const int what = 0)
    {
        switch (what)
        {
        case 0:
            for (int i = 0; i < kNumVoices; ++i)
            {
                vo1[i]->kill();
                vo2[i]->kill();
                vo3[i]->kill();
            }
            break;

        case 1:
            for (int i = 0; i < kNumVoices; ++i)
                vo1[i]->kill();
            break;

        case 2:
            for (int i = 0; i < kNumVoices; ++i)
                vo2[i]->kill();
            break;

        case 3:
            for (int i = 0; i < kNumVoices; ++i)
                vo3[i]->kill();
            break;
        }
    }

    void setWaveLater(const int part, const String& waveName)
    {
        switch (part)
        {
        case 1:
            wr1.setWaveLater(waveName);
            kill(1);
            // REMOVE THIS
            wr1.actuallySetWave();
            break;
        case 2:
            wr2.setWaveLater(waveName);
            kill(2);
            break;
        case 3:
            wr3.setWaveLater(waveName);
            kill(3);
            break;
        }
    }

    void update(const int index)
    {
        if (index == 89)
        {
            part1 = (parameters[89] > 0.9f);
            return;
        }
        if (index == 90)
        {
            part2 = (parameters[90] > 0.9f);
            return;
        }
        if (index == 91)
        {
            part3 = (parameters[91] > 0.9f);
            return;
        }

        for (int i = 0; i < kNumVoices; ++i)
        {
            vo1[i]->update(index);
            vo2[i]->update(index);
            vo3[i]->update(index);
        }
    }

private:
    static const int kNumVoices = 8;

    const float* parameters;

    double sampleRate;
    int benchwarmer;

    VexVoice* vo1[kNumVoices];
    VexVoice* vo2[kNumVoices];
    VexVoice* vo3[kNumVoices];

    long playCount;
    bool part1, part2, part3;

    WaveRenderer wr1;
    WaveRenderer wr2;
    WaveRenderer wr3;
};

#endif
