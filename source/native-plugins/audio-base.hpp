/*
 * Carla Native Plugins
 * Copyright (C) 2013-2023 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaMathUtils.hpp"

extern "C" {
#include "audio_decoder/ad.h"
}

// #include "water/threads/ScopedLock.h"
// #include "water/threads/SpinLock.h"

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Weffc++"
#endif

#include "zita-resampler/resampler.h"

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif

#if defined(CARLA_OS_WIN)
# include <windows.h>
# define CARLA_MLOCK(ptr, size) VirtualLock((ptr), (size))
#elif !defined(CARLA_OS_WASM)
# include <sys/mman.h>
# define CARLA_MLOCK(ptr, size) mlock((ptr), (size))
#else
# define CARLA_MLOCK(ptr, size)
#endif

#define DEBUG_FILE_OPS

typedef struct adinfo ADInfo;

static inline
bool carla_mlock(void* const ptr, const size_t size)
{
   #if defined(CARLA_OS_WASM)
    // unsupported
    return false;
    (void)ptr; (void)size;
   #elif defined(CARLA_OS_WIN)
    return ::VirtualLock(ptr, size) != FALSE;
   #else
    return ::mlock(ptr, size) == 0;
   #endif
}

struct AudioMemoryPool {
    float* buffer[2] = {};
//     float*   tmpbuf[2];
    uint32_t numFrames = 0;
//     uint32_t maxFrame;
//     volatile uint64_t startFrame;
    CarlaMutex mutex;

    AudioMemoryPool() noexcept {}

    ~AudioMemoryPool() noexcept
    {
        destroy();
    }

    void create(const uint32_t desiredNumFrames/*, const uint32_t fileNumFrames, const bool withTempBuffers*/)
    {
        CARLA_ASSERT(buffer[0] == nullptr);
        CARLA_ASSERT(buffer[1] == nullptr);
//         CARLA_ASSERT(tmpbuf[0] == nullptr);
//         CARLA_ASSERT(tmpbuf[1] == nullptr);
//         CARLA_ASSERT(startFrame == 0);
        CARLA_ASSERT(numFrames == 0);
//         CARLA_ASSERT(maxFrame == 0);

        buffer[0] = new float[desiredNumFrames];
        buffer[1] = new float[desiredNumFrames];
//         carla_zeroFloats(buffer[0], desiredNumFrames);
//         carla_zeroFloats(buffer[1], desiredNumFrames);
        CARLA_MLOCK(buffer[0], sizeof(float)*desiredNumFrames);
        CARLA_MLOCK(buffer[1], sizeof(float)*desiredNumFrames);

//         if (withTempBuffers)
//         {
//             tmpbuf[0] = new float[desiredNumFrames];
//             tmpbuf[1] = new float[desiredNumFrames];
//             carla_zeroFloats(tmpbuf[0], desiredNumFrames);
//             carla_zeroFloats(tmpbuf[1], desiredNumFrames);
//             CARLA_MLOCK(tmpbuf[0], sizeof(float)*desiredNumFrames);
//             CARLA_MLOCK(tmpbuf[1], sizeof(float)*desiredNumFrames);
//         }
//
//         const water::GenericScopedLock<water::SpinLock> gsl(mutex);
//
//         startFrame = 0;
//         numFrames = desiredNumFrames;
//         maxFrame = fileNumFrames;
    }

    void destroy() noexcept
    {
        {
            const CarlaMutexLocker cml(mutex);
//             startFrame = 0;
            numFrames = 0;
//             maxFrame = 0;
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

//         if (tmpbuf[0] != nullptr)
//         {
//             delete[] tmpbuf[0];
//             tmpbuf[0] = nullptr;
//         }
//
//         if (tmpbuf[1] != nullptr)
//         {
//             delete[] tmpbuf[1];
//             tmpbuf[1] = nullptr;
//         }
    }

//     // NOTE it is assumed that mutex is locked
//     bool tryPutData(float* const out1,
//                     float* const out2,
//                     uint64_t framePos,
//                     const uint32_t frames,
//                     const bool loopingMode,
//                     const bool isOffline,
//                     bool& needsRead,
//                     uint64_t& needsReadFrame)
//     {
//         CARLA_SAFE_ASSERT_RETURN(numFrames != 0, false);
//         CARLA_SAFE_ASSERT_RETURN(maxFrame != 0, false);
//
//         if (framePos >= maxFrame)
//         {
//             if (loopingMode)
//                 framePos %= maxFrame;
//             else
//                 return false;
//         }
//
//         uint64_t frameDiff;
//         const uint32_t numFramesNearEnd = numFrames*3/4;
//
//         if (framePos < startFrame)
//         {
//             if (startFrame + numFrames <= maxFrame)
//             {
//                 needsRead = true;
//                 needsReadFrame = framePos;
//                 return false;
//             }
//
//             frameDiff = framePos + (maxFrame - startFrame);
//
//             if (frameDiff + frames >= numFrames)
//             {
//                 needsRead = true;
//                 needsReadFrame = framePos;
//                 return false;
//             }
//
//             carla_copyFloats(out1, buffer[0] + frameDiff, frames);
//             carla_copyFloats(out2, buffer[1] + frameDiff, frames);
//         }
//         else
//         {
//             frameDiff = framePos - startFrame;
//
//             if (frameDiff + frames >= numFrames)
//             {
//                 needsRead = true;
//                 needsReadFrame = framePos;
//                 return false;
//             }
//
//             carla_copyFloats(out1, buffer[0] + frameDiff, frames);
//             carla_copyFloats(out2, buffer[1] + frameDiff, frames);
//         }
//
//         if (frameDiff > numFramesNearEnd)
//         {
//             needsRead = true;
//             needsReadFrame = framePos + (isOffline ? 0 : frames);
//         }
//
//         return true;
//     }

    CARLA_DECLARE_NON_COPYABLE(AudioMemoryPool)
};

class AudioFileReader
{
public:
    AudioFileReader()
//         : fEntireFileLoaded(false),
//           fLoopingMode(true),
//           fNeedsFrame(0),
//           fNeedsRead(false),
//           fPollTempData(nullptr),
//           fPollTempSize(0),
//           fResampleTempData(nullptr),
//           fResampleTempSize(0),
//           fPool(),
//           fPoolMutex(),
//           fPoolReadyToSwap(false),
//           fResampler(),
//           fReaderMutex()
    {
        ad_clear_nfo(&fFileNfo);
    }

    ~AudioFileReader()
    {
        destroy();
    }

    void destroy()
    {
        const CarlaMutexLocker cml(fReaderMutex);

        cleanup();

//         fPool.destroy();
//         fNeedsFrame = 0;
//         fNeedsRead = false;
    }

//     bool isEntireFileLoaded() const noexcept
//     {
//         return fEntireFileLoaded;
//     }

    int getCurrentBitRate() const noexcept
    {
        return fCurrentBitRate;
    }

//     uint32_t getMaxFrame() const noexcept
//     {
//         return fPool.maxFrame;
//     }

    ADInfo getFileInfo() const noexcept
    {
        return fFileNfo;
    }

//     void setLoopingMode(const bool on) noexcept
//     {
//         fLoopingMode = on;
//     }

//     void setNeedsRead(const uint64_t frame) noexcept
//     {
//         if (fEntireFileLoaded)
//             return;
//
//         fNeedsFrame = frame;
//         fNeedsRead = true;
//     }

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

        // Fix misinformation from libsndfile
        while (fFileNfo.frames % fFileNfo.channels)
            --fFileNfo.frames;

        // invalid
        if ((fFileNfo.channels != 1 && fFileNfo.channels != 2) || fFileNfo.frames <= 0)
        {
            if (fFileNfo.channels != 1 && fFileNfo.channels != 2)
                carla_stderr("loadFilename(\"%s\", ...) has not 1 or 2 channels", filename);

            if (fFileNfo.frames <= 0)
                carla_stderr("loadFilename(\"%s\", ...) has 0 frames", filename);

            ad_clear_nfo(&fFileNfo);
            ad_close(fFilePtr);
            fFilePtr = nullptr;
            return false;
        }

        const uint32_t numFrames = static_cast<uint32_t>(fFileNfo.frames);
        const bool needsResample = fFileNfo.sample_rate != sampleRate;
        uint32_t numResampledFrames;

        if (needsResample)
        {
            if (! fResampler.setup(fFileNfo.sample_rate, sampleRate, fFileNfo.channels, 32))
            {
                ad_clear_nfo(&fFileNfo);
                ad_close(fFilePtr);
                fFilePtr = nullptr;
                carla_stderr2("loadFilename(\"%s\", ...) error, resampler setup failed");
                return false;
            }

            fResampleRatio = static_cast<double>(sampleRate) / static_cast<double>(fFileNfo.sample_rate);
            numResampledFrames = static_cast<uint32_t>(static_cast<double>(numFrames) * fResampleRatio + 0.5);
        }
        else
        {
            fResampler.clear();
            fResampleRatio = 0.0;
            numResampledFrames = numFrames;
        }

        // read the first 30s of the file if seekable
        const uint32_t initialFrames = fFileNfo.can_seek == 0
                                     ? numFrames
                                     : std::min(numFrames, fFileNfo.sample_rate * 600);
        const uint32_t initialResampledFrames = fFileNfo.can_seek == 0
                                              ? numResampledFrames
                                              : std::min(numResampledFrames, sampleRate * 600);
        fInitialMemoryPool.create(initialResampledFrames);
        readIntoInitialMemoryPool(initialFrames, initialResampledFrames);

//         if (initialResampledFrames == numResampledFrames)
        {
            // file is no longer needed, we have it all in memory
            ad_close(fFilePtr);
            fFilePtr = nullptr;

            const float resampledFramesF = static_cast<float>(initialResampledFrames);
            const float previewDataSizeF = static_cast<float>(previewDataSize);
            for (uint i=0; i<previewDataSize; ++i)
            {
                const float stepF = static_cast<float>(i)/previewDataSizeF * resampledFramesF;
                const uint step = carla_fixedValue(0U, initialResampledFrames-1U, static_cast<uint>(stepF + 0.5f));
                previewData[i] = std::max(std::fabs(fInitialMemoryPool.buffer[0][step]),
                                          std::fabs(fInitialMemoryPool.buffer[1][step]));
            }

            return true;
        }

            // file is too big, do disk streaming as needed
//                 const uint32_t poolNumFrames = sampleRate * 5;
//                 const uint pollTempSize = poolNumFrames * fFileNfo.channels;
//                 uint resampleTempSize = 0;
//
//                 readFilePreview(previewDataSize, previewData);
//
//                 fPool.create(poolNumFrames, maxFrame, true);
//
//                 try {
//                     fPollTempData = new float[pollTempSize];
//                 } catch (...) {
//                     ad_clear_nfo(&fFileNfo);
//                     ad_close(fFilePtr);
//                     fFilePtr = nullptr;
//                     carla_stderr2("loadFilename error, out of memory");
//                     return false;
//                 }
//
//                 CARLA_MLOCK(fPollTempData, sizeof(float)*pollTempSize);
//
//                 if (needsResample)
//                 {
//                     resampleTempSize = static_cast<uint32_t>(static_cast<double>(poolNumFrames) * fResampleRatio + 0.5);
//                     resampleTempSize *= fFileNfo.channels;
//
//                     try {
//                         fResampleTempData = new float[resampleTempSize];
//                     } catch (...) {
//                         delete[] fPollTempData;
//                         fPollTempData = nullptr;
//                         ad_clear_nfo(&fFileNfo);
//                         ad_close(fFilePtr);
//                         fFilePtr = nullptr;
//                         carla_stderr2("loadFilename error, out of memory");
//                         return false;
//                     }
//
//                     CARLA_MLOCK(fResampleTempData, sizeof(float)*resampleTempSize);
//                 }
//
//                 fPollTempSize = pollTempSize;
//                 fResampleTempSize = resampleTempSize;

//             fNeedsRead = true;
        return true;
    }

//     void createSwapablePool(AudioFilePool& pool)
//     {
//         pool.create(fPool.numFrames, fPool.maxFrame, false);
//     }

//     void putAndSwapAllData(AudioFilePool& pool)
//     {
//         const water::GenericScopedLock<water::SpinLock> gsl1(fPool.mutex);
//         const water::GenericScopedLock<water::SpinLock> gsl2(pool.mutex);
//         CARLA_SAFE_ASSERT_RETURN(fPool.numFrames != 0,);
//         CARLA_SAFE_ASSERT_RETURN(fPool.buffer[0] != nullptr,);
//         CARLA_SAFE_ASSERT_RETURN(fPool.tmpbuf[0] == nullptr,);
//         CARLA_SAFE_ASSERT_RETURN(pool.numFrames == 0,);
//         CARLA_SAFE_ASSERT_RETURN(pool.buffer[0] == nullptr,);
//         CARLA_SAFE_ASSERT_RETURN(pool.tmpbuf[0] == nullptr,);
//
//         pool.startFrame = fPool.startFrame;
//         pool.numFrames = fPool.numFrames;
//         pool.buffer[0] = fPool.buffer[0];
//         pool.buffer[1] = fPool.buffer[1];
//
//         fPool.startFrame = 0;
//         fPool.numFrames = 0;
//         fPool.buffer[0] = nullptr;
//         fPool.buffer[1] = nullptr;
//     }

//     bool tryPutData(AudioFilePool& pool,
//                     float* const out1,
//                     float* const out2,
//                     uint64_t framePos,
//                     const uint32_t frames,
//                     const bool loopMode,
//                     const bool isOffline,
//                     bool& needsIdleRequest)
//     {
//         _tryPoolSwap(pool);
//
//         bool needsRead = false;
//         uint64_t needsReadFrame;
//         const bool ret = pool.tryPutData(out1, out2, framePos, frames, loopMode, isOffline, needsRead, needsReadFrame);
//
//         if (needsRead)
//         {
//             needsIdleRequest = true;
//             setNeedsRead(needsReadFrame);
//         }
//
// #ifdef DEBUG_FILE_OPS
//         if (! ret) {
//             carla_stdout("tryPutData fail");
//         }
// #endif
//         return ret;
//     }

    float tickFrames(float* const buffers[],
                     const uint32_t bufferOffset,
                     const uint32_t frames,
                     uint64_t framePos,
                     const bool loopingMode, const bool isOffline)
    {
        float* const out1 = buffers[0] + bufferOffset;
        float* const out2 = buffers[1] + bufferOffset;
        float* const playCV = buffers[2] + bufferOffset;

        // entire file in memory, use memory pool directly
        if (true)
        {
            uint32_t framesAvailable, numPoolFrames;

            {
                const CarlaMutexTryLocker cmtl(fInitialMemoryPool.mutex, isOffline);

                numPoolFrames = fInitialMemoryPool.numFrames;

                if (numPoolFrames == 0 || ! cmtl.wasLocked())
                {
                    carla_zeroFloats(out1, frames);
                    carla_zeroFloats(out2, frames);
                    carla_zeroFloats(playCV, frames);
                    return 0.f;
                }

                if (loopingMode && framePos >= numPoolFrames)
                    framePos %= numPoolFrames;

                if (framePos >= numPoolFrames)
                {
                    carla_zeroFloats(out1, frames);
                    carla_zeroFloats(out2, frames);
                    carla_zeroFloats(playCV, frames);
                    return 0.f;
                }

                framesAvailable = std::min(frames, numPoolFrames - static_cast<uint32_t>(framePos));

                carla_copyFloats(out1, fInitialMemoryPool.buffer[0] + framePos, framesAvailable);
                carla_copyFloats(out2, fInitialMemoryPool.buffer[1] + framePos, framesAvailable);
                carla_fillFloatsWithSingleValue(playCV, 10.f, framesAvailable);
            }

            if (frames != framesAvailable)
            {
                tickFrames(buffers,
                           bufferOffset + framesAvailable,
                           frames - framesAvailable,
                           framePos + framesAvailable,
                           loopingMode, isOffline);
            }

            return static_cast<float>(framePos) / static_cast<float>(numPoolFrames);
        }

        return 0.f;
    }

//     void readFilePreview(uint32_t previewDataSize, float* previewData)
//     {
//         carla_zeroFloats(previewData, previewDataSize);
//
//         const uint fileNumFrames = static_cast<uint>(fFileNfo.frames);
//         const float fileNumFramesF = static_cast<float>(fileNumFrames);
//         const float previewDataSizeF = static_cast<float>(previewDataSize);
//         const uint samplesPerRun = fFileNfo.channels;
//         const uint maxSampleToRead = fileNumFrames - samplesPerRun;
//         CARLA_SAFE_ASSERT_INT_RETURN(samplesPerRun == 1 || samplesPerRun == 2, samplesPerRun,);
//         float tmp[2] = { 0.0f, 0.0f };
//
//         if (samplesPerRun == 2)
//             previewDataSize -= 1;
//
//         for (uint i=0; i<previewDataSize; ++i)
//         {
//             const float posF = static_cast<float>(i)/previewDataSizeF * fileNumFramesF;
//             const uint pos = carla_fixedValue(0U, maxSampleToRead, static_cast<uint>(posF));
//
//             ad_seek(fFilePtr, pos);
//             ad_read(fFilePtr, tmp, samplesPerRun);
//             previewData[i] = std::max(std::fabs(tmp[0]), std::fabs(tmp[1]));
//         }
//     }

//     void readPoll()
//     {
//         const CarlaMutexLocker cml(fReaderMutex);
//
//         if (fFileNfo.channels == 0 || fFilePtr == nullptr)
//         {
//             carla_debug("R: no song loaded");
//             fNeedsFrame = 0;
//             fNeedsRead = false;
//             return;
//         }
//         if (fPollTempData == nullptr)
//         {
//             carla_debug("R: nothing to poll");
//             fNeedsFrame = 0;
//             fNeedsRead = false;
//             return;
//         }
//
//         const uint32_t maxFrame = fPool.maxFrame;
//         uint64_t lastFrame = fNeedsFrame;
//         int64_t readFrameCheck;
//
//         if (lastFrame >= maxFrame)
//         {
//             if (fLoopingMode)
//             {
//                 const uint64_t readFrameCheckLoop = lastFrame % maxFrame;
//                 CARLA_SAFE_ASSERT_RETURN(readFrameCheckLoop < INT32_MAX,);
//
//                 carla_debug("R: transport out of bounds for loop");
//                 readFrameCheck = static_cast<int64_t>(readFrameCheckLoop);
//             }
//             else
//             {
//                 carla_debug("R: transport out of bounds");
//                 fNeedsFrame = 0;
//                 fNeedsRead = false;
//                 return;
//             }
//         }
//         else
//         {
//             CARLA_SAFE_ASSERT_RETURN(lastFrame < INT32_MAX,);
//             readFrameCheck = static_cast<int64_t>(lastFrame);
//         }
//
//         const int64_t readFrame = readFrameCheck;
//
//         // temp data buffer
//         carla_zeroFloats(fPollTempData, fPollTempSize);
//
//         {
// #if 0
//             const int32_t sampleRate = 44100;
//             carla_debug("R: poll data - reading at frame %li, time %li:%02li, lastFrame %li",
//                         readFrame, readFrame/sampleRate/60, (readFrame/sampleRate) % 60, lastFrame);
// #endif
//
//             const int64_t readFrameReal = carla_isNotZero(fResampleRatio)
//                                         ? static_cast<int64_t>(static_cast<double>(readFrame) / fResampleRatio + 0.5)
//                                         : readFrame;
//
//             ad_seek(fFilePtr, readFrameReal);
//             size_t i = 0;
//             ssize_t j = 0;
//             ssize_t rv = ad_read(fFilePtr, fPollTempData, fPollTempSize);
//
//             if (rv < 0)
//             {
//                 carla_stderr("R: ad_read1 failed");
//                 fNeedsFrame = 0;
//                 fNeedsRead = false;
//                 return;
//             }
//
//             const size_t urv = static_cast<size_t>(rv);
//
//             // see if we can read more
//             if (readFrameReal + rv >= static_cast<ssize_t>(fFileNfo.frames) && urv < fPollTempSize)
//             {
// #ifdef DEBUG_FILE_OPS
//                 carla_stdout("R: from start");
// #endif
//                 ad_seek(fFilePtr, 0);
//                 j = ad_read(fFilePtr, fPollTempData+urv, fPollTempSize-urv);
//
//                 if (j < 0)
//                 {
//                     carla_stderr("R: ad_read2 failed");
//                     fNeedsFrame = 0;
//                     fNeedsRead = false;
//                     return;
//                 }
//
//                 rv += j;
//             }
//
// #ifdef DEBUG_FILE_OPS
//             carla_stdout("R: reading %li frames at frame %lu", rv, readFrameCheck);
// #endif
//             fCurrentBitRate = ad_get_bitrate(fFilePtr);
//
//             // local copy
//             const uint32_t poolNumFrames = fPool.numFrames;
//             float* const pbuffer0 = fPool.tmpbuf[0];
//             float* const pbuffer1 = fPool.tmpbuf[1];
//             const float* tmpbuf = fPollTempData;
//
//             // resample as needed
//             if (fResampleTempSize != 0)
//             {
//                 tmpbuf = fResampleTempData;
//                 fResampler.inp_count = static_cast<uint>(rv / fFileNfo.channels);
//                 fResampler.out_count = fResampleTempSize / fFileNfo.channels;
//                 fResampler.inp_data = fPollTempData;
//                 fResampler.out_data = fResampleTempData;
//                 fResampler.process();
//                 CARLA_ASSERT_INT(fResampler.inp_count <= 1, fResampler.inp_count);
//             }
//
//             j = 0;
//             do {
//                 if (fFileNfo.channels == 1)
//                 {
//                     for (; i < poolNumFrames && j < rv; ++i, ++j)
//                         pbuffer0[i] = pbuffer1[i] = tmpbuf[j];
//                 }
//                 else
//                 {
//                     for (; i < poolNumFrames && j < rv; ++j)
//                     {
//                         if (j % 2 == 0)
//                         {
//                             pbuffer0[i] = tmpbuf[j];
//                         }
//                         else
//                         {
//                             pbuffer1[i] = tmpbuf[j];
//                             ++i;
//                         }
//                     }
//                 }
//
//                 if (i >= poolNumFrames)
//                     break;
//
//                 if (rv == fFileNfo.frames)
//                 {
//                     // full file read
//                     j = 0;
// #ifdef DEBUG_FILE_OPS
//                     carla_stdout("R: full file was read, filling buffers again");
// #endif
//                 }
//                 else
//                 {
// #ifdef DEBUG_FILE_OPS
//                     carla_stdout("read break, not enough space");
// #endif
//                     carla_zeroFloats(pbuffer0, poolNumFrames - i);
//                     carla_zeroFloats(pbuffer1, poolNumFrames - i);
//                     break;
//                 }
//
//             } while (i < poolNumFrames);
//
//             // lock, and put data asap
//             const CarlaMutexLocker cmlp(fPoolMutex);
//             const water::GenericScopedLock<water::SpinLock> gsl(fPool.mutex);
//
//             std::memcpy(fPool.buffer[0], pbuffer0, sizeof(float)*poolNumFrames);
//             std::memcpy(fPool.buffer[1], pbuffer1, sizeof(float)*poolNumFrames);
//             fPool.startFrame = static_cast<uint64_t>(readFrame);
//             fPoolReadyToSwap = true;
// #ifdef DEBUG_FILE_OPS
//             carla_stdout("Reading done and internal pool is now full");
// #endif
//         }
//
//         fNeedsRead = false;
//     }

private:
//     bool fEntireFileLoaded;
//     bool fLoopingMode;
    int fCurrentBitRate = 0;
//     volatile uint64_t fNeedsFrame;
//     volatile bool fNeedsRead;

    void*  fFilePtr = nullptr;
    ADInfo fFileNfo = {};

//     float* fPollTempData;
//     uint fPollTempSize;

    double fResampleRatio = 0.0;
//     float* fResampleTempData;
//     uint fResampleTempSize;

    AudioMemoryPool fInitialMemoryPool;
//     CarlaMutex    fPoolMutex;
//     bool          fPoolReadyToSwap;
    Resampler     fResampler;
    CarlaMutex    fReaderMutex;

    // assumes reader lock is active
    void cleanup()
    {
        fCurrentBitRate = 0;
//         fEntireFileLoaded = false;
        fInitialMemoryPool.destroy();

        if (fFilePtr != nullptr)
        {
            ad_close(fFilePtr);
            fFilePtr = nullptr;
        }

//         if (fPollTempData != nullptr)
//         {
//             delete[] fPollTempData;
//             fPollTempData = nullptr;
//             fPollTempSize = 0;
//         }
//
//         if (fResampleTempData != nullptr)
//         {
//             delete[] fResampleTempData;
//             fResampleTempData = nullptr;
//             fResampleTempSize = 0;
//         }
    }

    void readIntoInitialMemoryPool(const uint numFrames, const uint numResampledFrames)
    {
        const uint channels = fFileNfo.channels;
        const uint fileBufferSize = numFrames * channels;

        float* const fileBuffer = (float*)std::malloc(fileBufferSize * sizeof(float));
        CARLA_SAFE_ASSERT_RETURN(fileBuffer != nullptr,);

        ad_seek(fFilePtr, 0);
        ssize_t rv = ad_read(fFilePtr, fileBuffer, fileBufferSize);
        CARLA_SAFE_ASSERT_INT2_RETURN(rv == static_cast<ssize_t>(fileBufferSize),
                                      rv, fileBufferSize,
                                      std::free(fileBuffer));

        fCurrentBitRate = ad_get_bitrate(fFilePtr);

        float* resampledBuffer;

        if (numFrames != numResampledFrames)
        {
            const uint resampledBufferSize = numResampledFrames * channels;
            resampledBuffer = (float*)std::malloc(resampledBufferSize * sizeof(float));
            CARLA_SAFE_ASSERT_RETURN(resampledBuffer != nullptr, std::free(fileBuffer););

            fResampler.inp_count = numFrames;
            fResampler.out_count = resampledBufferSize;
            fResampler.inp_data = fileBuffer;
            fResampler.out_data = resampledBuffer;
            fResampler.process();
            CARLA_SAFE_ASSERT_UINT(fResampler.out_count == 0, fResampler.out_count);

            rv = static_cast<ssize_t>(resampledBufferSize - fResampler.out_count);
        }
        else
        {
            resampledBuffer = fileBuffer;
        }

        {
            // lock, and put data asap
            const CarlaMutexLocker cml(fInitialMemoryPool.mutex);

            if (channels == 1)
            {
                for (ssize_t i=0; i < rv; ++i)
                    fInitialMemoryPool.buffer[0][i] = fInitialMemoryPool.buffer[1][i] = resampledBuffer[i];
            }
            else
            {
                for (ssize_t i=0, j=0; i < rv; ++j)
                {
                    fInitialMemoryPool.buffer[0][j] = resampledBuffer[i++];
                    fInitialMemoryPool.buffer[1][j] = resampledBuffer[i++];
                }
            }

            fInitialMemoryPool.numFrames = numResampledFrames;
        }

        if (resampledBuffer != fileBuffer)
            std::free(resampledBuffer);

        std::free(fileBuffer);

//         fEntireFileLoaded = true;
    }

    // try a pool data swap if possible and relevant
    // NOTE it is assumed that `pool` mutex is locked
//     void _tryPoolSwap(AudioFilePool& pool)
//     {
//         uint32_t tmp_u32;
//         uint64_t tmp_u64;
//         float* tmp_fp;
//
//         const CarlaMutexTryLocker cmtl(fPoolMutex);
//
//         if (! cmtl.wasLocked())
//             return;
//
//         const water::GenericScopedLock<water::SpinLock> gsl(fPool.mutex);
//
//         if (! fPoolReadyToSwap)
//             return;
//
//         tmp_u64 = pool.startFrame;
//         pool.startFrame = fPool.startFrame;
//         fPool.startFrame = tmp_u64;
//
//         tmp_u32 = pool.numFrames;
//         pool.numFrames = fPool.numFrames;
//         fPool.numFrames = tmp_u32;
//
//         tmp_fp = pool.buffer[0];
//         pool.buffer[0] = fPool.buffer[0];
//         fPool.buffer[0] = tmp_fp;
//
//         tmp_fp = pool.buffer[1];
//         pool.buffer[1] = fPool.buffer[1];
//         fPool.buffer[1] = tmp_fp;
//
//         fPoolReadyToSwap = false;
//
// #ifdef DEBUG_FILE_OPS
//         carla_stdout("Pools have been swapped, internal one is now invalidated");
// #endif
//     }

    CARLA_DECLARE_NON_COPYABLE(AudioFileReader)
};

#endif // AUDIO_BASE_HPP_INCLUDED
