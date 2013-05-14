/*
 * Carla Native Plugins
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "CarlaNative.hpp"
#include "CarlaString.hpp"

#include "audio-base.hpp"

#define PROGRAM_COUNT 16

class AudioFilePlugin : public PluginDescriptorClass,
                        public AbstractAudioPlayer
{
public:
    AudioFilePlugin(const HostDescriptor* const host)
        : PluginDescriptorClass(host),
          AbstractAudioPlayer(),
          fLoopMode(false),
          fDoProcess(false),
          fLastFrame(0),
          fMaxFrame(0),
          fThread(this, getSampleRate())
    {
        fPool.create(getSampleRate());
    }

    ~AudioFilePlugin() override
    {
        fPool.destroy();
        fThread.stopNow();
    }

    uint32_t getLastFrame() const override
    {
        return fLastFrame;
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() override
    {
        return 0; // TODO - loopMode
    }

    const Parameter* getParameterInfo(const uint32_t index) override
    {
        if (index != 0)
            return nullptr;

        static Parameter param;

        param.name  = "Loop Mode";
        param.unit  = nullptr;
        param.hints = static_cast<ParameterHints>(PARAMETER_IS_ENABLED|PARAMETER_IS_BOOLEAN);
        param.ranges.def = 1.0f;
        param.ranges.min = 0.0f;
        param.ranges.max = 1.0f;
        param.ranges.step = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 1.0f;
        param.scalePointCount = 0;
        param.scalePoints     = nullptr;

        return &param;
    }

    float getParameterValue(const uint32_t index) override
    {
        if (index != 0)
            return 0.0f;

        return fLoopMode ? 1.0f : 0.0f;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    uint32_t getMidiProgramCount() override
    {
        return PROGRAM_COUNT;
    }

    const MidiProgram* getMidiProgramInfo(const uint32_t index) override
    {
        if (index >= PROGRAM_COUNT)
            return NULL;

        static MidiProgram midiProgram;

        midiProgram.bank    = 0;
        midiProgram.program = index;
        midiProgram.name    = (const char*)fPrograms.shortNames[index];

        return &midiProgram;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        if (index != 0)
            return;

        bool b = (value > 0.5f);

        if (b == fLoopMode)
            return;

        fLoopMode = b;
        fThread.setNeedsRead();
    }

    void setMidiProgram(const uint8_t, const uint32_t bank, const uint32_t program) override
    {
        if (bank != 0 || program >= PROGRAM_COUNT)
            return;

        if (fPrograms.current != program)
        {
            loadFilename(fPrograms.fullNames[program]);
            fPrograms.current = program;
        }
    }

    void setCustomData(const char* const key, const char* const value) override
    {
        if (std::strlen(key) != 6)
            return;
        if (std::strncmp(key, "file", 4) != 0)
            return;
        if (key[4] < '0' || key[4] > '9')
            return;
        if (key[5] < '0' || key[5] > '9')
            return;

        uint8_t tens = key[4]-'0';
        uint8_t nums = key[5]-'0';

        uint32_t program = tens*10 + nums;

        if (program >= PROGRAM_COUNT)
            return;

        fPrograms.fullNames[program] = value;

        if (const char* const shortName = std::strrchr(value, OS_SEP))
            fPrograms.shortNames[program] = shortName+1;
        else
            fPrograms.shortNames[program] = value;

        fPrograms.shortNames[program].truncate(fPrograms.shortNames[program].rfind('.'));

        if (fPrograms.current == program)
            loadFilename(value);
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float**, float** const outBuffer, const uint32_t frames, const uint32_t, const MidiEvent* const) override
    {
        const TimeInfo* const timePos(getTimeInfo());

        float* out1 = outBuffer[0];
        float* out2 = outBuffer[1];

        if (! fDoProcess)
        {
            carla_stderr("P: no process");
            fLastFrame = timePos->frame;
            carla_zeroFloat(out1, frames);
            carla_zeroFloat(out2, frames);
            return;
        }

        // not playing
        if (! timePos->playing)
        {
            carla_stderr("P: not playing");
            fLastFrame = timePos->frame;

            if (timePos->frame == 0 && fLastFrame > 0)
                fThread.setNeedsRead();

            carla_zeroFloat(out1, frames);
            carla_zeroFloat(out2, frames);
            return;
        }

        fThread.tryPutData(fPool);

        // out of reach
        if (timePos->frame + frames < fPool.startFrame || timePos->frame >= fMaxFrame) /*&& ! loopMode)*/
        {
            carla_stderr("P: out of reach");
            fLastFrame = timePos->frame;

            fThread.setNeedsRead();

            carla_zeroFloat(out1, frames);
            carla_zeroFloat(out2, frames);
            return;
        }

        int64_t poolFrame = (int64_t)timePos->frame - fPool.startFrame;
        int64_t poolSize  = fPool.size;

        for (uint32_t i=0; i < frames; ++i, ++poolFrame)
        {
            if (poolFrame >= 0 && poolFrame < poolSize)
            {
                out1[i] = fPool.buffer[0][poolFrame];
                out2[i] = fPool.buffer[1][poolFrame];

                // reset
                fPool.buffer[0][poolFrame] = 0.0f;
                fPool.buffer[1][poolFrame] = 0.0f;
            }
            else
            {
                out1[i] = 0.0f;
                out2[i] = 0.0f;
            }
        }

        fLastFrame = timePos->frame;
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        if (! show)
            return;

        if (const char* const filename = uiOpenFile(false, "Open Audio File", ""))
        {
            char fileStr[] = { 'f', 'i', 'l', 'e', '\0', '\0', '\0' };
            fileStr[4] = '0' + (fPrograms.current / 10);
            fileStr[5] = '0' + (fPrograms.current % 10);

            uiCustomDataChanged(fileStr, filename);
        }

        uiClosed();
    }

private:
    bool fLoopMode;
    bool fDoProcess;

    uint32_t fLastFrame;
    uint32_t fMaxFrame;

    AudioFilePool   fPool;
    AudioFileThread fThread;

    struct Programs {
        uint32_t    current;
        CarlaString fullNames[PROGRAM_COUNT];
        CarlaString shortNames[PROGRAM_COUNT];

        Programs()
            : current(0) {}
    } fPrograms;

    void loadFilename(const char* const filename)
    {
        CARLA_ASSERT(filename != nullptr);
        carla_debug("AudioFilePlugin::loadFilename(\"%s\")", filename);

        if (filename == nullptr)
            return;

        fThread.stopNow();

        if (fThread.loadFilename(filename))
        {
            carla_stdout("AudioFilePlugin::loadFilename(\"%s\") - sucess", filename);
            fThread.startNow();
            fMaxFrame = fThread.getMaxFrame();
            fDoProcess = true;
        }
        else
        {
            carla_stderr("AudioFilePlugin::loadFilename(\"%s\") - failed", filename);
            fDoProcess = false;
            fMaxFrame = 0;
        }
    }

    PluginDescriptorClassEND(AudioFilePlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFilePlugin)
};

// -----------------------------------------------------------------------

static const PluginDescriptor audiofileDesc = {
    /* category  */ PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<PluginHints>(PLUGIN_IS_RTSAFE|PLUGIN_HAS_GUI),
    /* audioIns  */ 0,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 0, // TODO - loopMode
    /* paramOuts */ 0,
    /* name      */ "Audio File",
    /* label     */ "audiofile",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(AudioFilePlugin)
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_audiofile()
{
    carla_register_native_plugin(&audiofileDesc);
}

// -----------------------------------------------------------------------
