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

#include "water/threads/ScopedLock.h"
#include "water/threads/SpinLock.h"

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Weffc++"
#endif

#include "zita-resampler/resampler.h"

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif

#ifdef CARLA_OS_WIN
# include <windows.h>
# define CARLA_MLOCK(ptr, size) VirtualLock((ptr), (size))
#else
# include <sys/mman.h>
# define CARLA_MLOCK(ptr, size) mlock((ptr), (size))
#endif

typedef struct adinfo ADInfo;

struct AudioFilePool {
    float*   buffer[2];
    float*   tmpbuf[2];
    uint32_t numFrames;
    volatile uint64_t startFrame;

#ifdef CARLA_PROPER_CPP11_SUPPORT
    AudioFilePool() noexcept
        : buffer{nullptr},
          tmpbuf{nullptr},
          numFrames(0),
          startFrame(0) {}
#else
    AudioFilePool() noexcept
        : numFrames(0),
          startFrame(0)
    {
        buffer[0] = buffer[1] = nullptr;
        tmpbuf[0] = tmpbuf[1] = nullptr;
    }
#endif

    ~AudioFilePool()
    {
        destroy();
    }

    void create(const uint32_t desiredNumFrames, const bool withTempBuffers)
    {
        CARLA_ASSERT(buffer[0] == nullptr);
        CARLA_ASSERT(buffer[1] == nullptr);
        CARLA_ASSERT(tmpbuf[0] == nullptr);
        CARLA_ASSERT(tmpbuf[1] == nullptr);
        CARLA_ASSERT(startFrame == 0);
        CARLA_ASSERT(numFrames == 0);

        numFrames = desiredNumFrames;
        buffer[0] = new float[numFrames];
        buffer[1] = new float[numFrames];
        CARLA_MLOCK(buffer[0], sizeof(float)*numFrames);
        CARLA_MLOCK(buffer[1], sizeof(float)*numFrames);

        if (withTempBuffers)
        {
            tmpbuf[0] = new float[numFrames];
            tmpbuf[1] = new float[numFrames];
            CARLA_MLOCK(tmpbuf[0], sizeof(float)*numFrames);
            CARLA_MLOCK(tmpbuf[1], sizeof(float)*numFrames);
        }

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

        if (tmpbuf[0] != nullptr)
        {
            delete[] tmpbuf[0];
            tmpbuf[0] = nullptr;
        }

        if (tmpbuf[1] != nullptr)
        {
            delete[] tmpbuf[1];
            tmpbuf[1] = nullptr;
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

            if (tmpbuf[0] != nullptr)
                carla_zeroFloats(tmpbuf[0], numFrames);
            if (tmpbuf[1] != nullptr)
                carla_zeroFloats(tmpbuf[1], numFrames);
        }
    }

    CARLA_DECLARE_NON_COPY_STRUCT(AudioFilePool)
};

class AudioFileReader
{
public:
    AudioFileReader()
        : fEntireFileLoaded(false),
          fLoopingMode(true),
          fNeedsFrame(0),
          fNeedsRead(false),
          fFilePtr(nullptr),
          fFileNfo(),
          fMaxFrame(0),
          fResampleRatio(0.0),
          fPollTempData(nullptr),
          fPollTempSize(0),
          fResampleTempData(nullptr),
          fResampleTempSize(0),
          fPool(),
          fResampler(),
          fPoolMutex(),
          fReaderMutex()
    {
        static bool adInitiated = false;

        if (! adInitiated)
        {
            ad_init();
            adInitiated = true;
        }

        ad_clear_nfo(&fFileNfo);
    }

    ~AudioFileReader()
    {
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

        if (fResampleTempData != nullptr)
        {
            delete[] fResampleTempData;
            fResampleTempData = nullptr;
            fResampleTempSize = 0;
        }

        fPool.destroy();
    }

    void reset()
    {
        const water::GenericScopedLock<water::SpinLock> gsl(fPoolMutex);
        const CarlaMutexLocker cml(fReaderMutex);

        fNeedsFrame = 0;
        fNeedsRead = false;

        fPool.reset();
    }

    bool isEntireFileLoaded() const noexcept
    {
        return fEntireFileLoaded;
    }

    uint32_t getMaxFrame() const noexcept
    {
        return fMaxFrame;
    }

    uint64_t getPoolStartFrame() const noexcept
    {
        return fPool.startFrame;
    }

    uint32_t getPoolNumFrames() const noexcept
    {
        return fPool.numFrames;
    }

    ADInfo getFileInfo() const noexcept
    {
        return fFileNfo;
    }

    void setLoopingMode(const bool on) noexcept
    {
        fLoopingMode = on;
    }

    void setNeedsRead(const uint64_t frame) noexcept
    {
        if (fEntireFileLoaded)
            return;

        fNeedsFrame = frame;
        fNeedsRead = true;
    }

    bool loadFilename(const char* const filename, const uint32_t sampleRate)
    {
        CARLA_SAFE_ASSERT_RETURN(filename != nullptr && *filename != '\0', false);

        const CarlaMutexLocker cml(fReaderMutex);

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
            const uint32_t maxPoolNumFrames = sampleRate * 30;
            const bool needsResample = fFileNfo.sample_rate != sampleRate;

            if (needsResample)
            {
                if (! fResampler.setup(fFileNfo.sample_rate, sampleRate, fFileNfo.channels, 32))
                {
                    ad_clear_nfo(&fFileNfo);
                    ad_close(fFilePtr);
                    fFilePtr = nullptr;
                    carla_stderr2("loadFilename error, resampler setup failed");
                    return false;
                }

                fResampleRatio = static_cast<double>(sampleRate) / static_cast<double>(fFileNfo.sample_rate);
            }
            else
            {
                fResampler.clear();
                fResampleRatio = 0.0;
            }

            if (fileNumFrames <= maxPoolNumFrames)
            {
                // entire file fits in a small pool, lets read it now
                fPool.create(needsResample ? static_cast<uint32_t>(static_cast<double>(fileNumFrames) * fResampleRatio + 0.5)
                                           : fileNumFrames, false);
                readEntireFileIntoPool(needsResample);
                ad_close(fFilePtr);
                fFilePtr = nullptr;
            }
            else
            {
                // file is too big for our audio pool, we need an extra buffer
                const uint32_t poolNumFrames = sampleRate * 1;
                const uint pollTempSize = poolNumFrames * fFileNfo.channels;
                uint resampleTempSize = 0;

                fPool.create(poolNumFrames, true);

                try {
                    fPollTempData = new float[pollTempSize];
                } catch (...) {
                    ad_clear_nfo(&fFileNfo);
                    ad_close(fFilePtr);
                    fFilePtr = nullptr;
                    carla_stderr2("loadFilename error, out of memory");
                    return false;
                }

                CARLA_MLOCK(fPollTempData, sizeof(float)*pollTempSize);

                if (needsResample)
                {
                    resampleTempSize = static_cast<uint32_t>(static_cast<double>(poolNumFrames) * fResampleRatio + 0.5);
                    resampleTempSize *= fFileNfo.channels;

                    try {
                        fResampleTempData = new float[resampleTempSize];
                    } catch (...) {
                        delete[] fPollTempData;
                        fPollTempData = nullptr;
                        ad_clear_nfo(&fFileNfo);
                        ad_close(fFilePtr);
                        fFilePtr = nullptr;
                        carla_stderr2("loadFilename error, out of memory");
                        return false;
                    }

                    CARLA_MLOCK(fResampleTempData, sizeof(float)*resampleTempSize);
                }

                fPollTempSize = pollTempSize;
                fResampleTempSize = resampleTempSize;
            }

            fMaxFrame = carla_isNotZero(fResampleRatio)
                        ? static_cast<uint32_t>(static_cast<double>(fileNumFrames) * fResampleRatio + 0.5)
                        : fileNumFrames;

            fNeedsRead = true;
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

        const water::GenericScopedLock<water::SpinLock> gsl(fPoolMutex);

        pool.startFrame = fPool.startFrame;
        carla_copyFloats(pool.buffer[0], fPool.buffer[0], fPool.numFrames);
        carla_copyFloats(pool.buffer[1], fPool.buffer[1], fPool.numFrames);
    }

    bool tryPutData(float* const out1,
                    float* const out2,
                    uint64_t framePos,
                    const uint32_t frames,
                    const bool isOffline,
                    bool& needsRead)
    {
        CARLA_SAFE_ASSERT_RETURN(fPool.numFrames != 0, false);

        if (framePos >= fMaxFrame)
        {
            if (fLoopingMode)
                framePos %= fMaxFrame;
            else
                return false;
        }

        uint64_t frameDiff;
        const uint64_t numFramesNearEnd = fPool.numFrames*3/5;

        const water::GenericScopedLock<water::SpinLock> gsl(fPoolMutex);

        if (framePos < fPool.startFrame)
        {
            if (fPool.startFrame + fPool.numFrames <= fMaxFrame)
            {
                needsRead = true;
                setNeedsRead(framePos);
                return false;
            }

            frameDiff = framePos + (fMaxFrame - fPool.startFrame);

            if (frameDiff + frames >= fPool.numFrames)
            {
                needsRead = true;
                setNeedsRead(framePos);
                return false;
            }

            carla_copyFloats(out1, fPool.buffer[0] + frameDiff, frames);
            carla_copyFloats(out2, fPool.buffer[1] + frameDiff, frames);
        }
        else
        {
            frameDiff = framePos - fPool.startFrame;

            if (frameDiff + frames >= fPool.numFrames)
            {
                needsRead = true;
                setNeedsRead(framePos);
                return false;
            }

            carla_copyFloats(out1, fPool.buffer[0] + frameDiff, frames);
            carla_copyFloats(out2, fPool.buffer[1] + frameDiff, frames);
        }

        if (frameDiff > numFramesNearEnd)
        {
            needsRead = true;
            setNeedsRead(framePos + (isOffline ? 0 : frames));
        }

        return true;
    }

    void readEntireFileIntoPool(const bool needsResample)
    {
        CARLA_SAFE_ASSERT_RETURN(fPool.numFrames > 0,);

        const uint numChannels = fFileNfo.channels;
        const uint fileNumFrames = static_cast<uint>(fFileNfo.frames);
        const uint bufferSize = fileNumFrames * numChannels;

        float* const buffer = (float*)std::calloc(bufferSize, sizeof(float));
        CARLA_SAFE_ASSERT_RETURN(buffer != nullptr,);

        ad_seek(fFilePtr, 0);
        ssize_t rv = ad_read(fFilePtr, buffer, bufferSize);
        CARLA_SAFE_ASSERT_INT2_RETURN(rv == static_cast<ssize_t>(bufferSize),
                                      static_cast<int>(rv),
                                      static_cast<int>(bufferSize),
                                      std::free(buffer));

        float* rbuffer;

        if (needsResample)
        {
            const uint rbufferSize = fPool.numFrames * numChannels;
            rbuffer = (float*)std::calloc(rbufferSize, sizeof(float));
            CARLA_SAFE_ASSERT_RETURN(rbuffer != nullptr, std::free(buffer););

            rv = static_cast<ssize_t>(rbufferSize);

            fResampler.inp_count = fileNumFrames;
            fResampler.out_count = fPool.numFrames;
            fResampler.inp_data = buffer;
            fResampler.out_data = rbuffer;
            fResampler.process();
            CARLA_SAFE_ASSERT_INT(fResampler.inp_count <= 2, fResampler.inp_count);
        }
        else
        {
            rbuffer = buffer;
        }

        {
            // lock, and put data asap
            const water::GenericScopedLock<water::SpinLock> gsl(fPoolMutex);

            if (numChannels == 1)
            {
                for (ssize_t i=0, j=0; j < rv; ++i, ++j)
                    fPool.buffer[0][i] = fPool.buffer[1][i] = rbuffer[j];
            }
            else
            {
                for (ssize_t i=0, j=0; j < rv; ++j)
                {
                    if (j % 2 == 0)
                    {
                        fPool.buffer[0][i] = rbuffer[j];
                    }
                    else
                    {
                        fPool.buffer[1][i] = rbuffer[j];
                        ++i;
                    }
                }
            }
        }

        if (rbuffer != buffer)
            std::free(rbuffer);

        std::free(buffer);

        fEntireFileLoaded = true;
    }

    void readPoll()
    {
        const CarlaMutexLocker cml(fReaderMutex);

        if (fMaxFrame == 0 || fFileNfo.channels == 0 || fFilePtr == nullptr)
        {
            carla_debug("R: no song loaded");
            fNeedsFrame = 0;
            fNeedsRead = false;
            return;
        }
        if (fPollTempData == nullptr)
        {
            carla_debug("R: nothing to poll");
            fNeedsFrame = 0;
            fNeedsRead = false;
            return;
        }

        uint64_t lastFrame = fNeedsFrame;
        int64_t readFrameCheck;

        if (lastFrame >= fMaxFrame)
        {
            if (fLoopingMode)
            {
                const uint64_t readFrameCheckLoop = lastFrame % fMaxFrame;
                CARLA_SAFE_ASSERT_RETURN(readFrameCheckLoop < INT32_MAX,);

                carla_debug("R: transport out of bounds for loop");
                readFrameCheck = static_cast<int64_t>(readFrameCheckLoop);
            }
            else
            {
                carla_debug("R: transport out of bounds");
                fNeedsFrame = 0;
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

            const int64_t readFrameReal = carla_isNotZero(fResampleRatio)
                                        ? static_cast<int64_t>(static_cast<double>(readFrame) / fResampleRatio + 0.5)
                                        : readFrame;

            ad_seek(fFilePtr, readFrameReal);
            size_t i = 0;
            ssize_t j = 0;
            ssize_t rv = ad_read(fFilePtr, fPollTempData, fPollTempSize);

            if (rv < 0)
            {
                carla_stderr("R: ad_read1 failed");
                fNeedsFrame = 0;
                fNeedsRead = false;
                return;
            }

            const size_t urv = static_cast<size_t>(rv);

            // see if we can read more
            if (readFrameReal + rv >= static_cast<ssize_t>(fFileNfo.frames) && urv < fPollTempSize)
            {
                carla_debug("R: from start");
                ad_seek(fFilePtr, 0);
                j = ad_read(fFilePtr, fPollTempData+urv, fPollTempSize-urv);

                if (j < 0)
                {
                    carla_stderr("R: ad_read2 failed");
                    fNeedsFrame = 0;
                    fNeedsRead = false;
                    return;
                }

                rv += j;
            }

            carla_debug("R: reading %li frames at frame %lu", rv, readFrameCheck);

            // local copy
            const uint32_t poolNumFrames = fPool.numFrames;
            float* const pbuffer0 = fPool.tmpbuf[0];
            float* const pbuffer1 = fPool.tmpbuf[1];
            const float* tmpbuf = fPollTempData;

            // resample as needed
            if (fResampleTempSize != 0)
            {
                tmpbuf = fResampleTempData;
                fResampler.inp_count = static_cast<uint>(rv / fFileNfo.channels);
                fResampler.out_count = fResampleTempSize / fFileNfo.channels;
                fResampler.inp_data = fPollTempData;
                fResampler.out_data = fResampleTempData;
                fResampler.process();
                CARLA_ASSERT_INT(fResampler.inp_count <= 1, fResampler.inp_count);
            }

            j = 0;
            do {
                if (fFileNfo.channels == 1)
                {
                    for (; i < poolNumFrames && j < rv; ++i, ++j)
                        pbuffer0[i] = pbuffer1[i] = tmpbuf[j];
                }
                else
                {
                    for (; i < poolNumFrames && j < rv; ++j)
                    {
                        if (j % 2 == 0)
                        {
                            pbuffer0[i] = tmpbuf[j];
                        }
                        else
                        {
                            pbuffer1[i] = tmpbuf[j];
                            ++i;
                        }
                    }
                }

                if (i >= poolNumFrames)
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

                    carla_zeroFloats(pbuffer0, poolNumFrames - i);
                    carla_zeroFloats(pbuffer1, poolNumFrames - i);
                    break;
                }

            } while (i < poolNumFrames);

            // lock, and put data asap
            const water::GenericScopedLock<water::SpinLock> gsl(fPoolMutex);

            std::memcpy(fPool.buffer[0], pbuffer0, sizeof(float)*poolNumFrames);
            std::memcpy(fPool.buffer[1], pbuffer1, sizeof(float)*poolNumFrames);
            fPool.startFrame = static_cast<uint64_t>(readFrame);
        }

        fNeedsRead = false;
    }

private:
    bool fEntireFileLoaded;
    bool fLoopingMode;
    volatile uint64_t fNeedsFrame;
    volatile bool fNeedsRead;

    void*  fFilePtr;
    ADInfo fFileNfo;

    uint32_t fMaxFrame;
    double fResampleRatio;

    float* fPollTempData;
    uint fPollTempSize;

    float* fResampleTempData;
    uint fResampleTempSize;

    AudioFilePool fPool;
    Resampler     fResampler;

    water::SpinLock fPoolMutex;
    CarlaMutex      fReaderMutex;

    CARLA_DECLARE_NON_COPY_STRUCT(AudioFileReader)
};

#endif // AUDIO_BASE_HPP_INCLUDED
