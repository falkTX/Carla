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
#include "CarlaMemUtils.hpp"
#include "CarlaRingBuffer.hpp"

extern "C" {
#include "audio_decoder/ad.h"
}

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Weffc++"
#endif

#include "zita-resampler/resampler.h"

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
# pragma GCC diagnostic pop
#endif

typedef struct adinfo ADInfo;

// --------------------------------------------------------------------------------------------------------------------
// tuning

// disk streaming buffer size
static constexpr const uint16_t kFileReaderBufferSize = 1024;

// if reading a file smaller than this, load it all in memory
static constexpr const uint16_t kMinLengthSeconds = 30;

// size of the audio file ring buffer
static constexpr const uint16_t kRingBufferLengthSeconds = 6;

static inline
constexpr float max4f(const float a, const float b, const float c, const float d) noexcept
{
    return a > b && a > c && a > d ? a :
           b > a && b > c && b > d ? b :
           c > a && c > b && c > d ? c :
           d;
}

// --------------------------------------------------------------------------------------------------------------------

struct AudioMemoryPool {
    float* buffer[2] = {};
    uint32_t numFrames = 0;
    CarlaMutex mutex;

    AudioMemoryPool() noexcept {}

    ~AudioMemoryPool() noexcept
    {
        destroy();
    }

    void create(const uint32_t desiredNumFrames)
    {
        CARLA_ASSERT(buffer[0] == nullptr);
        CARLA_ASSERT(buffer[1] == nullptr);
        CARLA_ASSERT(numFrames == 0);

        buffer[0] = new float[desiredNumFrames];
        buffer[1] = new float[desiredNumFrames];
        carla_mlock(buffer[0], sizeof(float)*desiredNumFrames);
        carla_mlock(buffer[1], sizeof(float)*desiredNumFrames);

        const CarlaMutexLocker cml(mutex);
        numFrames = desiredNumFrames;
    }

    void destroy() noexcept
    {
        {
            const CarlaMutexLocker cml(mutex);
            numFrames = 0;
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
    }

    CARLA_DECLARE_NON_COPYABLE(AudioMemoryPool)
};

// --------------------------------------------------------------------------------------------------------------------

class AudioFileReader
{
public:
    enum QuadMode {
        kQuad1and2,
        kQuad3and4,
        kQuadAll
    };

    AudioFileReader()
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
    }

    int getCurrentBitRate() const noexcept
    {
        return fCurrentBitRate;
    }

    float getLastPlayPosition() const noexcept
    {
        return fLastPlayPosition;
    }

    float getReadableBufferFill() const noexcept
    {
        if (fFileNfo.channels == 0)
            return 0.f;

        if (fEntireFileLoaded)
            return 1.f;

        return 1.f - (static_cast<float>(fRingBufferR.getReadableDataSize() / sizeof(float))
                     / static_cast<float>(fRingBufferR.getSize() / sizeof(float)));
    }

    ADInfo getFileInfo() const noexcept
    {
        return fFileNfo;
    }

    bool loadFilename(const char* const filename, const uint32_t sampleRate, const QuadMode quadMode,
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

        // invalid
        if ((fFileNfo.channels != 1 && fFileNfo.channels != 2 && fFileNfo.channels != 4) || fFileNfo.frames <= 0)
        {
            if (fFileNfo.channels != 1 && fFileNfo.channels != 2 && fFileNfo.channels != 4)
                carla_stderr("loadFilename(\"%s\", ...) has not 1, 2 or 4 channels", filename);

            if (fFileNfo.frames <= 0)
                carla_stderr("loadFilename(\"%s\", ...) has 0 frames", filename);

            ad_clear_nfo(&fFileNfo);
            ad_close(fFilePtr);
            fFilePtr = nullptr;
            return false;
        }

        const uint64_t numFileFrames = static_cast<uint64_t>(fFileNfo.frames);
        const bool needsResample = fFileNfo.sample_rate != sampleRate;
        uint64_t numResampledFrames;

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
            numResampledFrames = static_cast<uint64_t>(static_cast<double>(numFileFrames) * fResampleRatio + 0.5);

            if (fPreviousResampledBuffer.buffer == nullptr)
                fPreviousResampledBuffer.buffer = new float[kFileReaderBufferSize];
        }
        else
        {
            numResampledFrames = numFileFrames;
        }

        fQuadMode = quadMode;

        if (fFileNfo.can_seek == 0 || numResampledFrames <= sampleRate * kMinLengthSeconds)
        {
            // read and cache the first few seconds of the file if seekable
            const uint64_t initialFrames = fFileNfo.can_seek == 0
                                         ? numFileFrames
                                         : std::min<uint64_t>(numFileFrames, fFileNfo.sample_rate * kMinLengthSeconds);
            const uint64_t initialResampledFrames = fFileNfo.can_seek == 0
                                                  ? numResampledFrames
                                                  : std::min<uint64_t>(numResampledFrames,
                                                                       sampleRate * kMinLengthSeconds);

            fInitialMemoryPool.create(initialResampledFrames);
            readIntoInitialMemoryPool(initialFrames, initialResampledFrames);

            // file is no longer needed, we have it all in memory
            ad_close(fFilePtr);
            fFilePtr = nullptr;

            const float resampledFramesF = static_cast<float>(numResampledFrames);
            const float previewDataSizeF = static_cast<float>(previewDataSize);
            for (uint i=0; i<previewDataSize; ++i)
            {
                const float stepF = static_cast<float>(i)/previewDataSizeF * resampledFramesF;
                const uint step = carla_fixedValue<uint64_t>(0, numResampledFrames-1, static_cast<uint>(stepF + 0.5f));
                previewData[i] = std::max(std::fabs(fInitialMemoryPool.buffer[0][step]),
                                          std::fabs(fInitialMemoryPool.buffer[1][step]));
            }

            fEntireFileLoaded = true;
        }
        else
        {
            readFilePreview(previewDataSize, previewData);

            // cache only the first few initial seconds, let disk streaming handle the rest
            const uint64_t initialFrames = std::min<uint64_t>(numFileFrames,
                                                              fFileNfo.sample_rate * kRingBufferLengthSeconds / 2);
            const uint64_t initialResampledFrames = std::min<uint64_t>(numResampledFrames,
                                                                       sampleRate * kRingBufferLengthSeconds / 2);

            fRingBufferL.createBuffer(sampleRate * kRingBufferLengthSeconds * sizeof(float), true);
            fRingBufferR.createBuffer(sampleRate * kRingBufferLengthSeconds * sizeof(float), true);

            fInitialMemoryPool.create(initialResampledFrames);
            readIntoInitialMemoryPool(initialFrames, initialResampledFrames);

            fRingBufferL.writeCustomData(fInitialMemoryPool.buffer[0], fInitialMemoryPool.numFrames * sizeof(float));
            fRingBufferR.writeCustomData(fInitialMemoryPool.buffer[1], fInitialMemoryPool.numFrames * sizeof(float));
            fRingBufferL.commitWrite();
            fRingBufferR.commitWrite();

            fEntireFileLoaded = false;
        }

        fTotalResampledFrames = numResampledFrames;
        fSampleRate = sampleRate;

        return true;
    }

    bool tickFrames(float* const buffers[],
                    uint32_t bufferOffset, uint32_t frames, uint64_t framePos,
                    const bool loopingMode, const bool isOffline)
    {
        float* outL = buffers[0] + bufferOffset;
        float* outR = buffers[1] + bufferOffset;
        float* playCV = buffers[2] + bufferOffset;

        if (loopingMode && framePos >= fTotalResampledFrames)
            framePos %= fTotalResampledFrames;

        if (framePos >= fTotalResampledFrames)
        {
            carla_zeroFloats(outL, frames);
            carla_zeroFloats(outR, frames);
            carla_zeroFloats(playCV, frames);
            fLastPlayPosition = 1.f;
            return false;
        }

        uint32_t numPoolFrames, usableFrames;

        {
            const CarlaMutexTryLocker cmtl(fInitialMemoryPool.mutex, isOffline);

            numPoolFrames = fInitialMemoryPool.numFrames;

            if (numPoolFrames == 0 || ! cmtl.wasLocked())
            {
                carla_zeroFloats(outL, frames);
                carla_zeroFloats(outR, frames);
                carla_zeroFloats(playCV, frames);
                return false;
            }

            if (framePos < numPoolFrames)
            {
                usableFrames = std::min(frames, numPoolFrames - static_cast<uint32_t>(framePos));

                carla_copyFloats(outL, fInitialMemoryPool.buffer[0] + framePos, usableFrames);
                carla_copyFloats(outR, fInitialMemoryPool.buffer[1] + framePos, usableFrames);
                carla_fillFloatsWithSingleValue(playCV, 10.f, usableFrames);

                outL += usableFrames;
                outR += usableFrames;
                playCV += usableFrames;
                bufferOffset += usableFrames;
                framePos += usableFrames;
                frames -= usableFrames;
            }

            if (fEntireFileLoaded && frames != 0)
                return tickFrames(buffers, bufferOffset, frames, framePos, loopingMode, isOffline);
        }

        fLastPlayPosition = static_cast<float>(framePos / 64) / static_cast<float>(fTotalResampledFrames / 64);

        if (fEntireFileLoaded)
            return false;

        if (frames == 0)
        {
            // ring buffer is good, waiting for data reads
            if (fRingBufferFramePos == numPoolFrames)
                return false;

            // out of bounds, host likely has repositioned transport
            if (fRingBufferFramePos > numPoolFrames)
            {
                fNextFileReadPos = 0;
                return true;
            }

            // within bounds, skip frames until we reach the end of the memory pool
            const uint32_t framesUpToPoolEnd = numPoolFrames - fRingBufferFramePos;
            if (fRingBufferR.getReadableDataSize() / sizeof(float) >= framesUpToPoolEnd)
            {
                fRingBufferL.skipRead(framesUpToPoolEnd * sizeof(float));
                fRingBufferR.skipRead(framesUpToPoolEnd * sizeof(float));
                fRingBufferFramePos = numPoolFrames;
            }

            return true;
        }

        uint32_t totalFramesAvailable = fRingBufferR.getReadableDataSize() / sizeof(float);

        if (framePos != fRingBufferFramePos)
        {
            // unaligned position, see if we need to relocate too
            if (framePos < fRingBufferFramePos || framePos >= fRingBufferFramePos + totalFramesAvailable - frames)
            {
                carla_zeroFloats(outL, frames);
                carla_zeroFloats(outR, frames);
                carla_zeroFloats(playCV, frames);

                // wait until the previous relocation is done
                if (fNextFileReadPos == -1)
                    fNextFileReadPos = framePos - frames;

                return true;
            }

            // oh nice, we can skip a few frames and be in sync
            const uint32_t diffFrames = framePos - fRingBufferFramePos;
            fRingBufferL.skipRead(diffFrames * sizeof(float));
            fRingBufferR.skipRead(diffFrames * sizeof(float));
            totalFramesAvailable -= diffFrames;
            fRingBufferFramePos = framePos;
        }

        usableFrames = std::min<uint32_t>(frames, totalFramesAvailable);

        if (usableFrames == 0)
        {
            carla_zeroFloats(outL, frames);
            carla_zeroFloats(outR, frames);
            carla_zeroFloats(playCV, frames);
            return framePos < fTotalResampledFrames;
        }

        fRingBufferL.readCustomData(outL, usableFrames * sizeof(float));
        fRingBufferR.readCustomData(outR, usableFrames * sizeof(float));
        carla_fillFloatsWithSingleValue(playCV, 10.f, usableFrames);

        fRingBufferFramePos += usableFrames;
        totalFramesAvailable -= usableFrames;

        if (frames != usableFrames)
        {
            if (loopingMode)
            {
                bufferOffset += usableFrames;
                framePos += usableFrames;
                frames -= usableFrames;
                return tickFrames(buffers, bufferOffset, frames, framePos, loopingMode, isOffline);
            }

            carla_zeroFloats(outL + usableFrames, frames - usableFrames);
            carla_zeroFloats(outR + usableFrames, frames - usableFrames);
            carla_zeroFloats(playCV + usableFrames, frames - usableFrames);
        }

        return totalFramesAvailable <= fSampleRate * 2;
    }

    void readFilePreview(uint32_t previewDataSize, float* const previewData)
    {
        carla_zeroFloats(previewData, previewDataSize);

        if (fFileNfo.can_seek == 0)
            return;

        const uint fileNumFrames = static_cast<uint>(fFileNfo.frames);
        const float fileNumFramesF = static_cast<float>(fileNumFrames);
        const float previewDataSizeF = static_cast<float>(previewDataSize);
        const uint channels = fFileNfo.channels;
        const uint samplesPerRun = channels * 4;
        const uint maxSampleToRead = fileNumFrames - samplesPerRun;
        const uint8_t quadoffs = fQuadMode == kQuad3and4 ? 2 : 0;
        float tmp[16] = {};

        for (uint i=0; i<previewDataSize; ++i)
        {
            const float posF = static_cast<float>(i)/previewDataSizeF * fileNumFramesF;
            const uint pos = carla_fixedValue(0U, maxSampleToRead, static_cast<uint>(posF));

            ad_seek(fFilePtr, pos);
            ad_read(fFilePtr, tmp, samplesPerRun);

            switch (channels)
            {
            case 1:
                previewData[i] = max4f(std::fabs(tmp[0]),
                                       std::fabs(tmp[1]),
                                       std::fabs(tmp[2]),
                                       std::fabs(tmp[3]));
                break;
            case 2:
                previewData[i] = max4f(std::fabs(tmp[0]),
                                       std::fabs(tmp[2]),
                                       std::fabs(tmp[4]),
                                       std::fabs(tmp[6]));
                previewData[i] = std::max(previewData[i], max4f(std::fabs(tmp[1]),
                                                                std::fabs(tmp[3]),
                                                                std::fabs(tmp[5]),
                                                                std::fabs(tmp[7])));
                break;
            case 4:
                if (fQuadMode == kQuadAll)
                {
                    previewData[i] = max4f(std::fabs(tmp[0]) + std::fabs(tmp[4])
                                           + std::fabs(tmp[8]) + std::fabs(tmp[12]),
                                           std::fabs(tmp[1]) + std::fabs(tmp[5])
                                           + std::fabs(tmp[9]) + std::fabs(tmp[13]),
                                           std::fabs(tmp[2]) + std::fabs(tmp[6])
                                           + std::fabs(tmp[10]) + std::fabs(tmp[14]),
                                           std::fabs(tmp[3]) + std::fabs(tmp[7])
                                           + std::fabs(tmp[11]) + std::fabs(tmp[15]));
                }
                else
                {
                    previewData[i] = max4f(std::fabs(tmp[quadoffs+0]),
                                           std::fabs(tmp[quadoffs+4]),
                                           std::fabs(tmp[quadoffs+8]),
                                           std::fabs(tmp[quadoffs+12]));
                    previewData[i] = std::max(previewData[i], max4f(std::fabs(tmp[quadoffs+1]),
                                                                    std::fabs(tmp[quadoffs+5]),
                                                                    std::fabs(tmp[quadoffs+9]),
                                                                    std::fabs(tmp[quadoffs+13])));
                }
                break;
            }
        }
    }

    void readPoll()
    {
        const CarlaMutexLocker cml(fReaderMutex);

        const uint channels = fFileNfo.channels;

        if (channels == 0 || fFilePtr == nullptr)
        {
            carla_debug("R: no song loaded");
            return;
        }

        fCurrentBitRate = ad_get_bitrate(fFilePtr);

        const bool needsResample = carla_isNotEqual(fResampleRatio, 1.0);
        const uint8_t quadoffs = fQuadMode == kQuad3and4 ? 2 : 0;
        const int64_t nextFileReadPos = fNextFileReadPos;

        if (nextFileReadPos != -1)
        {
            fRingBufferL.flush();
            fRingBufferR.flush();

            fPreviousResampledBuffer.frames = 0;
            fRingBufferFramePos = nextFileReadPos;
            ad_seek(fFilePtr, nextFileReadPos / fResampleRatio);

            if (needsResample)
                fResampler.reset();
        }

        if (needsResample)
        {
            float buffer[kFileReaderBufferSize];
            float rbuffer[kFileReaderBufferSize];
            ssize_t r;
            uint prev_inp_count = 0;

            while (fRingBufferR.getWritableDataSize() >= sizeof(rbuffer))
            {
                if (const uint32_t oldframes = fPreviousResampledBuffer.frames)
                {
                    prev_inp_count = oldframes;
                    fPreviousResampledBuffer.frames = 0;
                    std::memcpy(buffer, fPreviousResampledBuffer.buffer, sizeof(float) * oldframes * channels);
                }
                else if (prev_inp_count != 0)
                {
                    std::memmove(buffer,
                                 buffer + (sizeof(buffer) / sizeof(float) - prev_inp_count * channels),
                                 sizeof(float) * prev_inp_count * channels);
                }

                r = ad_read(fFilePtr,
                            buffer + (prev_inp_count * channels),
                            sizeof(buffer) / sizeof(float) - (prev_inp_count * channels));

                if (r < 0)
                {
                    carla_stderr("R: ad_read failed");
                    break;
                }

                if (r == 0)
                    break;

                fResampler.inp_count = prev_inp_count + r / channels;
                fResampler.out_count = sizeof(rbuffer) / sizeof(float) / channels;
                fResampler.inp_data = buffer;
                fResampler.out_data = rbuffer;
                fResampler.process();

                r = sizeof(rbuffer) / sizeof(float) - fResampler.out_count * channels;

                if (fResampleRatio > 1.0)
                {
                    if (fResampler.out_count == 0)
                    {
                        CARLA_SAFE_ASSERT_UINT(fResampler.inp_count != 0, fResampler.inp_count);
                    }
                    else
                    {
                        CARLA_SAFE_ASSERT_UINT(fResampler.inp_count == 0, fResampler.inp_count);
                    }
                }
                else
                {
                    CARLA_SAFE_ASSERT(fResampler.inp_count == 0);
                }

                prev_inp_count = fResampler.inp_count;

                if (r == 0)
                    break;

                switch (channels)
                {
                case 1:
                    fRingBufferL.writeCustomData(rbuffer, r * sizeof(float));
                    fRingBufferR.writeCustomData(rbuffer, r * sizeof(float));
                    break;
                case 2:
                    for (ssize_t i=0; i < r;)
                    {
                        fRingBufferL.writeCustomData(&rbuffer[i++], sizeof(float));
                        fRingBufferR.writeCustomData(&rbuffer[i++], sizeof(float));
                    }
                    break;
                case 4:
                    if (fQuadMode == kQuadAll)
                    {
                        float v;
                        for (ssize_t i=0; i < r; i += 4)
                        {
                            v = rbuffer[i] + rbuffer[i+1] + rbuffer[i+2] + rbuffer[i+3];
                            fRingBufferL.writeCustomData(&v, sizeof(float));
                            fRingBufferR.writeCustomData(&v, sizeof(float));
                        }
                    }
                    else
                    {
                        for (ssize_t i=quadoffs; i < r; i += 4)
                        {
                            fRingBufferL.writeCustomData(&rbuffer[i], sizeof(float));
                            fRingBufferR.writeCustomData(&rbuffer[i+1], sizeof(float));
                        }
                    }
                    break;
                }

                fRingBufferL.commitWrite();
                fRingBufferR.commitWrite();
            }

            if (prev_inp_count != 0)
            {
                fPreviousResampledBuffer.frames = prev_inp_count;
                std::memcpy(fPreviousResampledBuffer.buffer,
                            buffer + (sizeof(buffer) / sizeof(float) - prev_inp_count * channels),
                            sizeof(float) * prev_inp_count * channels);
            }
        }
        else
        {
            float buffer[kFileReaderBufferSize];
            ssize_t r;

            while (fRingBufferR.getWritableDataSize() >= sizeof(buffer))
            {
                r = ad_read(fFilePtr, buffer, sizeof(buffer)/sizeof(float));

                if (r < 0)
                {
                    carla_stderr("R: ad_read failed");
                    break;
                }

                if (r == 0)
                    break;

                switch (channels)
                {
                case 1:
                    fRingBufferL.writeCustomData(buffer, r * sizeof(float));
                    fRingBufferR.writeCustomData(buffer, r * sizeof(float));
                    break;
                case 2:
                    for (ssize_t i=0; i < r;)
                    {
                        fRingBufferL.writeCustomData(&buffer[i++], sizeof(float));
                        fRingBufferR.writeCustomData(&buffer[i++], sizeof(float));
                    }
                    break;
                case 4:
                    if (fQuadMode == kQuadAll)
                    {
                        float v;
                        for (ssize_t i=0; i < r; i += 4)
                        {
                            v = buffer[i] + buffer[i+1] + buffer[i+2] + buffer[i+3];
                            fRingBufferL.writeCustomData(&v, sizeof(float));
                            fRingBufferR.writeCustomData(&v, sizeof(float));
                        }
                    }
                    else
                    {
                        for (ssize_t i=quadoffs; i < r; i += 4)
                        {
                            fRingBufferL.writeCustomData(&buffer[i], sizeof(float));
                            fRingBufferR.writeCustomData(&buffer[i+1], sizeof(float));
                        }
                    }
                    break;
                }

                fRingBufferL.commitWrite();
                fRingBufferR.commitWrite();
            }
        }

        if (nextFileReadPos != -1)
            fNextFileReadPos = -1;
    }

private:
    bool fEntireFileLoaded = false;
    QuadMode fQuadMode = kQuad1and2;
    int fCurrentBitRate = 0;
    float fLastPlayPosition = 0.f;
    int64_t fNextFileReadPos = -1;
    uint64_t fTotalResampledFrames = 0;

    void*  fFilePtr = nullptr;
    ADInfo fFileNfo = {};

    uint32_t fSampleRate = 0;
    double fResampleRatio = 1.0;

    AudioMemoryPool fInitialMemoryPool;
    Resampler     fResampler;
    CarlaMutex    fReaderMutex;

    struct PreviousResampledBuffer {
        float* buffer = nullptr;
        uint32_t frames = 0;
    } fPreviousResampledBuffer;

    CarlaHeapRingBuffer fRingBufferL, fRingBufferR;
    uint64_t fRingBufferFramePos = 0;

    // assumes reader lock is active
    void cleanup()
    {
        fEntireFileLoaded = false;
        fCurrentBitRate = 0;
        fLastPlayPosition = 0.f;
        fNextFileReadPos = -1;
        fTotalResampledFrames = 0;
        fSampleRate = 0;
        fRingBufferFramePos = 0;
        fResampleRatio = 1.0;

        fResampler.clear();
        fInitialMemoryPool.destroy();
        fRingBufferL.deleteBuffer();
        fRingBufferR.deleteBuffer();

        if (fFilePtr != nullptr)
        {
            ad_close(fFilePtr);
            fFilePtr = nullptr;
        }

        delete[] fPreviousResampledBuffer.buffer;
        fPreviousResampledBuffer.buffer = nullptr;
        fPreviousResampledBuffer.frames = 0;
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
            resampledBuffer = (float*)std::malloc(numResampledFrames * channels * sizeof(float));
            CARLA_SAFE_ASSERT_RETURN(resampledBuffer != nullptr, std::free(fileBuffer););

            fResampler.inp_count = numFrames;
            fResampler.out_count = numResampledFrames;
            fResampler.inp_data = fileBuffer;
            fResampler.out_data = resampledBuffer;
            fResampler.process();

            fInitialMemoryPool.numFrames = numResampledFrames - fResampler.out_count;
            rv = fInitialMemoryPool.numFrames * channels;
        }
        else
        {
            resampledBuffer = fileBuffer;
        }

        {
            // lock, and put data asap
            const CarlaMutexLocker cml(fInitialMemoryPool.mutex);

            switch (channels)
            {
            case 1:
                for (ssize_t i=0; i < rv; ++i)
                    fInitialMemoryPool.buffer[0][i] = fInitialMemoryPool.buffer[1][i] = resampledBuffer[i];
                break;
            case 2:
                for (ssize_t i=0, j=0; i < rv; ++j)
                {
                    fInitialMemoryPool.buffer[0][j] = resampledBuffer[i++];
                    fInitialMemoryPool.buffer[1][j] = resampledBuffer[i++];
                }
                break;
            case 4:
                if (fQuadMode == kQuadAll)
                {
                    for (ssize_t i=0, j=0; i < rv; ++j)
                    {
                        fInitialMemoryPool.buffer[0][j] = fInitialMemoryPool.buffer[1][j]
                            = resampledBuffer[i] + resampledBuffer[i+1] + resampledBuffer[i+2] + resampledBuffer[i+3];
                        i += 4;
                    }
                }
                else
                {
                    for (ssize_t i = fQuadMode == kQuad3and4 ? 2 : 0, j = 0; i < rv; ++j)
                    {
                        fInitialMemoryPool.buffer[0][j] = resampledBuffer[i];
                        fInitialMemoryPool.buffer[1][j] = resampledBuffer[i+1];
                        i += 4;
                    }
                }
                break;
            }
        }

        if (resampledBuffer != fileBuffer)
            std::free(resampledBuffer);

        std::free(fileBuffer);
    }

    CARLA_DECLARE_NON_COPYABLE(AudioFileReader)
};

// --------------------------------------------------------------------------------------------------------------------

#endif // AUDIO_BASE_HPP_INCLUDED
