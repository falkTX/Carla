/*
 * Carla Native Plugins
 * Copyright (C) 2013-2020 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaNativePrograms.hpp"
#include "CarlaString.hpp"

#include "audio-base.hpp"

static const char* const audiofilesWildcard =
#ifdef HAVE_SNDFILE
    "*.aif;*.aifc;*.aiff;*.au;*.bwf;*.flac;*.htk;*.iff;*.mat4;*.mat5;*.oga;*.ogg;"
    "*.paf;*.pvf;*.pvf5;*.sd2;*.sf;*.snd;*.svx;*.vcc;*.w64;*.wav;*.xi;"
#endif
#ifdef HAVE_FFMPEG
    "*.3g2;*.3gp;*.aac;*.ac3;*.amr;*.ape;*.mp2;*.mp3;*.mpc;*.wma;"
# ifndef HAVE_SNDFILE
    "*.flac;*.oga;*.ogg;*.w64;*.wav;"
# endif
#endif
#if !defined(HAVE_SNDFILE) && !defined(HAVE_FFMPEG)
    ""
# ifndef BUILDING_FOR_CI
#  warning sndfile and ffmpeg libraries missing, no audio file support will be available
# endif
#endif
;

// -----------------------------------------------------------------------

class AudioFilePlugin : public NativePluginWithMidiPrograms<FileAudio>
{
public:
#ifndef __MOD_DEVICES__
    typedef enum _PendingInlineDisplay
# ifdef CARLA_PROPER_CPP11_SUPPORT
    : uint8_t
# endif
    {
        InlineDisplayNotPending,
        InlineDisplayNeedRequest,
        InlineDisplayRequesting
    } PendingInlineDisplay;
#endif

    enum Parameters {
        kParameterLooping,
        kParameterInfoChannels,
        kParameterInfoBitRate,
        kParameterInfoBitDepth,
        kParameterInfoSampleRate,
        kParameterInfoLength,
        kParameterCount
    };

    AudioFilePlugin(const NativeHostDescriptor* const host)
        : NativePluginWithMidiPrograms<FileAudio>(host, fPrograms, 2),
          fLoopMode(true),
          fDoProcess(false),
          fWasPlayingBefore(false),
          fNeedsFileRead(false),
          fMaxFrame(0),
          fPool(),
          fReader(),
          fPrograms(hostGetFilePath("audio"), audiofilesWildcard)
#ifndef __MOD_DEVICES__
        , fInlineDisplay()
#endif
    {
    }

    ~AudioFilePlugin() override
    {
        fReader.reset();
        fPool.destroy();
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const override
    {
        return kParameterCount;
    }

    const NativeParameter* getParameterInfo(const uint32_t index) const override
    {
        static NativeParameter param;

        param.scalePointCount  = 0;
        param.scalePoints      = nullptr;
        param.unit             = nullptr;
        param.ranges.step      = 1.0f;
        param.ranges.stepSmall = 1.0f;
        param.ranges.stepLarge = 1.0f;

        switch (index)
        {
        case kParameterLooping:
            param.name  = "Loop Mode";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_BOOLEAN);
            param.ranges.def = 1.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 1.0f;
            break;
        case kParameterInfoChannels:
            param.name  = "Num Channels";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_INTEGER|
                                                            NATIVE_PARAMETER_IS_OUTPUT);
            param.ranges.def = 0.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 2.0f;
            break;
        case kParameterInfoBitRate:
            param.name  = "Bit Rate";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_INTEGER|
                                                            NATIVE_PARAMETER_IS_OUTPUT);
            param.ranges.def = 0.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 384000.0f * 32.0f * 2.0f;
            break;
        case kParameterInfoBitDepth:
            param.name  = "Bit Depth";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_INTEGER|
                                                            NATIVE_PARAMETER_IS_OUTPUT);
            param.ranges.def = 0.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 32.0f;
            break;
        case kParameterInfoSampleRate:
            param.name  = "Sample Rate";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_INTEGER|
                                                            NATIVE_PARAMETER_IS_OUTPUT);
            param.ranges.def = 0.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 384000.0f;
            break;
        case kParameterInfoLength:
            param.name  = "Length";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_OUTPUT);
            param.ranges.def = 0.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = (float)INT64_MAX;
            param.unit = "s";
            break;
        default:
            return nullptr;
        }

        return &param;
    }

    float getParameterValue(const uint32_t index) const override
    {
        if (index == kParameterLooping)
            return fLoopMode ? 1.0f : 0.0f;

        const ADInfo nfo = fReader.getFileInfo();

        switch (index)
        {
        case kParameterInfoChannels:
            return static_cast<float>(nfo.channels);
        case kParameterInfoBitRate:
            return static_cast<float>(nfo.bit_rate);
        case kParameterInfoBitDepth:
            return static_cast<float>(nfo.bit_depth);
        case kParameterInfoSampleRate:
            return static_cast<float>(nfo.sample_rate);
        case kParameterInfoLength:
            return static_cast<float>(nfo.length)/1000.0f;
        default:
            return 0.0f;
        }
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setParameterValue(const uint32_t index, const float value) override
    {
        if (index != kParameterLooping)
            return;

        bool b = (value > 0.5f);

        if (b == fLoopMode)
            return;

        fLoopMode = b;
        fReader.setLoopingMode(b);
    }

    void setCustomData(const char* const key, const char* const value) override
    {
        if (std::strcmp(key, "file") != 0)
            return;

        invalidateNextFilename();
        loadFilename(value);
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process2(const float* const*, float** const outBuffer, const uint32_t frames,
                  const NativeMidiEvent*, uint32_t) override
    {
        const NativeTimeInfo* const timePos(getTimeInfo());

        float* out1 = outBuffer[0];
        float* out2 = outBuffer[1];
        bool needsIdleRequest = false;

        if (! fDoProcess)
        {
            // carla_stderr("P: no process");
            carla_zeroFloats(out1, frames);
            carla_zeroFloats(out2, frames);
            return;
        }

        // not playing
        if (! timePos->playing)
        {
            // carla_stderr("P: not playing");
            if (timePos->frame == 0 && fWasPlayingBefore)
                fReader.setNeedsRead(timePos->frame);

            carla_zeroFloats(out1, frames);
            carla_zeroFloats(out2, frames);
            fWasPlayingBefore = false;
            return;
        }
        else
        {
            fWasPlayingBefore = true;
        }

        // out of reach
        if ((timePos->frame < fPool.startFrame || timePos->frame >= fMaxFrame) && !fLoopMode)
        {
            if (timePos->frame < fPool.startFrame)
            {
                needsIdleRequest = true;
                fNeedsFileRead = true;
                fReader.setNeedsRead(timePos->frame);
            }

            carla_zeroFloats(out1, frames);
            carla_zeroFloats(out2, frames);

#ifndef __MOD_DEVICES__
            if (fInlineDisplay.writtenValues < 32)
            {
                fInlineDisplay.lastValuesL[fInlineDisplay.writtenValues] = 0.0f;
                fInlineDisplay.lastValuesR[fInlineDisplay.writtenValues] = 0.0f;
                ++fInlineDisplay.writtenValues;
            }
            if (fInlineDisplay.pending == InlineDisplayNotPending)
            {
                needsIdleRequest = true;
                fInlineDisplay.pending = InlineDisplayNeedRequest;
            }
#endif

            if (needsIdleRequest)
                hostRequestIdle();

            return;
        }

        if (fReader.isEntireFileLoaded())
        {
            // NOTE: timePos->frame is always < fMaxFrame (or looping)
            uint32_t targetStartFrame = static_cast<uint32_t>(fLoopMode ? timePos->frame % fMaxFrame : timePos->frame);

            for (uint32_t framesDone=0, framesToDo=frames, remainingFrames; framesDone < frames;)
            {
                if (targetStartFrame + framesToDo <= fMaxFrame)
                {
                    // everything fits together
                    carla_copyFloats(out1+framesDone, fPool.buffer[0]+targetStartFrame, framesToDo);
                    carla_copyFloats(out2+framesDone, fPool.buffer[1]+targetStartFrame, framesToDo);
                    break;
                }

                remainingFrames = std::min(fMaxFrame - targetStartFrame, framesToDo);
                carla_copyFloats(out1+framesDone, fPool.buffer[0]+targetStartFrame, remainingFrames);
                carla_copyFloats(out2+framesDone, fPool.buffer[1]+targetStartFrame, remainingFrames);
                framesDone += remainingFrames;
                framesToDo -= remainingFrames;

                if (! fLoopMode)
                {
                    // not looping, stop here
                    if (framesToDo != 0)
                    {
                        carla_zeroFloats(out1+framesDone, framesToDo);
                        carla_zeroFloats(out2+framesDone, framesToDo);
                    }
                    break;
                }

                // reset for next loop
                targetStartFrame = 0;
            }
        }
        else
        {
            if (! fReader.tryPutData(out1, out2, timePos->frame, frames, needsIdleRequest))
            {
                carla_zeroFloats(out1, frames);
                carla_zeroFloats(out2, frames);
            }
            if (needsIdleRequest)
                fNeedsFileRead = true;
        }

#ifndef __MOD_DEVICES__
        if (fInlineDisplay.writtenValues < 32)
        {
            fInlineDisplay.lastValuesL[fInlineDisplay.writtenValues] = carla_findMaxNormalizedFloat(out1, frames);
            fInlineDisplay.lastValuesR[fInlineDisplay.writtenValues] = carla_findMaxNormalizedFloat(out2, frames);
            ++fInlineDisplay.writtenValues;
        }
        if (fInlineDisplay.pending == InlineDisplayNotPending)
        {
            needsIdleRequest = true;
            fInlineDisplay.pending = InlineDisplayNeedRequest;
        }
#endif

        if (needsIdleRequest)
            hostRequestIdle();
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

    // -------------------------------------------------------------------
    // Plugin state calls

    void setStateFromFile(const char* const filename) override
    {
        loadFilename(filename);
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher calls

    void idle() override
    {
        NativePluginWithMidiPrograms<FileAudio>::idle();

        if (fNeedsFileRead)
            fReader.readPoll();

#ifndef __MOD_DEVICES__
        if (fInlineDisplay.pending == InlineDisplayNeedRequest)
        {
            fInlineDisplay.pending = InlineDisplayRequesting;
            hostQueueDrawInlineDisplay();
        }
#endif
    }

#ifndef __MOD_DEVICES__
    const NativeInlineDisplayImageSurface* renderInlineDisplay(const uint32_t rwidth, const uint32_t height) override
    {
        CARLA_SAFE_ASSERT_RETURN(height > 4, nullptr);

        const uint32_t width = rwidth == height ? height * 4 : rwidth;

        /* NOTE the code is this function is not optimized, still learning my way through pixels...
         */
        const size_t stride = width * 4;
        const size_t dataSize = stride * height;
        const uint pxToMove = fDoProcess ? fInlineDisplay.writtenValues : 0;

        uchar* data = fInlineDisplay.data;

        if (fInlineDisplay.dataSize != dataSize || data == nullptr)
        {
            delete[] data;
            data = new uchar[dataSize];
            std::memset(data, 0, dataSize);
            fInlineDisplay.data = data;
            fInlineDisplay.dataSize = dataSize;
        }
        else if (pxToMove != 0)
        {
            // shift all previous values to the left
            for (uint w=0; w < width - pxToMove; ++w)
                for (uint h=0; h < height; ++h)
                    std::memmove(&data[h * stride + w * 4], &data[h * stride + (w+pxToMove) * 4], 4);
        }

        fInlineDisplay.width  = static_cast<int>(width);
        fInlineDisplay.height = static_cast<int>(height);
        fInlineDisplay.stride = static_cast<int>(stride);

        if (pxToMove != 0)
        {
            const uint h2 = height / 2;

            // clear current line
            for (uint w=width-pxToMove; w < width; ++w)
                for (uint h=0; h < height; ++h)
                    memset(&data[h * stride + w * 4], 0, 4);

            // draw upper/left
            for (uint i=0; i < pxToMove && i < 32; ++i)
            {
                const float valueL = fInlineDisplay.lastValuesL[i];
                const float valueR = fInlineDisplay.lastValuesR[i];

                const uint h2L = static_cast<uint>(valueL * (float)h2);
                const uint h2R = static_cast<uint>(valueR * (float)h2);
                const uint w   = width - pxToMove + i;

                for (uint h=0; h < h2L; ++h)
                {
                    // -30dB
                    //if (valueL < 0.032f)
                    //    continue;

                    data[(h2 - h) * stride + w * 4 + 3] = 160;

                    // -12dB
                    if (valueL < 0.25f)
                    {
                        data[(h2 - h) * stride + w * 4 + 1] = 255;
                    }
                    // -3dB
                    else if (valueL < 0.70f)
                    {
                        data[(h2 - h) * stride + w * 4 + 2] = 255;
                        data[(h2 - h) * stride + w * 4 + 1] = 255;
                    }
                    else
                    {
                        data[(h2 - h) * stride + w * 4 + 2] = 255;
                    }
                }

                for (uint h=0; h < h2R; ++h)
                {
                    // -30dB
                    //if (valueR < 0.032f)
                    //    continue;

                    data[(h2 + h) * stride + w * 4 + 3] = 160;

                    // -12dB
                    if (valueR < 0.25f)
                    {
                        data[(h2 + h) * stride + w * 4 + 1] = 255;
                    }
                    // -3dB
                    else if (valueR < 0.70f)
                    {
                        data[(h2 + h) * stride + w * 4 + 2] = 255;
                        data[(h2 + h) * stride + w * 4 + 1] = 255;
                    }
                    else
                    {
                        data[(h2 + h) * stride + w * 4 + 2] = 255;
                    }
                }
            }
        }

        fInlineDisplay.writtenValues = 0;
        fInlineDisplay.pending = InlineDisplayNotPending;
        return (NativeInlineDisplayImageSurface*)(NativeInlineDisplayImageSurfaceCompat*)&fInlineDisplay;
    }
#endif

    // -------------------------------------------------------------------

private:
    bool fLoopMode;
    bool fDoProcess;
    bool fWasPlayingBefore;
    bool fNeedsFileRead;

    uint32_t fMaxFrame;

    AudioFilePool   fPool;
    AudioFileReader fReader;

    NativeMidiPrograms fPrograms;

#ifndef __MOD_DEVICES__
    struct InlineDisplay : NativeInlineDisplayImageSurfaceCompat {
        float lastValuesL[32];
        float lastValuesR[32];
        volatile PendingInlineDisplay pending;
        volatile uint8_t writtenValues;

        InlineDisplay()
            : NativeInlineDisplayImageSurfaceCompat(),
# ifdef CARLA_PROPER_CPP11_SUPPORT
              lastValuesL{0.0f},
              lastValuesR{0.0f},
# endif
              pending(InlineDisplayNotPending),
              writtenValues(0)
        {
# ifndef CARLA_PROPER_CPP11_SUPPORT
            carla_zeroFloats(lastValuesL, 32);
            carla_zeroFloats(lastValuesR, 32);
# endif
        }

        ~InlineDisplay()
        {
            if (data != nullptr)
            {
                delete[] data;
                data = nullptr;
            }
        }

        CARLA_DECLARE_NON_COPY_STRUCT(InlineDisplay)
        CARLA_PREVENT_HEAP_ALLOCATION
    } fInlineDisplay;
#endif

    void loadFilename(const char* const filename)
    {
        CARLA_ASSERT(filename != nullptr);
        carla_debug("AudioFilePlugin::loadFilename(\"%s\")", filename);

        fReader.reset();
        fPool.destroy();

        if (filename == nullptr || *filename == '\0')
        {
            fDoProcess = false;
            fMaxFrame = 0;
            return;
        }

        if (fReader.loadFilename(filename, static_cast<uint32_t>(getSampleRate())))
        {
            fPool.create(fReader.getPoolNumFrames(), false);
            fMaxFrame = fReader.getMaxFrame();

            if (fReader.isEntireFileLoaded())
                fReader.putAllData(fPool);
            else
                fReader.readPoll();

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

static const NativePluginDescriptor audiofileDesc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_UI
#ifndef __MOD_DEVICES__
                                                  |NATIVE_PLUGIN_HAS_INLINE_DISPLAY
#endif
                                                  |NATIVE_PLUGIN_REQUESTS_IDLE
                                                  |NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE
                                                  |NATIVE_PLUGIN_USES_TIME),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
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

CARLA_EXPORT
void carla_register_native_plugin_audiofile();

CARLA_EXPORT
void carla_register_native_plugin_audiofile()
{
    carla_register_native_plugin(&audiofileDesc);
}

// -----------------------------------------------------------------------
