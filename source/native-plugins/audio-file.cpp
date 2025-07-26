// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaNativePrograms.hpp"

#include "audio-base.hpp"

#include <cmath>

// --------------------------------------------------------------------------------------------------------------------

class VolumeFilter
{
    static constexpr const float kPIf = static_cast<float>(M_PI);

    float a0, b1, z1;

public:
    VolumeFilter(const float sampleRate) noexcept
    {
        setSampleRate(sampleRate);
    }

    void reset() noexcept
    {
        a0 = 1.f - b1;
        z1 = 0.f;
    }

    void setSampleRate(const float sampleRate) noexcept
    {
        const float frequency = 30.0f / sampleRate;

        b1 = std::exp(-2.f * kPIf * frequency);
        a0 = 1.f - b1;
        z1 = 0.f;
    }

    void processStereo(const float gain, float* buffers[2], const uint32_t frames) noexcept
    {
        const float _a0 = a0;
        const float _b1 = b1;
        float _z1 = z1;

        for (uint32_t i=0; i < frames; ++i)
        {
            _z1 = gain * _a0 + _z1 * _b1;
            buffers[0][i] *= _z1;
            buffers[1][i] *= _z1;
        }

        z1 = _z1;
    }
};

// --------------------------------------------------------------------------------------------------------------------


#ifndef __MOD_DEVICES__
class AudioFilePlugin : public NativePluginWithMidiPrograms<FileAudio>
#else
class AudioFilePlugin : public NativePluginClass
#endif
{
public:
   #ifndef __MOD_DEVICES__
    static constexpr const char* const audiofilesWildcard =
      #ifdef HAVE_SNDFILE
        "*.aif;*.aifc;*.aiff;*.au;*.bwf;*.flac;*.htk;*.iff;*.mat4;*.mat5;*.oga;*.ogg;*.opus;"
        "*.paf;*.pvf;*.pvf5;*.sd2;*.sf;*.snd;*.svx;*.vcc;*.w64;*.wav;*.xi;"
      #endif
      #ifdef HAVE_FFMPEG
        "*.3g2;*.3gp;*.aac;*.ac3;*.amr;*.ape;*.mp2;*.mp3;*.mpc;*.wma;"
       #ifndef HAVE_SNDFILE
        "*.flac;*.oga;*.ogg;*.w64;*.wav;"
       #endif
      #else
        "*.mp3;"
      #endif
    ;

    enum PendingInlineDisplay : uint8_t {
        InlineDisplayNotPending,
        InlineDisplayNeedRequest,
        InlineDisplayRequesting
    };
   #endif

    enum Parameters {
        kParameterLooping,
        kParameterHostSync,
        kParameterVolume,
        kParameterEnabled,
        kParameterQuadChannels,
        kParameterInfoChannels,
        kParameterInfoBitRate,
        kParameterInfoBitDepth,
        kParameterInfoSampleRate,
        kParameterInfoLength,
        kParameterInfoPosition,
        kParameterInfoPoolFill,
        kParameterCount
    };

    AudioFilePlugin(const NativeHostDescriptor* const host)
       #ifndef __MOD_DEVICES__
        : NativePluginWithMidiPrograms<FileAudio>(host, fPrograms, 3),
          fPrograms(hostGetFilePath("audio"), audiofilesWildcard),
       #else
        : NativePluginClass(host),
       #endif
          fVolumeFilter(getSampleRate()) {}

protected:
    // ----------------------------------------------------------------------------------------------------------------
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
        param.designation      = NATIVE_PARAMETER_DESIGNATION_NONE;

        switch (index)
        {
        case kParameterLooping:
            param.name  = "Loop Mode";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMATABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_BOOLEAN);
            param.ranges.def = 1.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 1.0f;
            break;
        case kParameterHostSync:
            param.name  = "Host Sync";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMATABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_BOOLEAN);
           #ifdef __MOD_DEVICES__
            param.ranges.def = 0.0f;
           #else
            param.ranges.def = 1.0f;
           #endif
            param.ranges.min = 0.0f;
            param.ranges.max = 1.0f;
            break;
        case kParameterVolume:
            param.name  = "Volume";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMATABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED);
            param.ranges.def = 100.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 127.0f;
            param.ranges.stepSmall = 0.5f;
            param.ranges.stepLarge = 10.0f;
            param.unit = "%";
            break;
        case kParameterEnabled:
            param.name  = "Enabled";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMATABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_BOOLEAN|
                                                            NATIVE_PARAMETER_USES_DESIGNATION);
            param.ranges.def = 1.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 1.0f;
            param.designation = NATIVE_PARAMETER_DESIGNATION_ENABLED;
            break;
        case kParameterQuadChannels:
            param.name  = "Quad Channels";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMATABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_INTEGER|
                                                            NATIVE_PARAMETER_USES_SCALEPOINTS);
            param.ranges.def = 0.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 2.0f;
            {
                static const NativeParameterScalePoint scalePoints[3] = {
                    { "Channels 1 + 2", 0 },
                    { "Channels 3 + 4", 1 },
                    { "Channels 1&2 + 3&4", 2 }
                };
                param.scalePointCount  = 3;
                param.scalePoints      = scalePoints;
            }
            break;
        case kParameterInfoChannels:
            param.name  = "Num Channels";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMATABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_INTEGER|
                                                            NATIVE_PARAMETER_IS_OUTPUT);
            param.ranges.def = 0.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 2.0f;
            break;
        case kParameterInfoBitRate:
            param.name  = "Bit Rate";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMATABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_INTEGER|
                                                            NATIVE_PARAMETER_IS_OUTPUT);
            param.ranges.def = 0.0f;
            param.ranges.min = -1.0f;
            param.ranges.max = 384000.0f * 64.0f * 2.0f;
            break;
        case kParameterInfoBitDepth:
            param.name  = "Bit Depth";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMATABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_INTEGER|
                                                            NATIVE_PARAMETER_IS_OUTPUT);
            param.ranges.def = 0.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 64.0f;
            break;
        case kParameterInfoSampleRate:
            param.name  = "Sample Rate";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMATABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_INTEGER|
                                                            NATIVE_PARAMETER_IS_OUTPUT);
            param.ranges.def = 0.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 384000.0f;
            break;
        case kParameterInfoLength:
            param.name  = "Length";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMATABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_OUTPUT);
            param.ranges.def = 0.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = (float)INT64_MAX;
            param.unit = "s";
            break;
        case kParameterInfoPosition:
            param.name  = "Position";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMATABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_OUTPUT);
            param.ranges.def = 0.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 100.0f;
            param.unit = "%";
            break;
        case kParameterInfoPoolFill:
            param.name  = "Pool Fill";
            param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_AUTOMATABLE|
                                                            NATIVE_PARAMETER_IS_ENABLED|
                                                            NATIVE_PARAMETER_IS_OUTPUT);
            param.ranges.def = 0.0f;
            param.ranges.min = 0.0f;
            param.ranges.max = 100.0f;
            param.unit = "%";
            break;
        default:
            return nullptr;
        }

        return &param;
    }

    float getParameterValue(const uint32_t index) const override
    {
        switch (index)
        {
        case kParameterLooping:
            return fLoopMode ? 1.0f : 0.0f;
        case kParameterHostSync:
            return fHostSync ? 1.f : 0.f;
        case kParameterEnabled:
            return fEnabled ? 1.f : 0.f;
        case kParameterQuadChannels:
            return fQuadMode;
        case kParameterVolume:
            return fVolume * 100.f;
        case kParameterInfoPosition:
            return fLastPosition;
        case kParameterInfoPoolFill:
            return fReadableBufferFill;
        case kParameterInfoBitRate:
            return static_cast<float>(fReader.getCurrentBitRate());
        }

        const ADInfo nfo = fReader.getFileInfo();

        switch (index)
        {
        case kParameterInfoChannels:
            return static_cast<float>(nfo.channels);
        case kParameterInfoBitDepth:
            return static_cast<float>(nfo.bit_depth);
        case kParameterInfoSampleRate:
            return static_cast<float>(nfo.sample_rate);
        case kParameterInfoLength:
            return static_cast<float>(nfo.length)/1000.f;
        }

        return 0.f;
    }

    void setParameterValue(const uint32_t index, const float value) override
    {
        if (index == kParameterVolume)
        {
            fVolume = value / 100.f;
            return;
        }

        if (index == kParameterQuadChannels)
        {
            const int ivalue = static_cast<int>(value + 0.5f);
            CARLA_SAFE_ASSERT_INT_RETURN(value >= AudioFileReader::kQuad1and2 && value <= AudioFileReader::kQuadAll,
                                         value,);

            fQuadMode = static_cast<AudioFileReader::QuadMode>(ivalue);
            fPendingFileReload = true;
            hostRequestIdle();
            return;
        }

        const bool b = value > 0.5f;

        switch (index)
        {
        case kParameterLooping:
            if (fLoopMode != b)
                fLoopMode = b;
            break;
        case kParameterHostSync:
            if (fHostSync != b)
            {
                fInternalTransportFrame = 0;
                fHostSync = b;
            }
            break;
        case kParameterEnabled:
            if (fEnabled != b)
            {
                fInternalTransportFrame = 0;
                fEnabled = b;
            }
            break;
        default:
            break;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Plugin state calls

    void setCustomData(const char* const key, const char* const value) override
    {
        if (std::strcmp(key, "file") != 0)
            return;

       #ifndef __MOD_DEVICES__
        invalidateNextFilename();
       #endif
        loadFilename(value);
    }

   #ifndef __MOD_DEVICES__
    void setStateFromFile(const char* const filename) override
    {
        loadFilename(filename);
    }
   #endif

    // ----------------------------------------------------------------------------------------------------------------
    // Plugin process calls

   #ifndef __MOD_DEVICES__
    void process2(const float* const*, float** const outBuffer, const uint32_t frames,
                  const NativeMidiEvent*, uint32_t) override
   #else
    void process(const float* const*, float** const outBuffer, const uint32_t frames,
                 const NativeMidiEvent*, uint32_t) override
   #endif
    {
        float* const out1 = outBuffer[0];
        float* const out2 = outBuffer[1];
        float* const playCV = outBuffer[2];

        if (! fDoProcess)
        {
            // carla_stderr("P: no process");
            carla_zeroFloats(out1, frames);
            carla_zeroFloats(out2, frames);
            carla_zeroFloats(playCV, frames);
            fLastPosition = 0.f;
            fReadableBufferFill = 0.f;
            return;
        }

        bool playing;
        uint64_t framePos;

        if (fHostSync)
        {
            const NativeTimeInfo* const timePos = getTimeInfo();
            playing = fEnabled && timePos->playing;
            framePos = timePos->frame;
        }
        else
        {
            playing = fEnabled;
            framePos = fInternalTransportFrame;

            if (playing)
                fInternalTransportFrame += frames;
        }

        // not playing
        if (! playing)
        {
            carla_zeroFloats(out1, frames);
            carla_zeroFloats(out2, frames);
            carla_zeroFloats(playCV, frames);
            return;
        }

        const bool offline = isOffline();
        bool needsIdleRequest = false;

        if (fReader.tickFrames(outBuffer, 0, frames, framePos, fLoopMode, offline) && ! fPendingFileRead)
        {
            if (offline)
            {
                fPendingFileRead = false;
                fReader.readPoll();
            }
            else
            {
                fPendingFileRead = true;
                needsIdleRequest = true;
            }
        }

        fLastPosition = fReader.getLastPlayPosition() * 100.f;
        fReadableBufferFill = fReader.getReadableBufferFill() * 100.f;

        fVolumeFilter.processStereo(fVolume, outBuffer, frames);

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

    // ----------------------------------------------------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        if (! show)
            return;

        if (const char* const filename = uiOpenFile(false, "Open Audio File", ""))
            uiCustomDataChanged("file", filename);

        uiClosed();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Plugin dispatcher calls

    void idle() override
    {
       #ifndef __MOD_DEVICES__
        NativePluginWithMidiPrograms<FileAudio>::idle();

        if (fInlineDisplay.pending == InlineDisplayNeedRequest)
        {
            fInlineDisplay.pending = InlineDisplayRequesting;
            hostQueueDrawInlineDisplay();
        }
       #endif

        if (fPendingFileReload)
        {
            fPendingFileReload = fPendingFileRead = false;

            if (char* const filename = fFilename.getAndReleaseBuffer())
            {
                loadFilename(filename);
                std::free(filename);
            }
        }
        else if (fPendingFileRead)
        {
            fPendingFileRead = false;
            fReader.readPoll();
        }
    }

    void sampleRateChanged(const double sampleRate) override
    {
        fVolumeFilter.setSampleRate(sampleRate);

        if (char* const filename = fFilename.getAndReleaseBuffer())
        {
            loadFilename(filename);
            std::free(filename);
        }
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

    // ----------------------------------------------------------------------------------------------------------------

private:
    bool fLoopMode = true;
   #ifdef __MOD_DEVICES__
    bool fHostSync = false;
   #else
    bool fHostSync = true;
   #endif
    bool fEnabled = true;
    bool fDoProcess = false;
    bool fPendingFileRead = false;
    bool fPendingFileReload = false;
    AudioFileReader::QuadMode fQuadMode = AudioFileReader::kQuad1and2;

    uint32_t fInternalTransportFrame = 0;
    float fLastPosition = 0.f;
    float fReadableBufferFill = 0.f;
    float fVolume = 1.f;

    AudioFileReader fReader;
    String fFilename;

    float fPreviewData[108] = {};

   #ifndef __MOD_DEVICES__
    NativeMidiPrograms fPrograms;

    struct InlineDisplay : NativeInlineDisplayImageSurfaceCompat {
        float lastValuesL[32] = {};
        float lastValuesR[32] = {};
        volatile PendingInlineDisplay pending = InlineDisplayNotPending;
        volatile uint8_t writtenValues = 0;

        InlineDisplay()
            : NativeInlineDisplayImageSurfaceCompat() {}

        ~InlineDisplay()
        {
            if (data != nullptr)
            {
                delete[] data;
                data = nullptr;
            }
        }

        CARLA_DECLARE_NON_COPYABLE(InlineDisplay)
        CARLA_PREVENT_HEAP_ALLOCATION
    } fInlineDisplay;
   #endif

    VolumeFilter fVolumeFilter;

    void loadFilename(const char* const filename)
    {
        CARLA_ASSERT(filename != nullptr);
        carla_stdout("AudioFilePlugin::loadFilename(\"%s\")", filename);

        fDoProcess = false;
        fReader.destroy();
        fFilename.clear();

        if (filename == nullptr || *filename == '\0')
            return;

        constexpr uint32_t kPreviewDataLen = sizeof(fPreviewData)/sizeof(float);

        if (fReader.loadFilename(filename, static_cast<uint32_t>(getSampleRate()), fQuadMode,
                                 kPreviewDataLen, fPreviewData))
        {
            fInternalTransportFrame = 0;
            fDoProcess = true;
            fFilename = filename;
            hostSendPreviewBufferData('f', kPreviewDataLen, fPreviewData);
        }
    }

    PluginClassEND(AudioFilePlugin)

    static const char* _get_buffer_port_name(NativePluginHandle, const uint32_t index, const bool isOutput)
    {
        if (!isOutput)
            return nullptr;

        switch (index)
        {
        case 0:
            return "output_1";
        case 1:
            return "output_2";
        case 2:
            return "Play status";
        }

        return nullptr;
    }

    static const NativePortRange* _get_buffer_port_range(NativePluginHandle, const uint32_t index, const bool isOutput)
    {
        if (!isOutput || index != 2)
            return nullptr;

        static NativePortRange npr = { 0.f, 10.f };
        return &npr;
    }

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFilePlugin)
};

// --------------------------------------------------------------------------------------------------------------------

CARLA_API_EXPORT
void carla_register_native_plugin_audiofile();

CARLA_API_EXPORT
void carla_register_native_plugin_audiofile()
{
    static const NativePluginDescriptor audiofileDesc = {
        /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
        /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                     #ifndef __MOD_DEVICES__
                                                      |NATIVE_PLUGIN_HAS_INLINE_DISPLAY
                                                     #endif
                                                      |NATIVE_PLUGIN_HAS_UI
                                                      |NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE
                                                      |NATIVE_PLUGIN_REQUESTS_IDLE
                                                      |NATIVE_PLUGIN_USES_CONTROL_VOLTAGE
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
        AudioFilePlugin::_instantiate,
        AudioFilePlugin::_cleanup,
        AudioFilePlugin::_get_parameter_count,
        AudioFilePlugin::_get_parameter_info,
        AudioFilePlugin::_get_parameter_value,
        AudioFilePlugin::_get_midi_program_count,
        AudioFilePlugin::_get_midi_program_info,
        AudioFilePlugin::_set_parameter_value,
        AudioFilePlugin::_set_midi_program,
        AudioFilePlugin::_set_custom_data,
        AudioFilePlugin::_ui_show,
        AudioFilePlugin::_ui_idle,
        AudioFilePlugin::_ui_set_parameter_value,
        AudioFilePlugin::_ui_set_midi_program,
        AudioFilePlugin::_ui_set_custom_data,
        AudioFilePlugin::_activate,
        AudioFilePlugin::_deactivate,
        AudioFilePlugin::_process,
        AudioFilePlugin::_get_state,
        AudioFilePlugin::_set_state,
        AudioFilePlugin::_dispatcher,
        AudioFilePlugin::_render_inline_display,
        /* cvIns  */ 0,
        /* cvOuts */ 1,
        AudioFilePlugin::_get_buffer_port_name,
        AudioFilePlugin::_get_buffer_port_range,
        /* ui_width  */ 0,
        /* ui_height */ 0
    };

    carla_register_native_plugin(&audiofileDesc);
}

// --------------------------------------------------------------------------------------------------------------------
