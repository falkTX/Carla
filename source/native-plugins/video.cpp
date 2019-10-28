/*
 * Carla Native Plugins
 * Copyright (C) 2012-2019 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaDefines.h"
#include "CarlaNativeExtUI.hpp"

// -----------------------------------------------------------------------

static const uint32_t videoBufferSize = 320*240*4*sizeof(float);

class VideoPlugin : public NativePluginAndUiClass
{
public:
    VideoPlugin(const NativeHostDescriptor* const host)
        : NativePluginAndUiClass(host, "video-ui"),
          fInlineDisplay() {}

protected:
    // -------------------------------------------------------------------
    // Plugin process calls

    void process(const float** inputs, float**, const uint32_t,
                 const NativeMidiEvent* const, const uint32_t) override
    {
        bool needsInlineRender = false;

//         carla_stdout("HERE 001");
        if (fInlineDisplay.mutex.tryLock())
        {
            // fInlineDisplay.buffer
            needsInlineRender = true;
            std::memcpy(fInlineDisplay.buffer, inputs[0], videoBufferSize);
            fInlineDisplay.mutex.unlock();
        }

        if (needsInlineRender /*&& ! fInlineDisplay.pending*/)
        {
            fInlineDisplay.pending = true;
            hostQueueDrawInlineDisplay();
        }

//         carla_stdout("HERE 002");
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher calls

    const NativeInlineDisplayImageSurface* renderInlineDisplay(const uint32_t width, const uint32_t height) override
    {
        CARLA_SAFE_ASSERT_RETURN(width > 0 && height > 0, nullptr);

        const size_t stride = width * 4;
        const size_t dataSize = stride * height;

        uchar* data = fInlineDisplay.data;

        if (fInlineDisplay.dataSize < dataSize || data == nullptr)
        {
            delete[] data;
            data = new uchar[dataSize];
            std::memset(data, 0, dataSize);
            fInlineDisplay.data = data;
            fInlineDisplay.dataSize = dataSize;
        }

        std::memset(data, 0, dataSize);

        fInlineDisplay.width = 320;
        fInlineDisplay.height = 240;
        fInlineDisplay.stride = 320 * 4;

        {
            const CarlaMutexLocker csl(fInlineDisplay.mutex);

            carla_stdout("Looking at data: %f %f %f %f %u %u %u %u",
                         static_cast<double>(fInlineDisplay.buffer[1000]),
                         static_cast<double>(fInlineDisplay.buffer[1001]),
                         static_cast<double>(fInlineDisplay.buffer[1002]),
                         static_cast<double>(fInlineDisplay.buffer[1003]),
                         width, height,
                         dataSize, videoBufferSize/sizeof(float)
                        );

            for (size_t i=0; i<dataSize && i<videoBufferSize/sizeof(float); ++i)
            {
                data[i] = static_cast<uint8_t>(
                            carla_fixedValue(0, 255, static_cast<int>(fInlineDisplay.buffer[i] * 255)));
//                 if (i % 4 == 0)
//                     data[i] = 160;
            }
        }

//         carla_stdout("renderInlineDisplay");
        fInlineDisplay.pending = false;
        return (NativeInlineDisplayImageSurface*)(NativeInlineDisplayImageSurfaceCompat*)&fInlineDisplay;
    }

private:
    struct InlineDisplay : NativeInlineDisplayImageSurfaceCompat {
        CarlaMutex mutex;
        float buffer[videoBufferSize/sizeof(float)];
        volatile bool pending;

        InlineDisplay()
            : NativeInlineDisplayImageSurfaceCompat(),
              mutex(),
              pending(false)
        {
            std::memset(buffer, 0, videoBufferSize);
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

    PluginClassEND(VideoPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VideoPlugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor videoDesc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_IS_RTSAFE
                                                  |NATIVE_PLUGIN_HAS_INLINE_DISPLAY
                                                  |NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_FIXED_BUFFERS
                                                  |NATIVE_PLUGIN_USES_VIDEO),
    /* supports  */ NATIVE_PLUGIN_SUPPORTS_NOTHING,
    /* audioIns  */ 0,
    /* audioOuts */ 1,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "Video",
    /* label     */ "video",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL_WITHCV(VideoPlugin),
    /* cvIns     */ 0,
    /* cvOuts    */ 0,
    /* videoIns  */ 1,
    /* videoOuts */ 0,
};

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_video();

CARLA_EXPORT
void carla_register_native_plugin_video()
{
    carla_register_native_plugin(&videoDesc);
}

// -----------------------------------------------------------------------
