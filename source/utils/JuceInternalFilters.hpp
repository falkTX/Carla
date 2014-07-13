/*
 * Juce Internal Filters
 * Copyright (c) 2013 Raw Material Software Ltd.
 * Copyright (C) 2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef JUCE_INTERNAL_FILTERS_HPP_INCLUDED
#define JUCE_INTERNAL_FILTERS_HPP_INCLUDED

#include "juce_audio_processors.h"

// -----------------------------------------------------------------------

namespace juce {

class InternalPluginFormat : public AudioPluginFormat
{
public:
    enum InternalFilterType
    {
        audioInputFilter = 0,
        audioOutputFilter,
        midiInputFilter,
        midiOutputFilter,
        endOfFilterTypes
    };

    InternalPluginFormat()
    {
        {
            AudioProcessorGraph::AudioGraphIOProcessor p(AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode);
            p.fillInPluginDescription(audioOutDesc);
        }

        {
            AudioProcessorGraph::AudioGraphIOProcessor p(AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode);
            p.fillInPluginDescription(audioInDesc);
        }

        {
            AudioProcessorGraph::AudioGraphIOProcessor p(AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode);
            p.fillInPluginDescription(midiInDesc);
        }

        {
            AudioProcessorGraph::AudioGraphIOProcessor p(AudioProcessorGraph::AudioGraphIOProcessor::midiOutputNode);
            p.fillInPluginDescription(midiOutDesc);
        }
    }

    // -------------------------------------------------------------------

    const PluginDescription* getDescriptionFor(const InternalFilterType type)
    {
        switch (type)
        {
        case audioInputFilter:
            return &audioInDesc;
        case audioOutputFilter:
            return &audioOutDesc;
        case midiInputFilter:
            return &midiInDesc;
        case midiOutputFilter:
            return &midiOutDesc;
        default:
            return nullptr;
        }
    }

    void getAllTypes(OwnedArray <PluginDescription>& results) override
    {
        for (int i = 0; i < (int) endOfFilterTypes; ++i)
            results.add(new PluginDescription(*getDescriptionFor((InternalFilterType)i)));
    }

    AudioPluginInstance* createInstanceFromDescription(const PluginDescription& desc, double, int) override
    {
        if (desc.name == audioOutDesc.name)
            return new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode);

        if (desc.name == audioInDesc.name)
            return new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode);

        if (desc.name == midiInDesc.name)
            return new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode);

        if (desc.name == midiOutDesc.name)
            return new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::midiOutputNode);

        return nullptr;
    }

    // -------------------------------------------------------------------

    String getName() const                                                   override { return "Internal";       }
    bool fileMightContainThisPluginType(const String&)                       override { return false;            }
    FileSearchPath getDefaultLocationsToSearch()                             override { return FileSearchPath(); }
    bool canScanForPlugins() const                                           override { return false;            }
    bool doesPluginStillExist(const PluginDescription&)                      override { return true;             }
    String getNameOfPluginFromIdentifier(const String& fileOrIdentifier)     override { return fileOrIdentifier; }
    bool pluginNeedsRescanning(const PluginDescription&)                     override { return false;            }
    StringArray searchPathsForPlugins(const FileSearchPath&, bool)           override { return StringArray();    }
    void findAllTypesForFile(OwnedArray <PluginDescription>&, const String&) override {}

    // -------------------------------------------------------------------

private:
    PluginDescription audioInDesc;
    PluginDescription audioOutDesc;
    PluginDescription midiInDesc;
    PluginDescription midiOutDesc;
};

} // namespace juce

typedef juce::InternalPluginFormat JuceInternalPluginFormat;

// -----------------------------------------------------------------------

#endif // JUCE_INTERNAL_FILTERS_HPP_INCLUDED
