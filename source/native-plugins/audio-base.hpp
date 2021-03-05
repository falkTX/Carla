/*
 * Carla Native Plugins
 * Copyright (C) 2013-2021 Filipe Coelho <falktx@falktx.com>
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

// #define DEBUG_FILE_OPS

typedef struct adinfo ADInfo;

struct AudioFilePool {
    float*   buffer[2];
    float*   tmpbuf[2];
    uint32_t numFrames;
    uint32_t maxFrame;
    volatile uint64_t startFrame;
    water::SpinLock mutex;

#ifdef CARLA_PROPER_CPP11_SUPPORT
    AudioFilePool() noexcept
        : buffer{nullptr},
          tmpbuf{nullptr},
          numFrames(0),
          maxFrame(0),
          startFrame(0),
          mutex() {}
#else
    AudioFilePool() noexcept
        : numFrames(0),
          startFrame(0),
          mutex()
    {
        buffer[0] = buffer[1] = nullptr;
        tmpbuf[0] = tmpbuf[1] = nullptr;
    }
#endif

    ~AudioFilePool()
    {
        destroy();
    }

    void create(const uint32_t desiredNumFrames, const uint32_t fileNumFrames, const bool withTempBuffers)
    {
        CARLA_ASSERT(buffer[0] == nullptr);
        CARLA_ASSERT(buffer[1] == nullptr);
        CARLA_ASSERT(tmpbuf[0] == nullptr);
        CARLA_ASSERT(tmpbuf[1] == nullptr);
        CARLA_ASSERT(startFrame == 0);
        CARLA_ASSERT(numFrames == 0);
        CARLA_ASSERT(maxFrame == 0);

        buffer[0] = new float[desiredNumFrames];
        buffer[1] = new float[desiredNumFrames];
        carla_zeroFloats(buffer[0], desiredNumFrames);
        carla_zeroFloats(buffer[1], desiredNumFrames);
        CARLA_MLOCK(buffer[0], sizeof(float)*desiredNumFrames);
        CARLA_MLOCK(buffer[1], sizeof(float)*desiredNumFrames);

        if (withTempBuffers)
        {
            tmpbuf[0] = new float[desiredNumFrames];
            tmpbuf[1] = new float[desiredNumFrames];
            carla_zeroFloats(tmpbuf[0], desiredNumFrames);
            carla_zeroFloats(tmpbuf[1], desiredNumFrames);
            CARLA_MLOCK(tmpbuf[0], sizeof(float)*desiredNumFrames);
            CARLA_MLOCK(tmpbuf[1], sizeof(float)*desiredNumFrames);
        }

        const water::GenericScopedLock<water::SpinLock> gsl(mutex);

        startFrame = 0;
        numFrames = desiredNumFrames;
        maxFrame = fileNumFrames;
    }

    void destroy() noexcept
    {
        {
            const water::GenericScopedLock<water::SpinLock> gsl(mutex);
            startFrame = 0;
            numFrames = 0;
            maxFrame = 0;
        }

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
    }

    // NOTE it is assumed that mutex is locked
    bool tryPutData(float* const out1,
                    float* const out2,
                    uint64_t framePos,
                    const uint32_t frames,
                    const bool loopingMode,
                    const bool isOffline,
                    bool& needsRead,
                    uint64_t& needsReadFrame)
    {
        CARLA_SAFE_ASSERT_RETURN(numFrames != 0, false);
        CARLA_SAFE_ASSERT_RETURN(maxFrame != 0, false);

        if (framePos >= maxFrame)
        {
            if (loopingMode)
                framePos %= maxFrame;
            else
                return false;
        }

        uint64_t frameDiff;
        const uint32_t numFramesNearEnd = numFrames*3/4;

        if (framePos < startFrame)
        {
            if (startFrame + numFrames <= maxFrame)
            {
                needsRead = true;
                needsReadFrame = framePos;
                return false;
            }

            frameDiff = framePos + (maxFrame - startFrame);

            if (frameDiff + frames >= numFrames)
            {
                needsRead = true;
                needsReadFrame = framePos;
                return false;
            }

            carla_copyFloats(out1, buffer[0] + frameDiff, frames);
            carla_copyFloats(out2, buffer[1] + frameDiff, frames);
        }
        else
        {
            frameDiff = framePos - startFrame;

            if (frameDiff + frames >= numFrames)
            {
                needsRead = true;
                needsReadFrame = framePos;
                return false;
            }

            carla_copyFloats(out1, buffer[0] + frameDiff, frames);
            carla_copyFloats(out2, buffer[1] + frameDiff, frames);
        }

        if (frameDiff > numFramesNearEnd)
        {
            needsRead = true;
            needsReadFrame = framePos + (isOffline ? 0 : frames);
        }

        return true;
    }

    CARLA_DECLARE_NON_COPY_STRUCT(AudioFilePool)
};

class AudioFileReader
{
public:
    AudioFileReader()
        : fEntireFileLoaded(false),
          fLoopingMode(true),
          fCurrentBitRate(0),
          fNeedsFrame(0),
          fNeedsRead(false),
          fFilePtr(nullptr),
          fFileNfo(),
          fPollTempData(nullptr),
          fPollTempSize(0),
          fResampleRatio(0.0),
          fResampleTempData(nullptr),
          fResampleTempSize(0),
          fPool(),
          fPoolMutex(),
          fPoolReadyToSwap(false),
          fResampler(),
          fReaderMutex()
    {
        ad_clear_nfo(&fFileNfo);
    }

    ~AudioFileReader()
    {
        cleanup();
    }

    void cleanup()
    {
        fPool.destroy();
        fCurrentBitRate = 0;
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
    }

    void destroy()
    {
        const CarlaMutexLocker cml(fReaderMutex);

        fPool.destroy();
        fNeedsFrame = 0;
        fNeedsRead = false;
    }

    bool isEntireFileLoaded() const noexcept
    {
        return fEntireFileLoaded;
    }

    int getCurrentBitRate() const noexcept
    {
        return fCurrentBitRate;
    }

    uint32_t getMaxFrame() const noexcept
    {
        return fPool.maxFrame;
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

    bool loadFilename(const char* const filename, const uint32_t sampleRate,
                      const uint32_t previewDataSize, float* previewData)
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
            uint32_t maxFrame;

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
                maxFrame = static_cast<uint32_t>(static_cast<double>(fileNumFrames) * fResampleRatio + 0.5);
            }
            else
            {
                fResampler.clear();
                fResampleRatio = 0.0;
                maxFrame = fileNumFrames;
            }

            if (fileNumFrames <= maxPoolNumFrames || fFileNfo.can_seek == 0)
            {
                // entire file fits in a small pool, lets read it now
                const uint32_t poolNumFrames = needsResample
                                             ? static_cast<uint32_t>(static_cast<double>(fileNumFrames) * fResampleRatio + 0.5)
                                             : fileNumFrames;
                fPool.create(poolNumFrames, maxFrame, false);
                readEntireFileIntoPool(needsResample);
                ad_close(fFilePtr);
                fFilePtr = nullptr;

                const float fileNumFramesF = static_cast<float>(fileNumFrames);
                const float previewDataSizeF = static_cast<float>(previewDataSize);
                for (uint i=0; i<previewDataSize; ++i)
                {
                    const float stepF = static_cast<float>(i)/previewDataSizeF * fileNumFramesF;
                    const uint step = carla_fixedValue(0U, fileNumFrames-1U, static_cast<uint>(stepF + 0.5f));
                    previewData[i] = std::max(std::fabs(fPool.buffer[0][step]), std::fabs(fPool.buffer[1][step]));
                }
            }
            else
            {
                // file is too big for our audio pool, we need an extra buffer
                const uint32_t poolNumFrames = sampleRate * 5;
                const uint pollTempSize = poolNumFrames * fFileNfo.channels;
                uint resampleTempSize = 0;

                readFilePreview(previewDataSize, previewData);

                fPool.create(poolNumFrames, maxFrame, true);

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

    void createSwapablePool(AudioFilePool& pool)
    {
        pool.create(fPool.numFrames, fPool.maxFrame, false);
    }

    void putAndSwapAllData(AudioFilePool& pool)
    {
        const water::GenericScopedLock<water::SpinLock> gsl1(fPool.mutex);
        const water::GenericScopedLock<water::SpinLock> gsl2(pool.mutex);
        CARLA_SAFE_ASSERT_RETURN(fPool.numFrames != 0,);
        CARLA_SAFE_ASSERT_RETURN(fPool.buffer[0] != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fPool.tmpbuf[0] == nullptr,);
        CARLA_SAFE_ASSERT_RETURN(pool.numFrames == 0,);
        CARLA_SAFE_ASSERT_RETURN(pool.buffer[0] == nullptr,);
        CARLA_SAFE_ASSERT_RETURN(pool.tmpbuf[0] == nullptr,);

        pool.startFrame = fPool.startFrame;
        pool.numFrames = fPool.numFrames;
        pool.buffer[0] = fPool.buffer[0];
        pool.buffer[1] = fPool.buffer[1];

        fPool.startFrame = 0;
        fPool.numFrames = 0;
        fPool.buffer[0] = nullptr;
        fPool.buffer[1] = nullptr;
    }

    bool tryPutData(AudioFilePool& pool,
                    float* const out1,
                    float* const out2,
                    uint64_t framePos,
                    const uint32_t frames,
                    const bool loopMode,
                    const bool isOffline,
                    bool& needsIdleRequest)
    {
        _tryPoolSwap(pool);

        bool needsRead = false;
        uint64_t needsReadFrame;
        const bool ret = pool.tryPutData(out1, out2, framePos, frames, loopMode, isOffline, needsRead, needsReadFrame);

        if (needsRead)
        {
            needsIdleRequest = true;
            setNeedsRead(needsReadFrame);
        }

#ifdef DEBUG_FILE_OPS
        if (! ret) {
            carla_stdout("tryPutData fail");
        }
#endif
        return ret;
    }

    void readFilePreview(uint32_t previewDataSize, float* previewData)
    {
        carla_zeroFloats(previewData, previewDataSize);

        const uint fileNumFrames = static_cast<uint>(fFileNfo.frames);
        const float fileNumFramesF = static_cast<float>(fileNumFrames);
        const float previewDataSizeF = static_cast<float>(previewDataSize);
        const uint samplesPerRun = fFileNfo.channels;
        const uint maxSampleToRead = fileNumFrames - samplesPerRun;
        CARLA_SAFE_ASSERT_INT_RETURN(samplesPerRun == 1 || samplesPerRun == 2, samplesPerRun,);
        float tmp[2] = { 0.0f, 0.0f };

        if (samplesPerRun == 2)
            previewDataSize -= 1;

        for (uint i=0; i<previewDataSize; ++i)
        {
            const float posF = static_cast<float>(i)/previewDataSizeF * fileNumFramesF;
            const uint pos = carla_fixedValue(0U, maxSampleToRead, static_cast<uint>(posF));

            ad_seek(fFilePtr, pos);
            ad_read(fFilePtr, tmp, samplesPerRun);
            previewData[i] = std::max(std::fabs(tmp[0]), std::fabs(tmp[1]));
        }
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

        fCurrentBitRate = ad_get_bitrate(fFilePtr);

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
            const water::GenericScopedLock<water::SpinLock> gsl(fPool.mutex);

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

        if (fFileNfo.channels == 0 || fFilePtr == nullptr)
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

        const uint32_t maxFrame = fPool.maxFrame;
        uint64_t lastFrame = fNeedsFrame;
        int64_t readFrameCheck;

        if (lastFrame >= maxFrame)
        {
            if (fLoopingMode)
            {
                const uint64_t readFrameCheckLoop = lastFrame % maxFrame;
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
#ifdef DEBUG_FILE_OPS
                carla_stdout("R: from start");
#endif
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

#ifdef DEBUG_FILE_OPS
            carla_stdout("R: reading %li frames at frame %lu", rv, readFrameCheck);
#endif
            fCurrentBitRate = ad_get_bitrate(fFilePtr);

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
#ifdef DEBUG_FILE_OPS
                    carla_stdout("R: full file was read, filling buffers again");
#endif
                }
                else
                {
#ifdef DEBUG_FILE_OPS
                    carla_stdout("read break, not enough space");
#endif
                    carla_zeroFloats(pbuffer0, poolNumFrames - i);
                    carla_zeroFloats(pbuffer1, poolNumFrames - i);
                    break;
                }

            } while (i < poolNumFrames);

            // lock, and put data asap
            const CarlaMutexLocker cmlp(fPoolMutex);
            const water::GenericScopedLock<water::SpinLock> gsl(fPool.mutex);

            std::memcpy(fPool.buffer[0], pbuffer0, sizeof(float)*poolNumFrames);
            std::memcpy(fPool.buffer[1], pbuffer1, sizeof(float)*poolNumFrames);
            fPool.startFrame = static_cast<uint64_t>(readFrame);
            fPoolReadyToSwap = true;
#ifdef DEBUG_FILE_OPS
            carla_stdout("Reading done and internal pool is now full");
#endif
        }

        fNeedsRead = false;
    }

private:
    bool fEntireFileLoaded;
    bool fLoopingMode;
    int fCurrentBitRate;
    volatile uint64_t fNeedsFrame;
    volatile bool fNeedsRead;

    void*  fFilePtr;
    ADInfo fFileNfo;

    float* fPollTempData;
    uint fPollTempSize;

    double fResampleRatio;
    float* fResampleTempData;
    uint fResampleTempSize;

    AudioFilePool fPool;
    CarlaMutex    fPoolMutex;
    bool          fPoolReadyToSwap;
    Resampler     fResampler;
    CarlaMutex    fReaderMutex;

    // try a pool data swap if possible and relevant
    // NOTE it is assumed that `pool` mutex is locked
    void _tryPoolSwap(AudioFilePool& pool)
    {
        uint32_t tmp_u32;
        uint64_t tmp_u64;
        float* tmp_fp;

        const CarlaMutexTryLocker cmtl(fPoolMutex);

        if (! cmtl.wasLocked())
            return;

        const water::GenericScopedLock<water::SpinLock> gsl(fPool.mutex);

        if (! fPoolReadyToSwap)
            return;

        tmp_u64 = pool.startFrame;
        pool.startFrame = fPool.startFrame;
        fPool.startFrame = tmp_u64;

        tmp_u32 = pool.numFrames;
        pool.numFrames = fPool.numFrames;
        fPool.numFrames = tmp_u32;

        tmp_fp = pool.buffer[0];
        pool.buffer[0] = fPool.buffer[0];
        fPool.buffer[0] = tmp_fp;

        tmp_fp = pool.buffer[1];
        pool.buffer[1] = fPool.buffer[1];
        fPool.buffer[1] = tmp_fp;

        fPoolReadyToSwap = false;

#ifdef DEBUG_FILE_OPS
        carla_stdout("Pools have been swapped, internal one is now invalidated");
#endif
    }

    CARLA_DECLARE_NON_COPY_STRUCT(AudioFileReader)
};

#endif // AUDIO_BASE_HPP_INCLUDED
