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

#ifndef AUDIO_BASE_HPP_INCLUDED
#define AUDIO_BASE_HPP_INCLUDED

#include "CarlaMutex.hpp"

#include "JuceHeader.h"

extern "C" {
#include "audio_decoder/ad.h"
}

typedef struct adinfo ADInfo;

struct AudioFilePool {
    float*   buffer[2];
    uint32_t startFrame;
    uint32_t size;

#ifdef CARLA_PROPER_CPP11_SUPPORT
    AudioFilePool()
        : buffer{nullptr},
          startFrame(0),
          size(0) {}
#else
    AudioFilePool()
        : startFrame(0),
          size(0)
    {
        buffer[0] = buffer[1] = nullptr;
    }
#endif

    ~AudioFilePool()
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

        size = sampleRate * 2;

        buffer[0] = new float[size];
        buffer[1] = new float[size];

        reset();
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

    void reset()
    {
        CARLA_ASSERT(size != 0);

        startFrame = 0;
        carla_zeroFloat(buffer[0], size);
        carla_zeroFloat(buffer[1], size);
    }
};

class AbstractAudioPlayer
{
public:
    virtual ~AbstractAudioPlayer() {}
    virtual uint32_t getLastFrame() const = 0;
};

class AudioFileThread : public juce::Thread
{
public:
    AudioFileThread(AbstractAudioPlayer* const player, const double sampleRate)
        : juce::Thread("AudioFileThread"),
          kPlayer(player),
          fNeedsRead(false),
          fFilePtr(nullptr)
    {
        CARLA_ASSERT(kPlayer != nullptr);

        static bool adInitiated = false;

        if (! adInitiated)
        {
            ad_init();
            adInitiated = true;
        }

        ad_clear_nfo(&fFileNfo);

        fPool.create(sampleRate);
    }

    ~AudioFileThread() override
    {
        CARLA_ASSERT(! isThreadRunning());

        if (fFilePtr != nullptr)
            ad_close(fFilePtr);

        fPool.destroy();
    }

    void startNow()
    {
        fNeedsRead = true;
        startThread(2);
    }

    void stopNow()
    {
        fNeedsRead = false;
        stopThread(1000);

        const CarlaMutex::ScopedLocker sl(fMutex);
        fPool.reset();
    }

    uint32_t getMaxFrame() const
    {
        return fFileNfo.frames > 0 ? fFileNfo.frames : 0;
    }

    void setNeedsRead()
    {
        fNeedsRead = true;
    }

    bool loadFilename(const char* const filename)
    {
        CARLA_ASSERT(! isThreadRunning());
        CARLA_ASSERT(filename != nullptr);

        fPool.startFrame = 0;

        // clear old data
        if (fFilePtr != nullptr)
        {
            ad_close(fFilePtr);
            fFilePtr = nullptr;
        }

        ad_clear_nfo(&fFileNfo);

        // open new
        fFilePtr = ad_open(filename, &fFileNfo);

        if (fFilePtr == nullptr)
            return false;

        ad_dump_nfo(99, &fFileNfo);

        if (fFileNfo.frames == 0)
            carla_stderr("L: filename \"%s\" has 0 frames", filename);

        if ((fFileNfo.channels == 1 || fFileNfo.channels == 2) && fFileNfo.frames > 0)
        {
            // valid
            readPoll();
            return true;
        }
        else
        {
            // invalid
            ad_clear_nfo(&fFileNfo);
            ad_close(fFilePtr);
            fFilePtr = nullptr;
            return false;
        }
    }

    void tryPutData(AudioFilePool& pool)
    {
        CARLA_ASSERT(pool.size == fPool.size);

        if (pool.size != fPool.size)
            return;
        if (! fMutex.tryLock())
            return;

        //if (pool.startFrame != fPool.startFrame || pool.buffer[0] != fPool.buffer[0] || pool.buffer[1] != fPool.buffer[1])
        {
            pool.startFrame = fPool.startFrame;
            carla_copyFloat(pool.buffer[0], fPool.buffer[0], fPool.size);
            carla_copyFloat(pool.buffer[1], fPool.buffer[1], fPool.size);
        }

        fMutex.unlock();
    }

    void readPoll()
    {
        if (fFileNfo.frames <= 0 || fFilePtr == nullptr)
        {
            carla_stderr("R: no song loaded");
            fNeedsRead = false;
            return;
        }

        int64_t lastFrame = kPlayer->getLastFrame();
        int64_t readFrame = lastFrame;
        int64_t maxFrame  = fFileNfo.frames;

        if (lastFrame >= maxFrame)
        {
#if 0
            if (false)
            //if (handlePtr->loopMode)
            {
                carla_stderr("R: DEBUG read loop, lastFrame:%i, maxFrame:%i", lastFrame, maxFrame);

                if (maxFrame >= static_cast<int64_t>(fPool.size))
                {
                    readFrame %= maxFrame;
                }
                else
                {
                    readFrame  = 0;
                    lastFrame -= lastFrame % maxFrame;
                }
            }
            else
#endif
            {
                carla_stderr("R: transport out of bounds");
                fNeedsRead = false;
                return;
            }
        }

        // temp data buffer
        const size_t tmpSize = fPool.size * fFileNfo.channels;

        float tmpData[tmpSize];
        carla_zeroFloat(tmpData, tmpSize);

        {
            carla_stderr("R: poll data - reading at %li:%02li", readFrame/44100/60, (readFrame/44100) % 60);

            ad_seek(fFilePtr, readFrame);
            ssize_t i, j, rv = ad_read(fFilePtr, tmpData, tmpSize);
            i = j = 0;

            // lock, and put data asap
            const CarlaMutex::ScopedLocker sl(fMutex);

            for (; i < fPool.size && j < rv; ++j)
            {
                if (fFileNfo.channels == 1)
                {
                    fPool.buffer[0][i] = tmpData[j];
                    fPool.buffer[1][i] = tmpData[j];
                    i++;
                }
                else
                {
                    if (j % 2 == 0)
                    {
                        fPool.buffer[0][i] = tmpData[j];
                    }
                    else
                    {
                        fPool.buffer[1][i] = tmpData[j];
                        i++;
                    }
                }
            }

#if 0
            if (false)
            //if (handlePtr->loopMode && i < fPool.size)
            {
                while (i < fPool.size)
                {
                    for (j=0; i < fPool.size && j < rv; ++j)
                    {
                        if (fFileNfo.channels == 1)
                        {
                            fPool.buffer[0][i] = tmpData[j];
                            fPool.buffer[1][i] = tmpData[j];
                            i++;
                        }
                        else
                        {
                            if (j % 2 == 0)
                            {
                                fPool.buffer[0][i] = tmpData[j];
                            }
                            else
                            {
                                fPool.buffer[1][i] = tmpData[j];
                                i++;
                            }
                        }
                    }
                }
            }
            else
#endif
            {
                for (; i < fPool.size; ++i)
                {
                    fPool.buffer[0][i] = 0.0f;
                    fPool.buffer[1][i] = 0.0f;
                }
            }

            fPool.startFrame = lastFrame;
        }

        fNeedsRead = false;
    }

protected:
    void run() override
    {
        while (! threadShouldExit())
        {
            const uint32_t lastFrame(kPlayer->getLastFrame());

            if (fNeedsRead || lastFrame < fPool.startFrame || (lastFrame - fPool.startFrame >= fPool.size*3/4 && lastFrame < fFileNfo.frames))
                readPoll();
            else
                carla_msleep(50);
        }
    }

private:
    AbstractAudioPlayer* const kPlayer;

    mutable bool fNeedsRead;

    void*  fFilePtr;
    ADInfo fFileNfo;

    AudioFilePool fPool;
    CarlaMutex    fMutex;
};

#endif // AUDIO_BASE_HPP_INCLUDED
