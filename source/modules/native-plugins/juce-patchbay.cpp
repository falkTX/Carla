/*
 * Carla Native Plugins
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#include "juce_audio_processors.h"
#include "juce_gui_extra.h"

using namespace juce;

// -----------------------------------------------------------------------

#include "juce-host/FilterGraph.h"
#include "juce-host/InternalFilters.h"
#include "juce-host/GraphEditorPanel.h"
#include "juce-host/MainHostWindow.h"

// -----------------------------------------------------------------------

class JucePatchbayPlugin : public NativePluginClass
{
public:
    JucePatchbayPlugin(const NativeHostDescriptor* const host)
        : NativePluginClass(host),
          fFormatManager(),
          fGraph(fFormatManager),
          fAudioBuffer(1, 0),
          fMidiKeyState(nullptr)
    {
        PropertiesFile::Options options;
        options.applicationName     = "Juce Audio Plugin Host";
        options.filenameSuffix      = "settings";
        options.osxLibrarySubFolder = "Preferences";

        fAppProperties = new ApplicationProperties();
        fAppProperties->setStorageParameters(options);

        fFormatManager.addDefaultFormats();
        fFormatManager.addFormat(new InternalPluginFormat());
        fGraph.ready(fAppProperties);

        fGraph.getGraph().setPlayConfigDetails(2, 2, getSampleRate(), static_cast<int>(getBufferSize()));

        fMidiBuffer.ensureSize(512*2);
        fMidiBuffer.clear();
    }

    ~JucePatchbayPlugin() override
    {
        fGraph.clear();
        fAppProperties = nullptr;
    }

protected:
    // -------------------------------------------------------------------
    // Plugin process calls

    void activate() override
    {
        fGraph.getGraph().prepareToPlay(getSampleRate(), static_cast<int>(getBufferSize()));

        fAudioBuffer.setSize(2, static_cast<int>(getBufferSize()));

        {
            const ScopedLock csl(fMidiKeyMutex);

            if (fMidiKeyState != nullptr)
                fMidiKeyState->reset();
        }
    }

    void deactivate() override
    {
        fGraph.getGraph().releaseResources();
    }

    void process(float** inBuffer, float** const outBuffer, const uint32_t frames, const NativeMidiEvent* const midiEvents, const uint32_t midiEventCount) override
    {
        fMidiBuffer.clear();

        for (uint32_t i=0; i < midiEventCount; ++i)
        {
            const NativeMidiEvent* const midiEvent(&midiEvents[i]);
            fMidiBuffer.addEvent(midiEvent->data, midiEvent->size, midiEvent->time);
        }

        {
            const ScopedLock csl(fMidiKeyMutex);

            if (fMidiKeyState != nullptr)
                fMidiKeyState->processNextMidiBuffer(fMidiBuffer, 0, static_cast<int>(frames), true);
        }

        FloatVectorOperations::copy(outBuffer[0], inBuffer[0], static_cast<int>(frames));
        FloatVectorOperations::copy(outBuffer[1], inBuffer[1], static_cast<int>(frames));
        AudioSampleBuffer audioBuf(outBuffer, 2, static_cast<int>(frames));

        fGraph.getGraph().processBlock(audioBuf, fMidiBuffer);

        MidiBuffer::Iterator outBufferIterator(fMidiBuffer);
        const uint8_t* midiData;
        int numBytes;
        int sampleNumber;

        NativeMidiEvent tmpEvent;
        tmpEvent.port = 0;

        for (; outBufferIterator.getNextEvent(midiData, numBytes, sampleNumber);)
        {
            if (numBytes <= 0 || numBytes > 4)
                continue;

            tmpEvent.size = numBytes;
            tmpEvent.time = sampleNumber;

            std::memcpy(tmpEvent.data, midiData, sizeof(uint8_t)*tmpEvent.size);
            writeMidiEvent(&tmpEvent);
        }
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        const MessageManagerLock mmLock;

        if (show)
        {
            if (fWindow == nullptr)
            {
                fWindow = new MainHostWindow(fFormatManager, fGraph, *fAppProperties);
                fWindow->setName(getUiName());
            }
            {
                const ScopedLock csl(fMidiKeyMutex);
                fMidiKeyState = fWindow->getMidiState();
            }
            fWindow->toFront(true);
        }
        else if (fWindow != nullptr)
        {
            {
                const ScopedLock csl(fMidiKeyMutex);
                fMidiKeyState = nullptr;
            }
            fWindow->setVisible(false);
            fWindow = nullptr;
        }
    }

    void uiIdle() override
    {
        if (fWindow == nullptr)
            return;

        if (fWindow->wasClosedByUser())
        {
            uiShow(false);
            uiClosed();
        }
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    char* getState() const override
    {
        ScopedPointer<XmlElement> xml(fGraph.createXml());

        MemoryOutputStream stream;
        xml->writeToStream(stream, String::empty);

        return strdup(stream.toUTF8().toRawUTF8());
    }

    void setState(const char* const data) override
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);

        String sdata(data);
        XmlDocument doc(sdata);
        ScopedPointer<XmlElement> xml(doc.getDocumentElement());

        if (xml != nullptr && xml->hasTagName("FILTERGRAPH"))
            fGraph.restoreFromXml(*xml);
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher calls

    void uiNameChanged(const char* const uiName) override
    {
        CARLA_SAFE_ASSERT_RETURN(uiName != nullptr,);

        if (fWindow == nullptr)
            return;

        const MessageManagerLock mmLock;
        fWindow->setName(uiName);
    }

private:
    AudioPluginFormatManager fFormatManager;
    FilterGraph fGraph;

    AudioSampleBuffer fAudioBuffer;
    MidiBuffer        fMidiBuffer;

    ScopedPointer<ApplicationProperties> fAppProperties;
    ScopedPointer<MainHostWindow> fWindow;

    MidiKeyboardState* fMidiKeyState;
    CriticalSection fMidiKeyMutex;

    PluginClassEND(JucePatchbayPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JucePatchbayPlugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor jucePatchbayDesc = {
    /* category  */ PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(PLUGIN_IS_SYNTH|PLUGIN_HAS_UI|PLUGIN_NEEDS_FIXED_BUFFERS|PLUGIN_NEEDS_UI_JUCE|PLUGIN_USES_STATE|PLUGIN_USES_TIME),
    /* supports  */ static_cast<NativePluginSupports>(0x0),
    /* audioIns  */ 2,
    /* audioOuts */ 2,
    /* midiIns   */ 1,
    /* midiOuts  */ 1,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "Juce Patchbay",
    /* label     */ "jucePatchbay",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(JucePatchbayPlugin)
};

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_jucePatchbay()
{
    carla_register_native_plugin(&jucePatchbayDesc);
}

// -----------------------------------------------------------------------

#include "juce-host/juce_MidiKeyboardComponent.h"
#include "juce-host/juce_MidiKeyboardComponent.cpp"

#include "juce-host/FilterGraph.cpp"
#include "juce-host/InternalFilters.cpp"
#include "juce-host/GraphEditorPanel.cpp"
#include "juce-host/MainHostWindow.cpp"

// -----------------------------------------------------------------------
