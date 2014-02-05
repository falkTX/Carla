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

#include "JucePluginWindow.hpp"

using namespace juce;

// -----------------------------------------------------------------------

#include "juce-host/FilterGraph.h"
#include "juce-host/InternalFilters.h"
#include "juce-host/GraphEditorPanel.h"

// -----------------------------------------------------------------------

class JucePatchbayPlugin : public NativePluginClass
{
public:
    JucePatchbayPlugin(const NativeHostDescriptor* const host)
        : NativePluginClass(host),
          graph(formatManager),
          fAudioBuffer(1, 0)
    {
        formatManager.addDefaultFormats();
        formatManager.addFormat(new InternalPluginFormat());
        graph.ready();

        graph.getGraph().setPlayConfigDetails(2, 2, getSampleRate(), static_cast<int>(getBufferSize()));

        fMidiBuffer.ensureSize(512*2);
        fMidiBuffer.clear();
    }

    ~JucePatchbayPlugin() override
    {
    }

protected:
    // -------------------------------------------------------------------
    // Plugin process calls

    void activate() override
    {
        graph.getGraph().prepareToPlay(getSampleRate(), static_cast<int>(getBufferSize()));

        fAudioBuffer.setSize(2, static_cast<int>(getBufferSize()));
    }

    void deactivate() override
    {
        graph.getGraph().releaseResources();
    }

    void process(float** inBuffer, float** const outBuffer, const uint32_t frames, const NativeMidiEvent* const, const uint32_t) override
    {
        fAudioBuffer.copyFrom(0, 0, inBuffer[0], static_cast<int>(frames));
        fAudioBuffer.copyFrom(1, 0, inBuffer[1], static_cast<int>(frames));

        graph.getGraph().processBlock(fAudioBuffer, fMidiBuffer);

        FloatVectorOperations::copy(outBuffer[0], fAudioBuffer.getSampleData(0), static_cast<int>(frames));
        FloatVectorOperations::copy(outBuffer[1], fAudioBuffer.getSampleData(1), static_cast<int>(frames));
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        if (show)
        {
            if (fWindow == nullptr)
            {
                fWindow = new JucePluginWindow();
                fWindow->setName(getUiName());
                fWindow->setResizable(true, false);
            }

            if (fComponent == nullptr)
            {
                fComponent = new GraphDocumentComponent(graph);
                fComponent->setSize(300, 300);
            }

            fWindow->show(fComponent);
        }
        else if (fWindow != nullptr)
        {
            fWindow->hide();

            fComponent = nullptr;
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

private:
    AudioPluginFormatManager formatManager;
    FilterGraph graph;

    AudioSampleBuffer fAudioBuffer;
    MidiBuffer        fMidiBuffer;

    ScopedPointer<GraphDocumentComponent> fComponent;
    ScopedPointer<JucePluginWindow> fWindow;

    //OwnedArray <PluginDescription> internalTypes;
    //KnownPluginList knownPluginList;
    //KnownPluginList::SortMethod pluginSortMethod;

    PluginClassEND(JucePatchbayPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JucePatchbayPlugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor jucePatchbayDesc = {
    /* category  */ PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(PLUGIN_IS_SYNTH|PLUGIN_HAS_UI/*|PLUGIN_USES_STATE*/),
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

// -----------------------------------------------------------------------
