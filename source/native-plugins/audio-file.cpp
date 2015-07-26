/*
 * Carla Native Plugins
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaNative.hpp"

#include "CarlaMutex.hpp"
#include "CarlaString.hpp"

#include "juce_audio_formats.h"

using namespace juce;

// -----------------------------------------------------------------------

static AudioFormatManager& getAudioFormatManagerInstance()
{
    static AudioFormatManager afm;
    afm.registerBasicFormats();
    return afm;
}

// -----------------------------------------------------------------------

class AudioFilePlugin : public NativePluginClass
{
public:
    AudioFilePlugin(const NativeHostDescriptor* const host)
        : NativePluginClass(host),
          fLoopMode(false),
          fDoProcess(false),
          fLength(0),
          //fThread("AudioFilePluginThread"),
          fReaderBuffer(),
          fReaderMutex(),
          fReader(),
          fReaderSource()
    {
        fReaderBuffer.setSize(2, static_cast<int>(getBufferSize()));
    }

    ~AudioFilePlugin() override
    {
        //fThread.stopThread(-1);
        fReader       = nullptr;
        fReaderSource = nullptr;
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
        param.hints = static_cast<NativeParameterHints>(NATIVE_PARAMETER_IS_ENABLED|NATIVE_PARAMETER_IS_BOOLEAN);
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

        const bool loopMode(value > 0.5f);

        if (fLoopMode == loopMode)
            return;

        fLoopMode = loopMode;

        const CarlaMutexLocker cml(fReaderMutex);

        if (fReaderSource != nullptr)
            fReaderSource->setLooping(loopMode);
    }

    void setCustomData(const char* const key, const char* const value) override
    {
        CARLA_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr && value[0] != '\0',);

        if (std::strcmp(key, "file") != 0)
            return;

        _loadAudioFile(value);
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float**, float** const outBuffer, const uint32_t frames, const NativeMidiEvent* const, const uint32_t) override
    {
        const NativeTimeInfo* const timePos(getTimeInfo());
        const int iframes(static_cast<int>(frames));

        float* const out1(outBuffer[0]);
        float* const out2(outBuffer[1]);

        if (fLength == 0 || ! fDoProcess)
        {
            //carla_stderr("P: no process");
            FloatVectorOperations::clear(out1, iframes);
            FloatVectorOperations::clear(out2, iframes);
            return;
        }

        const int64_t nextReadPos(fLoopMode ? (static_cast<int64_t>(timePos->frame) % fLength) : static_cast<int64_t>(timePos->frame));

        // not playing
        if (! timePos->playing)
        {
            //carla_stderr("P: not playing");
            FloatVectorOperations::clear(out1, iframes);
            FloatVectorOperations::clear(out2, iframes);

            const CarlaMutexLocker cml(fReaderMutex);

            if (fReaderSource != nullptr)
                fReaderSource->setNextReadPosition(nextReadPos);

            return;
        }

        const CarlaMutexLocker cml(fReaderMutex);

        if (fReaderSource != nullptr)
            fReaderSource->setNextReadPosition(nextReadPos);

        if (fReader == nullptr)
            return;

        fReader->read(&fReaderBuffer, 0, iframes, nextReadPos, true, true);

        FloatVectorOperations::copy(out1, fReaderBuffer.getReadPointer(0), iframes);
        FloatVectorOperations::copy(out2, fReaderBuffer.getReadPointer(1), iframes);
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
    // Plugin dispatcher calls

    void bufferSizeChanged(const uint32_t bufferSize) override
    {
        fReaderBuffer.setSize(2, static_cast<int>(bufferSize));
    }

private:
    bool fLoopMode;
    bool fDoProcess;
    int64_t fLength;

    //TimeSliceThread fThread;

    AudioSampleBuffer fReaderBuffer;
    CarlaMutex        fReaderMutex;

    ScopedPointer<AudioFormatReader>       fReader;
    ScopedPointer<AudioFormatReaderSource> fReaderSource;

    void _loadAudioFile(const char* const filename)
    {
        carla_stdout("AudioFilePlugin::loadFilename(\"%s\")", filename);

        fDoProcess = false;
        fLength    = 0;

        //fThread.stopThread(-1);

        {
            fReaderMutex.lock();
            AudioFormatReader*       const reader(fReader.release());
            AudioFormatReaderSource* const readerSource(fReaderSource.release());
            fReaderMutex.unlock();

            delete readerSource;
            delete reader;
        }

        const String jfilename = String(CharPointer_UTF8(filename));
        File file(jfilename);

        if (! file.existsAsFile())
            return;

        AudioFormatManager& afm(getAudioFormatManagerInstance());

        AudioFormat* const format(afm.findFormatForFileExtension(file.getFileExtension()));
        CARLA_SAFE_ASSERT_RETURN(format != nullptr,);

        if (MemoryMappedAudioFormatReader* const memReader = format->createMemoryMappedReader(file))
        {
            memReader->mapEntireFile();
            fReader = memReader;

            carla_stdout("Using memory mapped read file");
        }
        else
        {
            AudioFormatReader* const reader(afm.createReaderFor(file));
            CARLA_SAFE_ASSERT_RETURN(reader != nullptr,);

            // this code can be used for very large files
            //fThread.startThread();
            //BufferingAudioReader* const bufferingReader(new BufferingAudioReader(reader, fThread, getSampleRate()*2));
            //bufferingReader->setReadTimeout(50);

            AudioFormatReaderSource* const readerSource(new AudioFormatReaderSource(/*bufferingReader*/reader, false));
            readerSource->setLooping(fLoopMode);

            fReaderSource = readerSource;
            fReader       = reader;

            carla_stdout("Using regular read file");
        }

        fLength    = fReader->lengthInSamples;
        fDoProcess = true;
    }

    PluginClassEND(AudioFilePlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFilePlugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor audiofileDesc = {
    /* category  */ NATIVE_PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(NATIVE_PLUGIN_HAS_UI
                                                  |NATIVE_PLUGIN_NEEDS_UI_OPEN_SAVE),
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
