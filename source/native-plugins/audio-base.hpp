/*
 * Carla Native Plugins
 * Copyright (C) 2013-2019 Filipe Coelho <falktx@falktx.com>
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
    uint32_t numFrames;
    volatile uint64_t startFrame;

#ifdef CARLA_PROPER_CPP11_SUPPORT
    AudioFilePool() noexcept
        : buffer{nullptr},
          numFrames(0),
          startFrame(0) {}
#else
    AudioFilePool() noexcept
        : numFrames(0),
          startFrame(0)
    {
        buffer[0] = buffer[1] = nullptr;
    }
#endif

    ~AudioFilePool()
    {
        destroy();
    }

    void create(const uint32_t desiredNumFrames)
    {
        CARLA_ASSERT(buffer[0] == nullptr);
        CARLA_ASSERT(buffer[1] == nullptr);
        CARLA_ASSERT(startFrame == 0);
        CARLA_ASSERT(numFrames == 0);

        numFrames = desiredNumFrames;
        buffer[0] = new float[numFrames];
        buffer[1] = new float[numFrames];

        reset();
    }

    void destroy() noexcept
    {
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
        numFrames = 0;
    }

    void reset() noexcept
    {
        startFrame = 0;

        if (numFrames != 0)
        {
            carla_zeroFloats(buffer[0], numFrames);
            carla_zeroFloats(buffer[1], numFrames);
        }
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
    AudioFileThread(AbstractAudioPlayer* const player)
        : CarlaThread("AudioFileThread"),
          kPlayer(player),
          fEntireFileLoaded(false),
          fLoopingMode(true),
          fNeedsRead(false),
          fQuitNow(true),
          fFilePtr(nullptr),
          fFileNfo(),
          fNumFileFrames(0),
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
    }

    ~AudioFileThread() override
    {
        CARLA_ASSERT(fQuitNow);
        CARLA_ASSERT(! isThreadRunning());

        cleanup();
    }

    void cleanup()
    {
        fEntireFileLoaded = false;

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
        if (fPollTempData == nullptr)
            return;

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

    bool isEntireFileLoaded() const noexcept
    {
        return fEntireFileLoaded;
    }

    uint32_t getMaxFrame() const noexcept
    {
        return fNumFileFrames;
    }

    uint64_t getPoolStartFrame() const noexcept
    {
        return fPool.startFrame;
    }

    uint32_t getPoolNumFrames() const noexcept
    {
        return fPool.numFrames;
    }

    void setLoopingMode(const bool on) noexcept
    {
        fLoopingMode = on;
    }

    void setNeedsRead() noexcept
    {
        fNeedsRead = true;
    }

    bool loadFilename(const char* const filename, const uint32_t sampleRate)
    {
        CARLA_SAFE_ASSERT_RETURN(! isThreadRunning(), false);
        CARLA_SAFE_ASSERT_RETURN(filename != nullptr && *filename != '\0', false);

        cleanup();
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
            const uint32_t fileNumFrames = static_cast<uint32_t>(fFileNfo.frames);
            const uint32_t poolNumFrames = sampleRate * 5;

            if (fileNumFrames <= poolNumFrames)
            {
                // entire file fits in a small pool, lets read it now
                fPool.create(fileNumFrames);
                readEntireFileIntoPool();
                ad_close(fFilePtr);
                fFilePtr = nullptr;
            }
            else
            {
                // file is too big for our audio pool, we need an extra buffer
                fPool.create(poolNumFrames);

                const size_t pollTempSize = poolNumFrames * fFileNfo.channels;

                try {
                    fPollTempData = new float[pollTempSize];
                } catch (...) {
                    ad_close(fFilePtr);
                    fFilePtr = nullptr;
                    return false;
                }

                fPollTempSize = pollTempSize;
            }

            fNumFileFrames = fileNumFrames;

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

    void putAllData(AudioFilePool& pool)
    {
        CARLA_SAFE_ASSERT_RETURN(pool.numFrames == fPool.numFrames,);

        const CarlaMutexLocker cml(fMutex);

        pool.startFrame = fPool.startFrame;
        carla_copyFloats(pool.buffer[0], fPool.buffer[0], fPool.numFrames);
        carla_copyFloats(pool.buffer[1], fPool.buffer[1], fPool.numFrames);
    }

    bool tryPutData(AudioFilePool& pool, const uint64_t framePos, const uint32_t frames)
    {
        CARLA_SAFE_ASSERT_RETURN(pool.numFrames == fPool.numFrames, false);

        if (framePos >= fPool.numFrames)
            return false;

        const CarlaMutexLocker cml(fMutex);
        /*
        const CarlaMutexTryLocker cmtl(fMutex);
        if (! cmtl.wasLocked())
            return false;
        */

        pool.startFrame = fPool.startFrame;

        carla_copyFloats(pool.buffer[0] + framePos, fPool.buffer[0] + framePos, frames);
        carla_copyFloats(pool.buffer[1] + framePos, fPool.buffer[1] + framePos, frames);

        return true;
    }

    void readEntireFileIntoPool()
    {
        CARLA_SAFE_ASSERT_RETURN(fPool.numFrames > 0,);

        const uint numChannels = fFileNfo.channels;
        const size_t bufferSize = fPool.numFrames * numChannels;
        float* const buffer = (float*)std::malloc(bufferSize*sizeof(float));
        CARLA_SAFE_ASSERT_RETURN(buffer != nullptr,);

        carla_zeroFloats(buffer, bufferSize);

        ad_seek(fFilePtr, 0);
        ssize_t rv = ad_read(fFilePtr, buffer, bufferSize);
        CARLA_SAFE_ASSERT_INT2_RETURN(rv == static_cast<ssize_t>(bufferSize),
                                      static_cast<int>(rv),
                                      static_cast<int>(bufferSize),
                                      std::free(buffer));

        {
            // lock, and put data asap
            const CarlaMutexLocker cml(fMutex);

            for (ssize_t i=0, j=0; j < rv; ++j)
            {
                if (numChannels == 1)
                {
                    fPool.buffer[0][i] = buffer[j];
                    fPool.buffer[1][i] = buffer[j];
                    ++i;
                }
                else
                {
                    if (j % 2 == 0)
                    {
                        fPool.buffer[0][i] = buffer[j];
                    }
                    else
                    {
                        fPool.buffer[1][i] = buffer[j];
                        ++i;
                    }
                }
            }
        }

        std::free(buffer);

        fEntireFileLoaded = true;
    }

    void readPoll()
    {
        if (fNumFileFrames == 0 || fFileNfo.channels == 0 || fFilePtr == nullptr)
        {
            carla_debug("R: no song loaded");
            fNeedsRead = false;
            return;
        }
        if (fPollTempData == nullptr)
        {
            carla_debug("R: nothing to poll");
            fNeedsRead = false;
            return;
        }

        uint64_t lastFrame = kPlayer->getLastFrame();
        int64_t readFrameCheck;

        if (lastFrame >= fNumFileFrames)
        {
            if (fLoopingMode)
            {
                const uint64_t readFrameCheckLoop = lastFrame % fNumFileFrames;
                CARLA_SAFE_ASSERT_RETURN(readFrameCheckLoop < INT32_MAX,);

                carla_debug("R: transport out of bounds for loop");
                readFrameCheck = static_cast<int64_t>(readFrameCheckLoop);
            }
            else
            {
                carla_debug("R: transport out of bounds");
                fNeedsRead = false;
                return;
            }
        }
        else
        {
            CARLA_SAFE_ASSERT_RETURN(lastFrame < INT32_MAX,);
            readFrameCheck = static_cast<int64_t>(lastFrame);
        }

        const int64_t readFrame = readFrameCheck;

        // temp data buffer
        carla_zeroFloats(fPollTempData, fPollTempSize);

        {
#if 0
            const int32_t sampleRate = 44100;
            carla_debug("R: poll data - reading at frame %li, time %li:%02li, lastFrame %li",
                        readFrame, readFrame/sampleRate/60, (readFrame/sampleRate) % 60, lastFrame);
#endif

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
                for (; i < fPool.numFrames && j < rv; ++j)
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

                if (i >= fPool.numFrames)
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

                    carla_zeroFloats(fPool.buffer[0] + i, fPool.numFrames - i);
                    carla_zeroFloats(fPool.buffer[1] + i, fPool.numFrames - i);
                    break;
                }

            } while (i < fPool.numFrames);

            fPool.startFrame = lastFrame;
        }

        fNeedsRead = false;
    }

protected:
    void run() override
    {
        const uint64_t numFramesNearEnd = fPool.numFrames*3/4;
        uint64_t lastFrame;

        while (! fQuitNow)
        {
            lastFrame = kPlayer->getLastFrame();

            if (fNeedsRead || lastFrame < fPool.startFrame || lastFrame - fPool.startFrame >= numFramesNearEnd)
                readPoll();

            carla_msleep(50);
        }
    }

private:
    AbstractAudioPlayer* const kPlayer;

    bool fEntireFileLoaded;
    bool fLoopingMode;
    volatile bool fNeedsRead;
    volatile bool fQuitNow;

    void*  fFilePtr;
    ADInfo fFileNfo;

    uint32_t fNumFileFrames;

    float* fPollTempData;
    size_t fPollTempSize;

    AudioFilePool fPool;
    CarlaMutex    fMutex;

    CARLA_DECLARE_NON_COPY_STRUCT(AudioFileThread)
};

#endif // AUDIO_BASE_HPP_INCLUDED
