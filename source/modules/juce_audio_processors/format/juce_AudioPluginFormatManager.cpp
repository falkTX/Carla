/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   27th April 2017).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

namespace PluginFormatManagerHelpers
{
    struct ErrorCallbackOnMessageThread : public CallbackMessage
    {
        ErrorCallbackOnMessageThread (const String& inError,
                                      AudioPluginFormat::InstantiationCompletionCallback* inCallback)
            : error (inError), callback (inCallback)
        {
        }

        void messageCallback() override          { callback->completionCallback (nullptr, error); }

        String error;
        ScopedPointer<AudioPluginFormat::InstantiationCompletionCallback> callback;
    };

    struct ErrorLambdaOnMessageThread : public CallbackMessage
    {
        ErrorLambdaOnMessageThread (const String& inError,
                                    std::function<void (AudioPluginInstance*, const String&)> f)
            : error (inError), lambda (f)
        {
        }

        void messageCallback() override          { lambda (nullptr, error); }

        String error;
        std::function<void (AudioPluginInstance*, const String&)> lambda;
    };
}

AudioPluginFormatManager::AudioPluginFormatManager() {}
AudioPluginFormatManager::~AudioPluginFormatManager() {}

//==============================================================================
void AudioPluginFormatManager::addDefaultFormats()
{
   #if JUCE_DEBUG
    // you should only call this method once!
    for (int i = formats.size(); --i >= 0;)
    {
       #if JUCE_PLUGINHOST_VST && (JUCE_MAC || JUCE_WINDOWS || JUCE_LINUX || JUCE_IOS)
        jassert (dynamic_cast<VSTPluginFormat*> (formats[i]) == nullptr);
       #endif

       #if JUCE_PLUGINHOST_VST3 && (JUCE_MAC || JUCE_WINDOWS)
        jassert (dynamic_cast<VST3PluginFormat*> (formats[i]) == nullptr);
       #endif

       #if JUCE_PLUGINHOST_AU && (JUCE_MAC || JUCE_IOS)
        jassert (dynamic_cast<AudioUnitPluginFormat*> (formats[i]) == nullptr);
       #endif

       #if JUCE_PLUGINHOST_LADSPA && JUCE_LINUX
        jassert (dynamic_cast<LADSPAPluginFormat*> (formats[i]) == nullptr);
       #endif
    }
   #endif

   #if JUCE_PLUGINHOST_AU && (JUCE_MAC || JUCE_IOS)
    formats.add (new AudioUnitPluginFormat());
   #endif

   #if JUCE_PLUGINHOST_VST && (JUCE_MAC || JUCE_WINDOWS || JUCE_LINUX || JUCE_IOS)
    formats.add (new VSTPluginFormat());
   #endif

   #if JUCE_PLUGINHOST_VST3 && (JUCE_MAC || JUCE_WINDOWS)
    formats.add (new VST3PluginFormat());
   #endif

   #if JUCE_PLUGINHOST_LADSPA && JUCE_LINUX
    formats.add (new LADSPAPluginFormat());
   #endif
}

int AudioPluginFormatManager::getNumFormats()
{
    return formats.size();
}

AudioPluginFormat* AudioPluginFormatManager::getFormat (const int index)
{
    return formats [index];
}

void AudioPluginFormatManager::addFormat (AudioPluginFormat* const format)
{
    formats.add (format);
}

AudioPluginInstance* AudioPluginFormatManager::createPluginInstance (const PluginDescription& description, double rate,
                                                                     int blockSize, String& errorMessage) const
{
    if (AudioPluginFormat* format = findFormatForDescription (description, errorMessage))
        return format->createInstanceFromDescription (description, rate, blockSize, errorMessage);

    return nullptr;
}

void AudioPluginFormatManager::createPluginInstanceAsync (const PluginDescription& description,
                                                          double initialSampleRate,
                                                          int initialBufferSize,
                                                          AudioPluginFormat::InstantiationCompletionCallback* callback)
{
    String error;

    if (AudioPluginFormat* format = findFormatForDescription (description, error))
        return format->createPluginInstanceAsync (description, initialSampleRate, initialBufferSize, callback);

    (new PluginFormatManagerHelpers::ErrorCallbackOnMessageThread (error, callback))->post();
}

void AudioPluginFormatManager::createPluginInstanceAsync (const PluginDescription& description,
                                                          double initialSampleRate,
                                                          int initialBufferSize,
                                                          std::function<void (AudioPluginInstance*, const String&)> f)
{
    String error;

    if (AudioPluginFormat* format = findFormatForDescription (description, error))
        return format->createPluginInstanceAsync (description, initialSampleRate, initialBufferSize, f);

    (new PluginFormatManagerHelpers::ErrorLambdaOnMessageThread (error, f))->post();
}

AudioPluginFormat* AudioPluginFormatManager::findFormatForDescription (const PluginDescription& description, String& errorMessage) const
{
    errorMessage = String();

    for (int i = 0; i < formats.size(); ++i)
    {
        AudioPluginFormat* format;

        if ((format = formats.getUnchecked (i))->getName() == description.pluginFormatName
               && format->fileMightContainThisPluginType (description.fileOrIdentifier))
            return format;
    }

    errorMessage = NEEDS_TRANS ("No compatible plug-in format exists for this plug-in");

    return nullptr;
}

bool AudioPluginFormatManager::doesPluginStillExist (const PluginDescription& description) const
{
    for (int i = 0; i < formats.size(); ++i)
        if (formats.getUnchecked(i)->getName() == description.pluginFormatName)
            return formats.getUnchecked(i)->doesPluginStillExist (description);

    return false;
}

} // namespace juce
