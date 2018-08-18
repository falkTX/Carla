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

#ifndef WATER_AUDIOFORMAT_H_INCLUDED
#define WATER_AUDIOFORMAT_H_INCLUDED

#include "../text/StringArray.h"

namespace water {

//==============================================================================
/**
    Subclasses of AudioFormat are used to read and write different audio
    file formats.

    @see AudioFormatReader, AudioFormatWriter, WavAudioFormat, AiffAudioFormat
*/
class AudioFormat
{
public:
    //==============================================================================
    /** Destructor. */
    virtual ~AudioFormat();

    //==============================================================================
    /** Returns the name of this format.
        e.g. "WAV file" or "AIFF file"
    */
    const String& getFormatName() const;

    /** Returns all the file extensions that might apply to a file of this format.

        The first item will be the one that's preferred when creating a new file.

        So for a wav file this might just return ".wav"; for an AIFF file it might
        return two items, ".aif" and ".aiff"
    */
    const StringArray& getFileExtensions() const;

    //==============================================================================
    /** Returns true if this the given file can be read by this format.

        Subclasses shouldn't do too much work here, just check the extension or
        file type. The base class implementation just checks the file's extension
        against one of the ones that was registered in the constructor.
    */
    virtual bool canHandleFile (const File& fileToTest);

    /** Returns a set of sample rates that the format can read and write. */
    virtual Array<int> getPossibleSampleRates() = 0;

    /** Returns a set of bit depths that the format can read and write. */
    virtual Array<int> getPossibleBitDepths() = 0;

    /** Returns true if the format can do 2-channel audio. */
    virtual bool canDoStereo() = 0;

    /** Returns true if the format can do 1-channel audio. */
    virtual bool canDoMono() = 0;

    /** Returns true if the format uses compressed data. */
    virtual bool isCompressed();

    /** Returns a list of different qualities that can be used when writing.

        Non-compressed formats will just return an empty array, but for something
        like Ogg-Vorbis or MP3, it might return a list of bit-rates, etc.

        When calling createWriterFor(), an index from this array is passed in to
        tell the format which option is required.
    */
    virtual StringArray getQualityOptions();

    //==============================================================================
    /** Tries to create an object that can read from a stream containing audio
        data in this format.

        The reader object that is returned can be used to read from the stream, and
        should then be deleted by the caller.

        @param sourceStream                 the stream to read from - the AudioFormatReader object
                                            that is returned will delete this stream when it no longer
                                            needs it.
        @param deleteStreamIfOpeningFails   if no reader can be created, this determines whether this method
                                            should delete the stream object that was passed-in. (If a valid
                                            reader is returned, it will always be in charge of deleting the
                                            stream, so this parameter is ignored)
        @see AudioFormatReader
    */
    virtual AudioFormatReader* createReaderFor (InputStream* sourceStream,
                                                bool deleteStreamIfOpeningFails) = 0;

protected:
    /** Creates an AudioFormat object.

        @param formatName       this sets the value that will be returned by getFormatName()
        @param fileExtensions   an array of file extensions - these will be returned by getFileExtensions()
    */
    AudioFormat (String formatName, StringArray fileExtensions);

    /** Creates an AudioFormat object.

        @param formatName       this sets the value that will be returned by getFormatName()
        @param fileExtensions   a whitespace-separated list of file extensions - these will
                                be returned by getFileExtensions()
    */
    AudioFormat (StringRef formatName, StringRef fileExtensions);

private:
    //==============================================================================
    String formatName;
    StringArray fileExtensions;
};

}

#endif   // WATER_AUDIOFORMAT_H_INCLUDED
