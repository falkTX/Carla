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

class AudioFilePlugin : public NativePluginWithMidiPrograms<FileAudio>,
                        public AbstractAudioPlayer
{
public:
    AudioFilePlugin(const NativeHostDescriptor* const host)
        : NativePluginWithMidiPrograms<FileAudio>(host, fPrograms, 2),
          AbstractAudioPlayer(),
          fLoopMode(true),
          fDoProcess(false),
          fLastFrame(0),
          fMaxFrame(0),
          fPool(),
          fThread(this),
          fPrograms(hostGetFilePath("audio"), audiofilesWildcard),
          fInlineDisplay() {}

    ~AudioFilePlugin() override
    {
        fThread.stopNow();
        fPool.destroy();
    }

    uint64_t getLastFrame() const override
    {
        return fLastFrame;
    }

protected:
    // -------------------------------------------------------------------
    // Plugin parameter calls

    uint32_t getParameterCount() const override
    {
        return 1;
    }

    const NativeParameter* getParameterInfo(const uint32_t index) const override
    {
        if (index != 0)
            return nullptr;

        static NativeParameter param;

        param.name  = "Loop Mode";
        param.unit  = nullptr;
        param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMABLE|NATIVE_PARAMETER_IS_ENABLED|NATIVE_PARAMETER_IS_BOOLEAN);
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
        fThread.setLoopingMode(b);
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

    void process2(const float**, float** const outBuffer, const uint32_t frames,
                  const NativeMidiEvent*, uint32_t) override
    {
        const NativeTimeInfo* const timePos(getTimeInfo());

        float* out1 = outBuffer[0];
        float* out2 = outBuffer[1];

        if (! fDoProcess)
        {
            //carla_stderr("P: no process");
            fLastFrame = timePos->frame;
            carla_zeroFloats(out1, frames);
            carla_zeroFloats(out2, frames);
            return;
        }

        // not playing
        if (! timePos->playing)
        {
            //carla_stderr("P: not playing");
            if (timePos->frame == 0 && fLastFrame > 0)
                fThread.setNeedsRead();

            fLastFrame = timePos->frame;
            carla_zeroFloats(out1, frames);
            carla_zeroFloats(out2, frames);
            return;
        }

        // out of reach
        if ((timePos->frame < fPool.startFrame || timePos->frame >= fMaxFrame) && !fLoopMode)
        {
            if (timePos->frame < fPool.startFrame)
                fThread.setNeedsRead();

            fLastFrame = timePos->frame;
            carla_zeroFloats(out1, frames);
            carla_zeroFloats(out2, frames);

            if (fInlineDisplay.writtenValues < 32)
            {
                fInlineDisplay.lastValuesL[fInlineDisplay.writtenValues] = 0.0f;
                fInlineDisplay.lastValuesR[fInlineDisplay.writtenValues] = 0.0f;
                ++fInlineDisplay.writtenValues;
            }

            if (! fInlineDisplay.pending)
            {
                fInlineDisplay.pending = true;
                hostQueueDrawInlineDisplay();
            }
            return;
        }

        if (fThread.isEntireFileLoaded())
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
            // NOTE: timePos->frame is always >= fPool.startFrame
            const uint64_t poolStartFrame = timePos->frame - fThread.getPoolStartFrame();

            if (fThread.tryPutData(fPool, poolStartFrame, frames))
            {
                carla_copyFloats(out1, fPool.buffer[0]+poolStartFrame, frames);
                carla_copyFloats(out2, fPool.buffer[1]+poolStartFrame, frames);
            }
            else
            {
                carla_zeroFloats(out1, frames);
                carla_zeroFloats(out2, frames);
            }
        }

        if (fInlineDisplay.writtenValues < 32)
        {
            fInlineDisplay.lastValuesL[fInlineDisplay.writtenValues] = carla_findMaxNormalizedFloat(out1, frames);
            fInlineDisplay.lastValuesR[fInlineDisplay.writtenValues] = carla_findMaxNormalizedFloat(out2, frames);
            ++fInlineDisplay.writtenValues;
        }

        if (! fInlineDisplay.pending)
        {
            fInlineDisplay.pending = true;
            hostQueueDrawInlineDisplay();
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

    // -------------------------------------------------------------------
    // Plugin state calls

    void setStateFromFile(const char* const filename) override
    {
        loadFilename(filename);
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher calls

    const NativeInlineDisplayImageSurface* renderInlineDisplay(const uint32_t width, const uint32_t height) override
    {
        CARLA_SAFE_ASSERT_RETURN(width > 0 && height > 0, nullptr);

        /* NOTE the code is this function is not optimized, still learning my way through pixels...
         */
        const size_t stride = width * 4;
        const size_t dataSize = stride * height;
        const uint pxToMove = fInlineDisplay.writtenValues;

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

        const uint h2 = height / 2;

        // clear current line
        for (uint w=width-pxToMove; w < width; ++w)
            for (uint h=0; h < height; ++h)
                memset(&data[h * stride + w * 4], 0, 4);

        // draw upper/left
        for (uint i=0; i < pxToMove; ++i)
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

        fInlineDisplay.writtenValues = 0;
        fInlineDisplay.pending = false;
        return (NativeInlineDisplayImageSurface*)(NativeInlineDisplayImageSurfaceCompat*)&fInlineDisplay;
    }

    // -------------------------------------------------------------------

private:
    bool fLoopMode;
    bool fDoProcess;

    volatile uint64_t fLastFrame;
    uint32_t fMaxFrame;

    AudioFilePool   fPool;
    AudioFileThread fThread;

    NativeMidiPrograms fPrograms;

    struct InlineDisplay : NativeInlineDisplayImageSurfaceCompat {
        float lastValuesL[32];
        float lastValuesR[32];
        volatile uint8_t writtenValues;
        volatile bool pending;

        InlineDisplay()
            : NativeInlineDisplayImageSurfaceCompat(),
#ifdef CARLA_PROPER_CPP11_SUPPORT
              lastValuesL{0.0f},
              lastValuesR{0.0f},
#endif
              writtenValues(0),
              pending(false)
        {
#ifndef CARLA_PROPER_CPP11_SUPPORT
            carla_zeroFloats(lastValuesL, 32);
            carla_zeroFloats(lastValuesR, 32);
#endif
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

    void loadFilename(const char* const filename)
    {
        CARLA_ASSERT(filename != nullptr);
        carla_debug("AudioFilePlugin::loadFilename(\"%s\")", filename);

        fThread.stopNow();
        fPool.destroy();

        if (filename == nullptr || *filename == '\0')
        {
            fDoProcess = false;
            fMaxFrame = 0;
            return;
        }

        if (fThread.loadFilename(filename, static_cast<uint32_t>(getSampleRate())))
        {
            fPool.create(fThread.getPoolNumFrames());
            fMaxFrame = fThread.getMaxFrame();

            if (fThread.isEntireFileLoaded())
                fThread.putAllData(fPool);
            else
                fThread.startNow();

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
                                                  |NATIVE_PLUGIN_HAS_INLINE_DISPLAY
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE
                                                  |NATIVE_PLUGIN_REQUESTS_IDLE
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
