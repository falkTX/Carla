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
#include "CarlaMutex.hpp"
#include "CarlaString.hpp"

#include <QtCore/QThread>

extern "C" {
#include "audio_decoder/ad.h"
}

#define PROGRAM_COUNT 16

typedef struct adinfo ADInfo;

class AudioFilePlugin : public PluginDescriptorClass
{
public:
    AudioFilePlugin(const HostDescriptor* const host)
        : PluginDescriptorClass(host),
          filePtr(nullptr),
          lastFrame(0),
          maxFrame(0),
          loopMode(false),
          doProcess(false)
    {
        static bool adInitiated = false;

        if (! adInitiated)
        {
            ad_init();
            adInitiated = true;
        }

        ad_clear_nfo(&fileNfo);

        pool.create(getSampleRate());
        thread.setPoolSize(pool.size);
    }

    ~AudioFilePlugin() override
    {
        doProcess = false;

        thread.stopNow();

        if (filePtr != nullptr)
            ad_close(filePtr);

        pool.destroy();
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() override
    {
        return 0; // FIXME
        return 1;
    }

    const Parameter* getParameterInfo(const uint32_t index) override
    {
        return nullptr; // FIXME - loop mode needs work

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

        return loopMode ? 1.0f : 0.0f;
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
        midiProgram.name    = (const char*)programs.shortNames[index];

        return &midiProgram;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        if (index != 0)
            return;

        bool b = (value > 0.5f);

        if (b == loopMode)
            return;

        loopMode = b;
        thread.setNeedsRead();
    }

    void setMidiProgram(const uint8_t, const uint32_t bank, const uint32_t program) override
    {
        if (bank != 0 || program >= PROGRAM_COUNT)
            return;

        if (programs.current != program)
        {
            loadFilename(programs.fullNames[program]);
            programs.current = program;
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

        programs.fullNames[program] = value;

        if (const char* shortName = std::strrchr(value, OS_SEP))
            programs.shortNames[program] = shortName+1;
        else
            programs.shortNames[program] = value;

        programs.shortNames[program].truncate(programs.shortNames[program].rfind('.'));

        if (programs.current == program)
            loadFilename(value);
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float**, float** const outBuffer, const uint32_t frames, const uint32_t, const MidiEvent* const) override
    {
        const TimeInfo* const timePos(getTimeInfo());

        float* out1 = outBuffer[0];
        float* out2 = outBuffer[1];

        if (! doProcess)
        {
            lastFrame = timePos->frame;

            carla_zeroFloat(out1, frames);
            carla_zeroFloat(out2, frames);
            return;
        }

        // not playing
        if (! timePos->playing)
        {
            if (timePos->frame == 0 && lastFrame > 0)
                thread.setNeedsRead();

            lastFrame = timePos->frame;

            carla_zeroFloat(out1, frames);
            carla_zeroFloat(out2, frames);
            return;
        }

        const CarlaMutex::ScopedLocker sl(&mutex);

        // out of reach
        if (timePos->frame + frames < pool.startFrame || timePos->frame >= maxFrame) /*&& ! loopMode)*/
        {
            lastFrame = timePos->frame;
            thread.setNeedsRead();

            carla_zeroFloat(out1, frames);
            carla_zeroFloat(out2, frames);
            return;
        }

        int64_t poolFrame = (int64_t)timePos->frame - pool.startFrame;
        int64_t poolSize  = pool.size;

        for (uint32_t i=0; i < frames; ++i, ++poolFrame)
        {
            if (poolFrame >= 0 && poolFrame < poolSize)
            {
                out1[i] = pool.buffer[0][poolFrame];
                out2[i] = pool.buffer[1][poolFrame];

                // reset
                pool.buffer[0][poolFrame] = 0.0f;
                pool.buffer[1][poolFrame] = 0.0f;
            }
            else
            {
                out1[i] = 0.0f;
                out2[i] = 0.0f;
            }
        }

        lastFrame = timePos->frame;
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
            fileStr[4] = '0' + (programs.current / 10);
            fileStr[5] = '0' + (programs.current % 10);

            uiCustomDataChanged(fileStr, filename);
        }

        uiClosed();
    }

private:
    struct Pool {
        float*   buffer[2];
        uint32_t startFrame;
        uint32_t size;

        Pool()
            : buffer{nullptr},
              startFrame(0),
              size(0) {}

        ~Pool()
        {
            CARLA_ASSERT(buffer[0] == nullptr);
            CARLA_ASSERT(buffer[1] == nullptr);
            CARLA_ASSERT(startFrame == 0);
            CARLA_ASSERT(size == 0);
        }

        void create(const uint32_t sampleRate)
        {
            CARLA_ASSERT(buffer[0] == nullptr);
            CARLA_ASSERT(buffer[1] == nullptr);
            CARLA_ASSERT(startFrame == 0);
            CARLA_ASSERT(size == 0);

            size = sampleRate * 6;

            buffer[0] = new float[size];
            buffer[1] = new float[size];

            carla_zeroFloat(buffer[0], size);
            carla_zeroFloat(buffer[1], size);
        }

        void destroy()
        {
            CARLA_ASSERT(buffer[0] != nullptr);
            CARLA_ASSERT(buffer[1] != nullptr);
            CARLA_ASSERT(size != 0);

            if (buffer[0] != nullptr)
            {
                delete[] buffer[0];
                buffer[0] = nullptr;
            }

            if (buffer[1] != nullptr)
            {
                delete[] buffer[1];
                buffer[1] = nullptr;
            }

            startFrame = 0;
            size = 0;
        }
    };

    struct Programs {
        uint32_t    current;
        CarlaString fullNames[PROGRAM_COUNT];
        CarlaString shortNames[PROGRAM_COUNT];

        Programs()
            : current(0) {}
    };

    class AudioFileThread : public QThread
    {
    public:
        AudioFileThread()
            : QThread(nullptr),
              fNeedsRead(false),
              fQuitNow(true),
              fLastFrame(0),
              fPoolStartFrame(0),
              fPoolSize(0)
        {
        }

        ~AudioFileThread()
        {
            CARLA_ASSERT(fQuitNow == true);
            CARLA_ASSERT(! isRunning());
        }

        void stopNow()
        {
            fNeedsRead = false;
            fQuitNow = true;

            if (isRunning() && ! wait(1000))
                terminate();
        }

        void readPoll()
        {
        }

        void setNeedsRead()
        {
            fNeedsRead = true;
        }

        void setLastFrame(const uint32_t lastFrame)
        {
            fLastFrame = lastFrame;
        }

        void setPoolStart(const uint32_t poolStartFrame)
        {
            fPoolStartFrame = poolStartFrame;
        }

        void setPoolSize(const uint32_t  poolSize)
        {
            fPoolSize = poolSize;
        }

    protected:
        void run() override
        {
            while (! fQuitNow)
            {
                if (fNeedsRead || (fLastFrame >= fPoolStartFrame && fLastFrame - fPoolStartFrame >= fPoolSize*3/4))
                    readPoll();
                else
                    carla_msleep(50);
            }
        }

    private:
        bool fNeedsRead;
        bool fQuitNow;

        uint32_t fLastFrame;
        uint32_t fPoolStartFrame;
        uint32_t fPoolSize;
    };

    void*  filePtr;
    ADInfo fileNfo;

    uint32_t lastFrame;
    uint32_t maxFrame;

    bool loopMode;
    bool doProcess;

    Pool     pool;
    Programs programs;

    CarlaMutex      mutex;
    AudioFileThread thread;

    void loadFilename(const char* const filename)
    {
        // wait for jack processing to end
        doProcess = false;

        {
            const CarlaMutex::ScopedLocker sl(&mutex);

            maxFrame = 0;
            pool.startFrame = 0;

            thread.stopNow();
        }

        // clear old data
        if (filePtr != nullptr)
        {
            ad_close(filePtr);
            filePtr = nullptr;
        }

        ad_clear_nfo(&fileNfo);

        if (filename == nullptr)
            return;

        // open new
        filePtr = ad_open(filename, &fileNfo);

        if (filePtr == nullptr)
            return;

        ad_dump_nfo(99, &fileNfo);

        if (fileNfo.frames == 0)
            carla_stderr("L: filename \"%s\" has 0 frames", filename);

        if ((fileNfo.channels == 1 || fileNfo.channels == 2) && fileNfo.frames > 0)
        {
            maxFrame = fileNfo.frames;
            thread.readPoll();
            doProcess = true;

            thread.start();
        }
        else
        {
            ad_clear_nfo(&fileNfo);
            ad_close(filePtr);
            filePtr = nullptr;
        }
    }

    PluginDescriptorClassEND(AudioFilePlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFilePlugin)
};

// -----------------------------------------------------------------------

static const PluginDescriptor audiofileDesc = {
    /* category  */ PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<PluginHints>(PLUGIN_HAS_GUI),
    /* audioIns  */ 0,
    /* audioOuts */ 2,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 1,
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
