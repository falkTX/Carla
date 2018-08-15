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

#include "AudioFormat.h"
#include "../files/File.h"

namespace water {

AudioFormat::AudioFormat (String name, StringArray extensions)
   : formatName (name), fileExtensions (extensions)
{
}

AudioFormat::AudioFormat (StringRef name, StringRef extensions)
   : formatName (name.text), fileExtensions (StringArray::fromTokens (extensions, false))
{
}

AudioFormat::~AudioFormat()
{
}

bool AudioFormat::canHandleFile (const File& f)
{
    for (int i = 0; i < fileExtensions.size(); ++i)
        if (f.hasFileExtension (fileExtensions[i]))
            return true;

    return false;
}

const String& AudioFormat::getFormatName() const                { return formatName; }
const StringArray& AudioFormat::getFileExtensions() const       { return fileExtensions; }
bool AudioFormat::isCompressed()                                { return false; }
StringArray AudioFormat::getQualityOptions()                    { return StringArray(); }

#if 0
MemoryMappedAudioFormatReader* AudioFormat::createMemoryMappedReader (const File&)
{
    return nullptr;
}

MemoryMappedAudioFormatReader* AudioFormat::createMemoryMappedReader (FileInputStream* fin)
{
    delete fin;
    return nullptr;
}
#endif

}
