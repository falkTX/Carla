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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaNative.hpp"
#include "CarlaString.hpp"

#include "audio-base.hpp"

#define PROGRAM_COUNT 16

class AudioFilePlugin : public PluginClass,
                        public AbstractAudioPlayer
{
public:
    AudioFilePlugin(const HostDescriptor* const host)
        : PluginClass(host),
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

    uint32_t getParameterCount() const override
    {
        return 0; // TODO - loopMode
    }

    const Parameter* getParameterInfo(const uint32_t index) const override
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

    float getParameterValue(const uint32_t index) const override
    {
        if (index != 0)
            return 0.0f;

        return fLoopMode ? 1.0f : 0.0f;
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

    void setCustomData(const char* const key, const char* const value) override
    {
        if (std::strcmp(key, "file") != 0)
            return;

        loadFilename(value);
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float**, float** const outBuffer, const uint32_t frames, const MidiEvent* const, const uint32_t) override
    {
        const TimeInfo* const timePos(getTimeInfo());

        float* out1 = outBuffer[0];
        float* out2 = outBuffer[1];

        if (! fDoProcess)
        {
            //carla_stderr("P: no process");
            fLastFrame = timePos->frame;
            carla_zeroFloat(out1, frames);
            carla_zeroFloat(out2, frames);
            return;
        }

        // not playing
        if (! timePos->playing)
        {
            //carla_stderr("P: not playing");
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
            //carla_stderr("P: out of reach");
            fLastFrame = timePos->frame;

            if (timePos->frame + frames < fPool.startFrame)
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
            uiCustomDataChanged("file", filename);

        uiClosed();
    }

private:
    bool fLoopMode;
    bool fDoProcess;

    uint32_t fLastFrame;
    uint32_t fMaxFrame;

    AudioFilePool   fPool;
    AudioFileThread fThread;

    void loadFilename(const char* const filename)
    {
        CARLA_ASSERT(filename != nullptr);
        carla_debug("AudioFilePlugin::loadFilename(\"%s\")", filename);

        fThread.stopNow();

        if (filename == nullptr || *filename == '\0')
        {
            fDoProcess = false;
            fMaxFrame = 0;
            return;
        }

        if (fThread.loadFilename(filename))
        {
            fThread.startNow();
            fMaxFrame = fThread.getMaxFrame();
            fDoProcess = true;
        }
        else
        {
            fDoProcess = false;
            fMaxFrame = 0;
        }
    }

    PluginClassEND(AudioFilePlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFilePlugin)
};

// -----------------------------------------------------------------------

static const PluginDescriptor audiofileDesc = {
    /* category  */ PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<PluginHints>(PLUGIN_IS_RTSAFE|PLUGIN_HAS_GUI|PLUGIN_NEEDS_OPENSAVE),
    /* supports  */ static_cast<PluginSupports>(0x0),
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

CARLA_EXPORT
void carla_register_native_plugin_audiofile()
{
    carla_register_native_plugin(&audiofileDesc);
}

// -----------------------------------------------------------------------
