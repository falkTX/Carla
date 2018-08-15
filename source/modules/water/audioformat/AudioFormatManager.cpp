/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2015 ROLI Ltd.
   Copyright (C) 2018 Filipe Coelho <falktx@falktx.com>

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   Water is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ==============================================================================
*/

#include "AudioFormatManager.h"

#include "../audiocodecs/WavAudioFormat.h"
#include "../files/FileInputStream.h"

namespace water {

AudioFormatManager::AudioFormatManager()  : defaultFormatIndex (0) {}
AudioFormatManager::~AudioFormatManager() {}

//==============================================================================
void AudioFormatManager::registerFormat (AudioFormat* newFormat, const bool makeThisTheDefaultFormat)
{
    jassert (newFormat != nullptr);

    if (newFormat != nullptr)
    {
       #if DEBUG
        for (int i = getNumKnownFormats(); --i >= 0;)
        {
            if (getKnownFormat (i)->getFormatName() == newFormat->getFormatName())
            {
                jassertfalse; // trying to add the same format twice!
            }
        }
       #endif

        if (makeThisTheDefaultFormat)
            defaultFormatIndex = getNumKnownFormats();

        knownFormats.add (newFormat);
    }
}

void AudioFormatManager::registerBasicFormats()
{
    registerFormat (new WavAudioFormat(), true);
#if 0
    registerFormat (new AiffAudioFormat(), false);
#endif

   #if WATER_USE_FLAC
    registerFormat (new FlacAudioFormat(), false);
   #endif

   #if WATER_USE_OGGVORBIS
    registerFormat (new OggVorbisAudioFormat(), false);
   #endif

   #if WATER_MAC || WATER_IOS
    registerFormat (new CoreAudioFormat(), false);
   #endif

   #if WATER_USE_MP3AUDIOFORMAT
    registerFormat (new MP3AudioFormat(), false);
   #endif

   #if WATER_USE_WINDOWS_MEDIA_FORMAT
    registerFormat (new WindowsMediaAudioFormat(), false);
   #endif
}

void AudioFormatManager::clearFormats()
{
    knownFormats.clear();
    defaultFormatIndex = 0;
}

int AudioFormatManager::getNumKnownFormats() const
{
    return knownFormats.size();
}

AudioFormat* AudioFormatManager::getKnownFormat (const int index) const
{
    return knownFormats [index];
}

AudioFormat* AudioFormatManager::getDefaultFormat() const
{
    return getKnownFormat (defaultFormatIndex);
}

AudioFormat* AudioFormatManager::findFormatForFileExtension (const String& fileExtension) const
{
    if (! fileExtension.startsWithChar ('.'))
        return findFormatForFileExtension ("." + fileExtension);

    for (int i = 0; i < getNumKnownFormats(); ++i)
        if (getKnownFormat(i)->getFileExtensions().contains (fileExtension, true))
            return getKnownFormat(i);

    return nullptr;
}

String AudioFormatManager::getWildcardForAllFormats() const
{
    StringArray extensions;

    for (int i = 0; i < getNumKnownFormats(); ++i)
        extensions.addArray (getKnownFormat(i)->getFileExtensions());

    extensions.trim();
    extensions.removeEmptyStrings();

    for (int i = 0; i < extensions.size(); ++i)
        extensions.set (i, (extensions[i].startsWithChar ('.') ? "*" : "*.") + extensions[i]);

    extensions.removeDuplicates (true);
    return extensions.joinIntoString (";");
}

//==============================================================================
AudioFormatReader* AudioFormatManager::createReaderFor (const File& file)
{
    // you need to actually register some formats before the manager can
    // use them to open a file!
    jassert (getNumKnownFormats() > 0);

    for (int i = 0; i < getNumKnownFormats(); ++i)
    {
        AudioFormat* const af = getKnownFormat(i);

        if (af->canHandleFile (file))
            if (InputStream* const in = file.createInputStream())
                if (AudioFormatReader* const r = af->createReaderFor (in, true))
                    return r;
    }

    return nullptr;
}

AudioFormatReader* AudioFormatManager::createReaderFor (InputStream* audioFileStream)
{
    // you need to actually register some formats before the manager can
    // use them to open a file!
    jassert (getNumKnownFormats() > 0);

    ScopedPointer<InputStream> in (audioFileStream);

    if (in != nullptr)
    {
        const int64 originalStreamPos = in->getPosition();

        for (int i = 0; i < getNumKnownFormats(); ++i)
        {
            if (AudioFormatReader* const r = getKnownFormat(i)->createReaderFor (in, false))
            {
                in.release();
                return r;
            }

            in->setPosition (originalStreamPos);

            // the stream that is passed-in must be capable of being repositioned so
            // that all the formats can have a go at opening it.
            jassert (in->getPosition() == originalStreamPos);
        }
    }

    return nullptr;
}

}
