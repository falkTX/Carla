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

#include "CarlaNative.hpp"

#include "zynaddsubfx/Effects/Alienwah.h"
#include "zynaddsubfx/Effects/Chorus.h"
#include "zynaddsubfx/Effects/Distorsion.h"
#include "zynaddsubfx/Effects/DynamicFilter.h"
#include "zynaddsubfx/Effects/Echo.h"
#include "zynaddsubfx/Effects/Phaser.h"
#include "zynaddsubfx/Effects/Reverb.h"

#include "juce_audio_basics.h"

using juce::FloatVectorOperations;

// -----------------------------------------------------------------------

class FxAbstractPlugin : public PluginClass
{
protected:
    FxAbstractPlugin(const HostDescriptor* const host, const uint32_t paramCount, const uint32_t programCount)
        : PluginClass(host),
          fParamCount(paramCount-2), // volume and pan handled by host
          fProgramCount(programCount),
          fEffect(nullptr),
          efxoutl(nullptr),
          efxoutr(nullptr),
          fFirstInit(true)
    {
        const uint32_t bufferSize(getBufferSize());
        efxoutl = new float[bufferSize];
        efxoutr = new float[bufferSize];
        FloatVectorOperations::clear(efxoutl, bufferSize);
        FloatVectorOperations::clear(efxoutr, bufferSize);
    }

    ~FxAbstractPlugin() override
    {
        if (efxoutl != nullptr)
        {
            delete[] efxoutl;
            efxoutl = nullptr;
        }

        if (efxoutr != nullptr)
        {
            delete[] efxoutr;
            efxoutr = nullptr;
        }

        if (fEffect != nullptr)
        {
            delete fEffect;
            fEffect = nullptr;
        }
    }

    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const final
    {
        return fParamCount;
    }

    float getParameterValue(const uint32_t index) const final
    {
        return fEffect->getpar(index+2);
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    uint32_t getMidiProgramCount() const final
    {
        return fProgramCount;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) final
    {
        fFirstInit = false;
        fEffect->changepar(index+2, value);
    }

    void setMidiProgram(const uint8_t, const uint32_t, const uint32_t program) final
    {
        fFirstInit = false;
        fEffect->setpreset(program);

        const float volume(float(fEffect->getpar(0))/127.0f);
        hostSetVolume(volume);

        const unsigned char pan(fEffect->getpar(1));
        const float panning(float(pan)/63.5f-1.0f);
        hostSetPanning(panning);

        fEffect->changepar(0, 127);
        fEffect->changepar(1, 64);
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void activate() final
    {
        fEffect->cleanup();

        if (fFirstInit)
        {
            fFirstInit = false;
            hostSetDryWet(0.5f);
        }
    }

    void process(float** const inBuffer, float** const outBuffer, const uint32_t frames, const MidiEvent* const, const uint32_t) final
    {
        fEffect->out(Stereo<float*>(inBuffer[0], inBuffer[1]));

        FloatVectorOperations::copy(outBuffer[0], efxoutl, frames);
        FloatVectorOperations::copy(outBuffer[1], efxoutr, frames);
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher

    void bufferSizeChanged(const uint32_t bufferSize) final
    {
        delete[] efxoutl;
        delete[] efxoutr;
        efxoutl = new float[bufferSize];
        efxoutr = new float[bufferSize];
        FloatVectorOperations::clear(efxoutl, bufferSize);
        FloatVectorOperations::clear(efxoutr, bufferSize);

        doReinit(bufferSize, getSampleRate());
    }

    void sampleRateChanged(const double sampleRate) final
    {
        doReinit(getBufferSize(), sampleRate);
    }

    void doReinit(const uint32_t bufferSize, const double sampleRate)
    {
        float params[fParamCount];

        for (uint32_t i=0; i < fParamCount; ++i)
            params[i] = fEffect->getpar(i+2);

        reinit(bufferSize, sampleRate);

        for (uint32_t i=0; i < fParamCount; ++i)
            fEffect->changepar(i+2, params[i]);
    }

    virtual void reinit(const uint32_t bufferSize, const double sampleRate) = 0;

    // -------------------------------------------------------------------

    const uint32_t fParamCount;
    const uint32_t fProgramCount;

    Effect* fEffect;
    float*  efxoutl;
    float*  efxoutr;
    bool    fFirstInit;
};

// -----------------------------------------------------------------------

class FxAlienWahPlugin : public FxAbstractPlugin
{
public:
    FxAlienWahPlugin(const HostDescriptor* const host)
        : FxAbstractPlugin(host, 11, 4)
    {
        fEffect = new Alienwah(false, efxoutl, efxoutr, getSampleRate(), getBufferSize());
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    const Parameter* getParameterInfo(const uint32_t index) const override
    {
        if (index >= fParamCount)
            return nullptr;

        static Parameter param;
        static ParameterScalePoint scalePoints[2];

        int hints = PARAMETER_IS_ENABLED|PARAMETER_IS_INTEGER;

        param.name = nullptr;
        param.unit = nullptr;
        param.ranges.def       = 1.0f;
        param.ranges.min       = 0.0f;
        param.ranges.max       = 127.0f;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 20.0f;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
        case 0:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Frequency";
            param.ranges.def = 70.0f;
            break;
        case 1:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Randomness";
            param.ranges.def = 0.0f;
            break;
        case 2:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN|PARAMETER_USES_SCALEPOINTS;
            param.name = "LFO Type";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            param.scalePointCount = 2;
            param.scalePoints     = scalePoints;
            scalePoints[0].label  = "Sine";
            scalePoints[1].label  = "Triangle";
            scalePoints[0].value  = 0.0f;
            scalePoints[1].value  = 1.0f;
            break;
        case 3:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Stereo";
            param.ranges.def = 62.0f;
            break;
        case 4:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Depth";
            param.ranges.def = 60.0f;
            break;
        case 5:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Feedback";
            param.ranges.def = 105.0f;
            break;
        case 6:
            param.name = "Delay";
            param.ranges.def = 25.0f;
            param.ranges.min = 1.0f;
            param.ranges.max = 100.0f;
            break;
        case 7:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "L/R Cross";
            param.ranges.def = 0.0f;
            break;
        case 8:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Phase";
            param.ranges.def = 64.0f;
            break;
        }

        param.hints = static_cast<ParameterHints>(hints);

        return &param;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    const MidiProgram* getMidiProgramInfo(const uint32_t index) const override
    {
        if (index >= fProgramCount)
            return nullptr;

        static MidiProgram midiProg;

        midiProg.bank    = 0;
        midiProg.program = index;

        switch (index)
        {
        case 0:
            midiProg.name = "AlienWah1";
            break;
        case 1:
            midiProg.name = "AlienWah2";
            break;
        case 2:
            midiProg.name = "AlienWah3";
            break;
        case 3:
            midiProg.name = "AlienWah4";
            break;
        default:
            midiProg.name = nullptr;
            break;
        }

        return &midiProg;
    }

    // -------------------------------------------------------------------

    void reinit(const uint32_t bufferSize, const double sampleRate)
    {
        delete fEffect;
        fEffect = new Alienwah(false, efxoutl, efxoutr, sampleRate, bufferSize);
    }

    PluginClassEND(FxAlienWahPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxAlienWahPlugin)
};

// -----------------------------------------------------------------------

class FxChorusPlugin : public FxAbstractPlugin
{
public:
    FxChorusPlugin(const HostDescriptor* const host)
        : FxAbstractPlugin(host, 12, 10)
    {
        fEffect = new Chorus(false, efxoutl, efxoutr, getSampleRate(), getBufferSize());
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    const Parameter* getParameterInfo(const uint32_t index) const override
    {
        if (index >= fParamCount)
            return nullptr;

        static Parameter param;
        static ParameterScalePoint scalePoints[2];

        int hints = PARAMETER_IS_ENABLED|PARAMETER_IS_INTEGER;

        param.name = nullptr;
        param.unit = nullptr;
        param.ranges.def       = 1.0f;
        param.ranges.min       = 0.0f;
        param.ranges.max       = 127.0f;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 20.0f;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
        case 0:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Frequency";
            param.ranges.def = 50.0f;
            break;
        case 1:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Randomness";
            param.ranges.def = 0.0f;
            break;
        case 2:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN|PARAMETER_USES_SCALEPOINTS;
            param.name = "LFO Type";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            param.scalePointCount = 2;
            param.scalePoints     = scalePoints;
            scalePoints[0].label  = "Sine";
            scalePoints[1].label  = "Triangle";
            scalePoints[0].value  = 0.0f;
            scalePoints[1].value  = 1.0f;
            break;
        case 3:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Stereo";
            param.ranges.def = 90.0f;
            break;
        case 4:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Depth";
            param.ranges.def = 40.0f;
            break;
        case 5:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Delay";
            param.ranges.def = 85.0f;
            break;
        case 6:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Feedback";
            param.ranges.def = 64.0f;
            break;
        case 7:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "L/R Cross";
            param.ranges.def = 119.0f;
            break;
        case 8:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN;
            param.name = "Flange Mode";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            break;
        case 9:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN;
            param.name = "Subtract Output";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            break;
        }

        param.hints = static_cast<ParameterHints>(hints);

        return &param;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    const MidiProgram* getMidiProgramInfo(const uint32_t index) const override
    {
        if (index >= fProgramCount)
            return nullptr;

        static MidiProgram midiProg;

        midiProg.bank    = 0;
        midiProg.program = index;

        switch (index)
        {
        case 0:
            midiProg.name = "Chorus1";
            break;
        case 1:
            midiProg.name = "Chorus2";
            break;
        case 2:
            midiProg.name = "Chorus3";
            break;
        case 3:
            midiProg.name = "Celeste1";
            break;
        case 4:
            midiProg.name = "Celeste2";
            break;
        case 5:
            midiProg.name = "Flange1";
            break;
        case 6:
            midiProg.name = "Flange2";
            break;
        case 7:
            midiProg.name = "Flange3";
            break;
        case 8:
            midiProg.name = "Flange4";
            break;
        case 9:
            midiProg.name = "Flange5";
            break;
        default:
            midiProg.name = nullptr;
            break;
        }

        return &midiProg;
    }

    // -------------------------------------------------------------------

    void reinit(const uint32_t bufferSize, const double sampleRate)
    {
        delete fEffect;
        fEffect = new Chorus(false, efxoutl, efxoutr, sampleRate, bufferSize);
    }

    PluginClassEND(FxChorusPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxChorusPlugin)
};

// -----------------------------------------------------------------------

class FxDistortionPlugin : public FxAbstractPlugin
{
public:
    FxDistortionPlugin(const HostDescriptor* const host)
        : FxAbstractPlugin(host, 11, 6)
    {
        fEffect = new Distorsion(false, efxoutl, efxoutr, getSampleRate(), getBufferSize());
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    const Parameter* getParameterInfo(const uint32_t index) const override
    {
        if (index >= fParamCount)
            return nullptr;

        static Parameter param;
        static ParameterScalePoint scalePoints[14];

        int hints = PARAMETER_IS_ENABLED|PARAMETER_IS_INTEGER;

        param.name = nullptr;
        param.unit = nullptr;
        param.ranges.def       = 1.0f;
        param.ranges.min       = 0.0f;
        param.ranges.max       = 127.0f;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 20.0f;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
        case 0:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "L/R Cross";
            param.ranges.def = 35.0f;
            break;
        case 1:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Drive";
            param.ranges.def = 56.0f;
            break;
        case 2:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Level";
            param.ranges.def = 70.0f;
            break;
        case 3:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_USES_SCALEPOINTS;
            param.name = "Type";
            param.ranges.def = 0.0f;
            param.ranges.max = 13.0f;
            param.scalePointCount = 14;
            param.scalePoints     = scalePoints;
            scalePoints[ 0].label = "Arctangent";
            scalePoints[ 1].label = "Asymmetric";
            scalePoints[ 2].label = "Pow";
            scalePoints[ 3].label = "Sine";
            scalePoints[ 4].label = "Quantisize";
            scalePoints[ 5].label = "Zigzag";
            scalePoints[ 6].label = "Limiter";
            scalePoints[ 7].label = "Upper Limiter";
            scalePoints[ 8].label = "Lower Limiter";
            scalePoints[ 9].label = "Inverse Limiter";
            scalePoints[10].label = "Clip";
            scalePoints[11].label = "Asym2";
            scalePoints[12].label = "Pow2";
            scalePoints[13].label = "Sigmoid";
            scalePoints[ 0].value = 0.0f;
            scalePoints[ 1].value = 1.0f;
            scalePoints[ 2].value = 2.0f;
            scalePoints[ 3].value = 3.0f;
            scalePoints[ 4].value = 4.0f;
            scalePoints[ 5].value = 5.0f;
            scalePoints[ 6].value = 6.0f;
            scalePoints[ 7].value = 7.0f;
            scalePoints[ 8].value = 8.0f;
            scalePoints[ 9].value = 9.0f;
            scalePoints[10].value = 10.0f;
            scalePoints[11].value = 11.0f;
            scalePoints[12].value = 12.0f;
            scalePoints[13].value = 13.0f;
            break;
        case 4:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN;
            param.name = "Negate";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            break;
        case 5:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Low-Pass Filter";
            param.ranges.def = 96.0f;
            break;
        case 6:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "High-Pass Filter";
            param.ranges.def = 0.0f;
            break;
        case 7:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN;
            param.name = "Stereo";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            break;
        case 8:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN;
            param.name = "Pre-Filtering";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            break;
        }

        param.hints = static_cast<ParameterHints>(hints);

        return &param;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    const MidiProgram* getMidiProgramInfo(const uint32_t index) const override
    {
        if (index >= fProgramCount)
            return nullptr;

        static MidiProgram midiProg;

        midiProg.bank    = 0;
        midiProg.program = index;

        switch (index)
        {
        case 0:
            midiProg.name = "Overdrive 1";
            break;
        case 1:
            midiProg.name = "Overdrive 2";
            break;
        case 2:
            midiProg.name = "A. Exciter 1";
            break;
        case 3:
            midiProg.name = "A. Exciter 2";
            break;
        case 4:
            midiProg.name = "Guitar Amp";
            break;
        case 5:
            midiProg.name = "Quantisize";
            break;
        default:
            midiProg.name = nullptr;
            break;
        }

        return &midiProg;
    }

    // -------------------------------------------------------------------

    void reinit(const uint32_t bufferSize, const double sampleRate)
    {
        delete fEffect;
        fEffect = new Distorsion(false, efxoutl, efxoutr, sampleRate, bufferSize);
    }

    PluginClassEND(FxDistortionPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxDistortionPlugin)
};

// -----------------------------------------------------------------------

class FxDynamicFilterPlugin : public FxAbstractPlugin
{
public:
    FxDynamicFilterPlugin(const HostDescriptor* const host)
        : FxAbstractPlugin(host, 10, 5)
    {
        fEffect = new DynamicFilter(false, efxoutl, efxoutr, getSampleRate(), getBufferSize());
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    const Parameter* getParameterInfo(const uint32_t index) const override
    {
        if (index >= fParamCount)
            return nullptr;

        static Parameter param;
        static ParameterScalePoint scalePoints[2];

        int hints = PARAMETER_IS_ENABLED|PARAMETER_IS_INTEGER;

        param.name = nullptr;
        param.unit = nullptr;
        param.ranges.def       = 1.0f;
        param.ranges.min       = 0.0f;
        param.ranges.max       = 127.0f;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 20.0f;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
        case 0:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Frequency";
            param.ranges.def = 80.0f;
            break;
        case 1:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Randomness";
            param.ranges.def = 0.0f;
            break;
        case 2:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN|PARAMETER_USES_SCALEPOINTS;
            param.name = "LFO Type";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            param.scalePointCount = 2;
            param.scalePoints     = scalePoints;
            scalePoints[0].label  = "Sine";
            scalePoints[1].label  = "Triangle";
            scalePoints[0].value  = 0.0f;
            scalePoints[1].value  = 1.0f;
            break;
        case 3:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Stereo";
            param.ranges.def = 64.0f;
            break;
        case 4:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Depth";
            param.ranges.def = 0.0f;
            break;
        case 5:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Amp sns";
            param.ranges.def = 90.0f;
            break;
        case 6:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN;
            param.name = "Amp sns inv";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            break;
        case 7:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Amp Smooth";
            param.ranges.def = 60.0f;
            break;
        }

        param.hints = static_cast<ParameterHints>(hints);

        return &param;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    const MidiProgram* getMidiProgramInfo(const uint32_t index) const override
    {
        if (index >= fProgramCount)
            return nullptr;

        static MidiProgram midiProg;

        midiProg.bank    = 0;
        midiProg.program = index;

        switch (index)
        {
        case 0:
            midiProg.name = "WahWah";
            break;
        case 1:
            midiProg.name = "AutoWah";
            break;
        case 2:
            midiProg.name = "Sweep";
            break;
        case 3:
            midiProg.name = "VocalMorph1";
            break;
        case 4:
            midiProg.name = "VocalMorph2";
            break;
        default:
            midiProg.name = nullptr;
            break;
        }

        return &midiProg;
    }

    // -------------------------------------------------------------------

    void reinit(const uint32_t bufferSize, const double sampleRate)
    {
        delete fEffect;
        fEffect = new DynamicFilter(false, efxoutl, efxoutr, sampleRate, bufferSize);
    }

    PluginClassEND(FxDynamicFilterPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxDynamicFilterPlugin)
};

// -----------------------------------------------------------------------

class FxEchoPlugin : public FxAbstractPlugin
{
public:
    FxEchoPlugin(const HostDescriptor* const host)
        : FxAbstractPlugin(host, 7, 9)
    {
        fEffect = new Echo(false, efxoutl, efxoutr, getSampleRate(), getBufferSize());
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    const Parameter* getParameterInfo(const uint32_t index) const override
    {
        if (index >= fParamCount)
            return nullptr;

        static Parameter param;

        int hints = PARAMETER_IS_ENABLED|PARAMETER_IS_INTEGER;

        param.name = nullptr;
        param.unit = nullptr;
        param.ranges.def       = 1.0f;
        param.ranges.min       = 0.0f;
        param.ranges.max       = 127.0f;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 20.0f;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
        case 0:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Delay";
            param.ranges.def = 35.0f;
            break;
        case 1:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "L/R Delay";
            param.ranges.def = 64.0f;
            break;
        case 2:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "L/R Cross";
            param.ranges.def = 30.0f;
            break;
        case 3:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Feedback";
            param.ranges.def = 59.0f;
            break;
        case 4:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "High Damp";
            param.ranges.def = 0.0f;
            break;
        }

        param.hints = static_cast<ParameterHints>(hints);

        return &param;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    const MidiProgram* getMidiProgramInfo(const uint32_t index) const override
    {
        if (index >= fProgramCount)
            return nullptr;

        static MidiProgram midiProg;

        midiProg.bank    = 0;
        midiProg.program = index;

        switch (index)
        {
        case 0:
            midiProg.name = "Echo 1";
            break;
        case 1:
            midiProg.name = "Echo 2";
            break;
        case 2:
            midiProg.name = "Echo 3";
            break;
        case 3:
            midiProg.name = "Simple Echo";
            break;
        case 4:
            midiProg.name = "Canyon";
            break;
        case 5:
            midiProg.name = "Panning Echo 1";
            break;
        case 6:
            midiProg.name = "Panning Echo 2";
            break;
        case 7:
            midiProg.name = "Panning Echo 3";
            break;
        case 8:
            midiProg.name = "Feedback Echo";
            break;
        default:
            midiProg.name = nullptr;
            break;
        }

        return &midiProg;
    }

    // -------------------------------------------------------------------

    void reinit(const uint32_t bufferSize, const double sampleRate)
    {
        delete fEffect;
        fEffect = new Echo(false, efxoutl, efxoutr, sampleRate, bufferSize);
    }

    PluginClassEND(FxEchoPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxEchoPlugin)
};

// -----------------------------------------------------------------------

class FxPhaserPlugin : public FxAbstractPlugin
{
public:
    FxPhaserPlugin(const HostDescriptor* const host)
        : FxAbstractPlugin(host, 15, 12)
    {
        fEffect = new Phaser(false, efxoutl, efxoutr, getSampleRate(), getBufferSize());
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    const Parameter* getParameterInfo(const uint32_t index) const override
    {
        if (index >= fParamCount)
            return nullptr;

        static Parameter param;
        static ParameterScalePoint scalePoints[2];

        int hints = PARAMETER_IS_ENABLED|PARAMETER_IS_INTEGER;

        param.name = nullptr;
        param.unit = nullptr;
        param.ranges.def       = 1.0f;
        param.ranges.min       = 0.0f;
        param.ranges.max       = 127.0f;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 20.0f;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
        case 0:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Frequency";
            param.ranges.def = 36.0f;
            break;
        case 1:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Randomness";
            param.ranges.def = 0.0f;
            break;
        case 2:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN|PARAMETER_USES_SCALEPOINTS;
            param.name = "LFO Type";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            param.scalePointCount = 2;
            param.scalePoints     = scalePoints;
            scalePoints[0].label  = "Sine";
            scalePoints[1].label  = "Triangle";
            scalePoints[0].value  = 0.0f;
            scalePoints[1].value  = 1.0f;
            break;
        case 3:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "LFO Stereo";
            param.ranges.def = 64.0f;
            break;
        case 4:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Depth";
            param.ranges.def = 110.0f;
            break;
        case 5:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Feedback";
            param.ranges.def = 64.0f;
            break;
        case 6:
            param.name = "Stages";
            param.ranges.def = 1.0f;
            param.ranges.min = 1.0f;
            param.ranges.max = 12.0f;
            break;
        case 7:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "L/R Cross|Offset";
            param.ranges.def = 0.0f;
            break;
        case 8:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN;
            param.name = "Subtract Output";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            break;
        case 9:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Phase|Width";
            param.ranges.def = 20.0f;
            break;
        case 10:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN;
            param.name = "Hyper";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            break;
        case 11:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Distortion";
            param.ranges.def = 0.0f;
            break;
        case 12:
            hints |= PARAMETER_IS_AUTOMABLE|PARAMETER_IS_BOOLEAN;
            param.name = "Analog";
            param.ranges.def = 0.0f;
            param.ranges.max = 1.0f;
            break;
        }

        param.hints = static_cast<ParameterHints>(hints);

        return &param;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    const MidiProgram* getMidiProgramInfo(const uint32_t index) const override
    {
        if (index >= fProgramCount)
            return nullptr;

        static MidiProgram midiProg;

        midiProg.bank    = 0;
        midiProg.program = index;

        switch (index)
        {
        case 0:
            midiProg.name = "Phaser 1";
            break;
        case 1:
            midiProg.name = "Phaser 2";
            break;
        case 2:
            midiProg.name = "Phaser 3";
            break;
        case 3:
            midiProg.name = "Phaser 4";
            break;
        case 4:
            midiProg.name = "Phaser 5";
            break;
        case 5:
            midiProg.name = "Phaser 6";
            break;
        case 6:
            midiProg.name = "APhaser 1";
            break;
        case 7:
            midiProg.name = "APhaser 2";
            break;
        case 8:
            midiProg.name = "APhaser 3";
            break;
        case 9:
            midiProg.name = "APhaser 4";
            break;
        case 10:
            midiProg.name = "APhaser 5";
            break;
        case 11:
            midiProg.name = "APhaser 6";
            break;
        default:
            midiProg.name = nullptr;
            break;
        }

        return &midiProg;
    }

    // -------------------------------------------------------------------

    void reinit(const uint32_t bufferSize, const double sampleRate)
    {
        delete fEffect;
        fEffect = new Phaser(false, efxoutl, efxoutr, sampleRate, bufferSize);
    }

    PluginClassEND(FxPhaserPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxPhaserPlugin)
};

// -----------------------------------------------------------------------

class FxReverbPlugin : public FxAbstractPlugin
{
public:
    FxReverbPlugin(const HostDescriptor* const host)
        : FxAbstractPlugin(host, 13, 13)
    {
        fEffect = new Reverb(false, efxoutl, efxoutr, getSampleRate(), getBufferSize());
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    const Parameter* getParameterInfo(const uint32_t index) const override
    {
        if (index >= fParamCount)
            return nullptr;

        static Parameter param;
        static ParameterScalePoint scalePoints[3];

        int hints = PARAMETER_IS_ENABLED | PARAMETER_IS_INTEGER;

        param.name = nullptr;
        param.unit = nullptr;
        param.ranges.def       = 1.0f;
        param.ranges.min       = 0.0f;
        param.ranges.max       = 127.0f;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 20.0f;
        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;

        switch (index)
        {
        case 0:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Time";
            param.ranges.def = 63.0f;
            break;
        case 1:
            param.name = "Delay";
            param.ranges.def = 24.0f;
            break;
        case 2:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Feedback";
            param.ranges.def = 0.0f;
            break;
        case 3:
            hints = 0x0;
            param.name = "bw";
            break;
        case 4:
            hints = 0x0;
            param.name = "E/R";
            break;
        case 5:
            param.name = "Low-Pass Filter";
            param.ranges.def = 85.0f;
            break;
        case 6:
            param.name = "High-Pass Filter";
            param.ranges.def = 5.0f;
            break;
        case 7:
            hints |= PARAMETER_IS_AUTOMABLE;
            param.name = "Damp";
            param.ranges.def = 83.0f;
            param.ranges.min = 64.0f;
            break;
        case 8:
            hints |= PARAMETER_USES_SCALEPOINTS;
            param.name = "Type";
            param.ranges.def = 1.0f;
            param.ranges.max = 2.0f;
            param.scalePointCount = 3;
            param.scalePoints     = scalePoints;
            scalePoints[0].label  = "Random";
            scalePoints[1].label  = "Freeverb";
            scalePoints[2].label  = "Bandwidth";
            scalePoints[0].value  = 0.0f;
            scalePoints[1].value  = 1.0f;
            scalePoints[2].value  = 2.0f;
            break;
        case 9:
            param.name = "Room size";
            param.ranges.def = 64.0f;
            param.ranges.min = 1.0f;
            break;
        case 10:
            param.name = "Bandwidth";
            param.ranges.def = 20.0f;
            break;
        }

        param.hints = static_cast<ParameterHints>(hints);

        return &param;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    const MidiProgram* getMidiProgramInfo(const uint32_t index) const override
    {
        if (index >= fProgramCount)
            return nullptr;

        static MidiProgram midiProg;

        midiProg.bank    = 0;
        midiProg.program = index;

        switch (index)
        {
        case 0:
            midiProg.name = "Cathedral1";
            break;
        case 1:
            midiProg.name = "Cathedral2";
            break;
        case 2:
            midiProg.name = "Cathedral3";
            break;
        case 3:
            midiProg.name = "Hall1";
            break;
        case 4:
            midiProg.name = "Hall2";
            break;
        case 5:
            midiProg.name = "Room1";
            break;
        case 6:
            midiProg.name = "Room2";
            break;
        case 7:
            midiProg.name = "Basement";
            break;
        case 8:
            midiProg.name = "Tunnel";
            break;
        case 9:
            midiProg.name = "Echoed1";
            break;
        case 10:
            midiProg.name = "Echoed2";
            break;
        case 11:
            midiProg.name = "VeryLong1";
            break;
        case 12:
            midiProg.name = "VeryLong2";
            break;
        default:
            midiProg.name = nullptr;
            break;
        }

        return &midiProg;
    }

    // -------------------------------------------------------------------

    void reinit(const uint32_t bufferSize, const double sampleRate)
    {
        delete fEffect;
        fEffect = new Reverb(false, efxoutl, efxoutr, sampleRate, bufferSize);
    }

    PluginClassEND(FxReverbPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxReverbPlugin)
};

// -----------------------------------------------------------------------

static const PluginDescriptor fxAlienWahDesc = {
    /* category  */ PLUGIN_CATEGORY_MODULATOR,
    /* hints     */ static_cast<PluginHints>(PLUGIN_IS_RTSAFE|PLUGIN_USES_PANNING|PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ static_cast<PluginSupports>(0x0),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 11-2,
    /* paramOuts */ 0,
    /* name      */ "ZynAlienWah",
    /* label     */ "zynAlienWah",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(FxAlienWahPlugin)
};

static const PluginDescriptor fxChorusDesc = {
    /* category  */ PLUGIN_CATEGORY_MODULATOR,
    /* hints     */ static_cast<PluginHints>(PLUGIN_IS_RTSAFE|PLUGIN_USES_PANNING|PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ static_cast<PluginSupports>(0x0),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 12-2,
    /* paramOuts */ 0,
    /* name      */ "ZynChorus",
    /* label     */ "zynChorus",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(FxChorusPlugin)
};

static const PluginDescriptor fxDistortionDesc = {
    /* category  */ PLUGIN_CATEGORY_MODULATOR,
    /* hints     */ static_cast<PluginHints>(PLUGIN_USES_PANNING|PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ static_cast<PluginSupports>(0x0),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 11-2,
    /* paramOuts */ 0,
    /* name      */ "ZynDistortion",
    /* label     */ "zynDistortion",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(FxDistortionPlugin)
};

static const PluginDescriptor fxDynamicFilterDesc = {
    /* category  */ PLUGIN_CATEGORY_FILTER,
    /* hints     */ static_cast<PluginHints>(PLUGIN_USES_PANNING|PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ static_cast<PluginSupports>(0x0),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 10-2,
    /* paramOuts */ 0,
    /* name      */ "ZynDynamicFilter",
    /* label     */ "zynDynamicFilter",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(FxDynamicFilterPlugin)
};

static const PluginDescriptor fxEchoDesc = {
    /* category  */ PLUGIN_CATEGORY_DELAY,
    /* hints     */ static_cast<PluginHints>(PLUGIN_IS_RTSAFE|PLUGIN_USES_PANNING|PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ static_cast<PluginSupports>(0x0),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 7-2,
    /* paramOuts */ 0,
    /* name      */ "ZynEcho",
    /* label     */ "zynEcho",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(FxEchoPlugin)
};

static const PluginDescriptor fxPhaserDesc = {
    /* category  */ PLUGIN_CATEGORY_MODULATOR,
    /* hints     */ static_cast<PluginHints>(PLUGIN_USES_PANNING|PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ static_cast<PluginSupports>(0x0),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 15-2,
    /* paramOuts */ 0,
    /* name      */ "ZynPhaser",
    /* label     */ "zynPhaser",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(FxPhaserPlugin)
};

static const PluginDescriptor fxReverbDesc = {
    /* category  */ PLUGIN_CATEGORY_DELAY,
    /* hints     */ static_cast<PluginHints>(PLUGIN_USES_PANNING|PLUGIN_NEEDS_FIXED_BUFFERS),
    /* supports  */ static_cast<PluginSupports>(0x0),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 13-2,
    /* paramOuts */ 0,
    /* name      */ "ZynReverb",
    /* label     */ "zynReverb",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(FxReverbPlugin)
};

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_zynaddsubfx_fx()
{
    carla_register_native_plugin(&fxAlienWahDesc);
    carla_register_native_plugin(&fxChorusDesc);
    carla_register_native_plugin(&fxDistortionDesc);
    carla_register_native_plugin(&fxDynamicFilterDesc);
    carla_register_native_plugin(&fxEchoDesc);
    carla_register_native_plugin(&fxPhaserDesc);
    carla_register_native_plugin(&fxReverbDesc);
}

// -----------------------------------------------------------------------
