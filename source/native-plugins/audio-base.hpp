/*
 * Carla Native Plugins
 * Copyright (C) 2013-2018 Filipe Coelho <falktx@falktx.com>
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

#ifndef AUDIO_BASE_HPP_INCLUDED
#define AUDIO_BASE_HPP_INCLUDED

#include "CarlaThread.hpp"
#include "CarlaMathUtils.hpp"

extern "C" {
#include "audio_decoder/ad.h"
}

typedef struct adinfo ADInfo;

struct AudioFilePool {
    float*   buffer[2];
    uint64_t startFrame;
    uint32_t sampleRate;
    uint32_t size;

#ifdef CARLA_PROPER_CPP11_SUPPORT
    AudioFilePool()
        : buffer{nullptr},
          startFrame(0),
          sampleRate(0),
          size(0) {}
#else
    AudioFilePool()
        : startFrame(0),
          sampleRate(0),
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

    void create(const uint32_t srate)
    {
        CARLA_ASSERT(buffer[0] == nullptr);
        CARLA_ASSERT(buffer[1] == nullptr);
        CARLA_ASSERT(startFrame == 0);
        CARLA_ASSERT(size == 0);

        size = srate * 8;
        sampleRate = srate;

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
        startFrame = 0;
        CARLA_SAFE_ASSERT_RETURN(size != 0,);

        carla_zeroFloats(buffer[0], size);
        carla_zeroFloats(buffer[1], size);
    }

    CARLA_DECLARE_NON_COPY_STRUCT(AudioFilePool)
};

class AbstractAudioPlayer
{
public:
    virtual ~AbstractAudioPlayer() {}
    virtual uint64_t getLastFrame() const = 0;
};

class AudioFileThread : public CarlaThread
{
public:
    AudioFileThread(AbstractAudioPlayer* const player, const uint32_t sampleRate)
        : CarlaThread("AudioFileThread"),
          kPlayer(player),
          fLoopingMode(true),
          fNeedsRead(false),
          fQuitNow(true),
          fFilePtr(nullptr),
          fFileNfo(),
          fMaxPlayerFrame(0),
          fPollTempData(nullptr),
          fPollTempSize(0),
          fPool(),
          fMutex()
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
        CARLA_ASSERT(fQuitNow);
        CARLA_ASSERT(! isThreadRunning());

        if (fFilePtr != nullptr)
        {
            ad_close(fFilePtr);
            fFilePtr = nullptr;
        }

        if (fPollTempData != nullptr)
        {
            delete[] fPollTempData;
            fPollTempData = nullptr;
            fPollTempSize = 0;
        }

        fPool.destroy();
    }

    void startNow()
    {
        fNeedsRead = true;
        fQuitNow = false;
        startThread();
    }

    void stopNow()
    {
        fNeedsRead = false;
        fQuitNow = true;

        stopThread(1000);

        const CarlaMutexLocker cml(fMutex);
        fPool.reset();
    }

    uint32_t getMaxFrame() const noexcept
    {
        return fMaxPlayerFrame;
    }

    void setLoopingMode(const bool on) noexcept
    {
        fLoopingMode = on;
    }

    void setNeedsRead() noexcept
    {
        fNeedsRead = true;
    }

    bool loadFilename(const char* const filename)
    {
        CARLA_SAFE_ASSERT_RETURN(! isThreadRunning(), false);
        CARLA_SAFE_ASSERT_RETURN(filename != nullptr && *filename != '\0', false);

        fPool.startFrame = 0;

        // clear old data
        if (fFilePtr != nullptr)
        {
            ad_close(fFilePtr);
            fFilePtr = nullptr;
        }
        if (fPollTempData != nullptr)
        {
            delete[] fPollTempData;
            fPollTempData = nullptr;
            fPollTempSize = 0;
            fMaxPlayerFrame = 0;
        }

        ad_clear_nfo(&fFileNfo);

        // open new
        fFilePtr = ad_open(filename, &fFileNfo);

        if (fFilePtr == nullptr)
            return false;

        ad_dump_nfo(99, &fFileNfo);

        // Fix for misinformation using libsndfile
        if (fFileNfo.frames % fFileNfo.channels)
            --fFileNfo.frames;

        if (fFileNfo.frames <= 0)
            carla_stderr("L: filename \"%s\" has 0 frames", filename);

        if ((fFileNfo.channels == 1 || fFileNfo.channels == 2) && fFileNfo.frames > 0)
        {
            // valid
            const size_t pollTempSize = std::min(static_cast<uint>(fFileNfo.frames),
                                                 fPool.size * fFileNfo.channels);

            try {
                fPollTempData = new float[pollTempSize];
            } catch (...) {
                ad_close(fFilePtr);
                fFilePtr = nullptr;
                return false;
            }

            fMaxPlayerFrame = static_cast<uint32_t>(fFileNfo.frames/fFileNfo.channels);
            fPollTempSize = pollTempSize;

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
        CARLA_SAFE_ASSERT_RETURN(pool.size == fPool.size,);

        if (! fMutex.tryLock())
            return;

        //if (pool.startFrame != fPool.startFrame || pool.buffer[0] != fPool.buffer[0] || pool.buffer[1] != fPool.buffer[1])
        {
            pool.startFrame = fPool.startFrame;
            carla_copyFloats(pool.buffer[0], fPool.buffer[0], fPool.size);
            carla_copyFloats(pool.buffer[1], fPool.buffer[1], fPool.size);
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

        const uint64_t lastFrame = kPlayer->getLastFrame();
        int32_t readFrameCheck;

        if (lastFrame >= static_cast<uint64_t>(fFileNfo.frames))
        {
            if (fLoopingMode)
            {
                const uint64_t readFrameCheckLoop = lastFrame % fMaxPlayerFrame;
                CARLA_SAFE_ASSERT_RETURN(readFrameCheckLoop < INT32_MAX,);

                carla_debug("R: transport out of bounds for loop");
                readFrameCheck = static_cast<int32_t>(readFrameCheckLoop);
            }
            else
            {
                carla_stderr("R: transport out of bounds");
                fNeedsRead = false;
                return;
            }
        }
        else
        {
            CARLA_SAFE_ASSERT_RETURN(lastFrame < INT32_MAX,);
            readFrameCheck = static_cast<int32_t>(lastFrame);
        }

        const int32_t readFrame = readFrameCheck;

        // temp data buffer
        carla_zeroFloats(fPollTempData, fPollTempSize);

        {
            carla_debug("R: poll data - reading at %li:%02li", readFrame/fPool.sampleRate/60, (readFrame/fPool.sampleRate) % 60);

            ad_seek(fFilePtr, readFrame);
            size_t i = 0;
            ssize_t j = 0;
            ssize_t rv = ad_read(fFilePtr, fPollTempData, fPollTempSize);

            if (rv < 0)
            {
                carla_stderr("R: ad_read failed");
                fNeedsRead = false;
                return;
            }

            const size_t urv = static_cast<size_t>(rv);

            // see if we can read more
            if (readFrame + rv >= static_cast<ssize_t>(fFileNfo.frames) && urv < fPollTempSize)
            {
                carla_debug("R: from start");
                ad_seek(fFilePtr, 0);
                rv += ad_read(fFilePtr, fPollTempData+urv, fPollTempSize-urv);
            }

            // lock, and put data asap
            const CarlaMutexLocker cml(fMutex);

            do {
                for (; i < fPool.size && j < rv; ++j)
                {
                    if (fFileNfo.channels == 1)
                    {
                        fPool.buffer[0][i] = fPollTempData[j];
                        fPool.buffer[1][i] = fPollTempData[j];
                        i++;
                    }
                    else
                    {
                        if (j % 2 == 0)
                        {
                            fPool.buffer[0][i] = fPollTempData[j];
                        }
                        else
                        {
                            fPool.buffer[1][i] = fPollTempData[j];
                            i++;
                        }
                    }
                }

                if (i >= fPool.size)
                    break;

                if (rv == fFileNfo.frames)
                {
                    // full file read
                    j = 0;
                    carla_debug("R: full file was read, filling buffers again");
                }
                else
                {
                    carla_debug("read break, not enough space");

                    // FIXME use carla_zeroFloats
                    for (; i < fPool.size; ++i)
                    {
                        fPool.buffer[0][i] = 0.0f;
                        fPool.buffer[1][i] = 0.0f;
                    }

                    break;
                }

            } while (i < fPool.size);

            fPool.startFrame = lastFrame;
        }

        fNeedsRead = false;
    }

protected:
    void run() override
    {
        while (! fQuitNow)
        {
            const uint64_t lastFrame = kPlayer->getLastFrame();
            const uint64_t loopedFrame = fLoopingMode ? lastFrame % fMaxPlayerFrame : lastFrame;

            if (fNeedsRead || lastFrame < fPool.startFrame || (lastFrame - fPool.startFrame >= fPool.size*3/4 && loopedFrame < fMaxPlayerFrame))
                readPoll();
            else
                carla_msleep(50);
        }
    }

private:
    AbstractAudioPlayer* const kPlayer;

    bool fLoopingMode;
    bool fNeedsRead;
    bool fQuitNow;

    void*  fFilePtr;
    ADInfo fFileNfo;

    uint32_t fMaxPlayerFrame;

    float* fPollTempData;
    size_t fPollTempSize;

    AudioFilePool fPool;
    CarlaMutex    fMutex;

    CARLA_DECLARE_NON_COPY_STRUCT(AudioFileThread)
};

#endif // AUDIO_BASE_HPP_INCLUDED
