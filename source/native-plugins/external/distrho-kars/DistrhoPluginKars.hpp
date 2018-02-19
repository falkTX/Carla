/*
 * DISTRHO Kars Plugin, based on karplong by Chris Cannam.
 * Copyright (C) 2015 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DISTRHO_PLUGIN_KARS_HPP_INCLUDED
#define DISTRHO_PLUGIN_KARS_HPP_INCLUDED

#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class DistrhoPluginKars : public Plugin
{
public:
    static const int      kMaxNotes = 128;
    static const uint32_t kNoteNull = (uint32_t)-1;

    enum Parameters
    {
        paramSustain = 0,
        paramCount
    };

    DistrhoPluginKars();

protected:
    // -------------------------------------------------------------------
    // Information

    const char* getLabel() const noexcept override
    {
        return "Kars";
    }

    const char* getDescription() const override
    {
        return "Simple karplus-strong plucked string synth.";
    }

    const char* getMaker() const noexcept override
    {
        return "falkTX";
    }

    const char* getHomePage() const override
    {
        return "https://github.com/DISTRHO/Kars";
    }

    const char* getLicense() const noexcept override
    {
        return "ISC";
    }

    uint32_t getVersion() const noexcept override
    {
        return d_version(1, 0, 0);
    }

    int64_t getUniqueId() const noexcept override
    {
        return d_cconst('D', 'K', 'r', 's');
    }

    // -------------------------------------------------------------------
    // Init

    void initParameter(uint32_t index, Parameter& parameter) override;

    // -------------------------------------------------------------------
    // Internal data

    float getParameterValue(uint32_t index) const override;
    void  setParameterValue(uint32_t index, float value) override;

    // -------------------------------------------------------------------
    // Process

    void activate() override;
    void run(const float**, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount) override;

    // -------------------------------------------------------------------

private:
    bool     fSustain;
    double   fSampleRate;
    uint32_t fBlockStart;

    struct Note {
        uint32_t on;
        uint32_t off;
        uint8_t  velocity;
        float   index;
        float   size;
        int     sizei;
        float*  wavetable;

        Note() noexcept
          : on(kNoteNull),
            off(kNoteNull),
            velocity(0),
            index(0.0f),
            size(0.0f),
            wavetable(nullptr) {}

        ~Note() noexcept
        {
            if (wavetable != nullptr)
            {
                delete[] wavetable;
                wavetable = nullptr;
            }
        }

        void setSampleRate(const double sampleRate)
        {
            if (wavetable != nullptr)
                delete[] wavetable;

            const float frequency = 440.0f * std::pow(2.0f, (index - 69.0f) / 12.0f);
            size  = sampleRate / frequency;
            sizei = int(size)+1;
            wavetable = new float[sizei];
            std::memset(wavetable, 0, sizeof(float)*static_cast<size_t>(sizei));
        }

    } fNotes[kMaxNotes];

    void addSamples(float* out, int voice, uint32_t offset, uint32_t count);

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DistrhoPluginKars)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif  // DISTRHO_PLUGIN_KARS_HPP_INCLUDED
