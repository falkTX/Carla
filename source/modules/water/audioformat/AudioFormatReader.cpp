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

#include "AudioFormatReader.h"

#include "../buffers/AudioSampleBuffer.h"
#include "../streams/InputStream.h"

namespace water {

AudioFormatReader::AudioFormatReader (InputStream* const in, const String& name)
    : sampleRate (0),
      bitsPerSample (0),
      lengthInSamples (0),
      numChannels (0),
      usesFloatingPointData (false),
      input (in),
      formatName (name)
{
}

AudioFormatReader::~AudioFormatReader()
{
    delete input;
}

bool AudioFormatReader::read (int* const* destSamples,
                              int numDestChannels,
                              int64 startSampleInSource,
                              int numSamplesToRead,
                              const bool fillLeftoverChannelsWithCopies)
{
    jassert (numDestChannels > 0); // you have to actually give this some channels to work with!

    const size_t originalNumSamplesToRead = (size_t) numSamplesToRead;
    int startOffsetInDestBuffer = 0;

    if (startSampleInSource < 0)
    {
        const int silence = (int) jmin (-startSampleInSource, (int64) numSamplesToRead);

        for (int i = numDestChannels; --i >= 0;)
            if (destSamples[i] != nullptr)
                zeromem (destSamples[i], sizeof (int) * (size_t) silence);

        startOffsetInDestBuffer += silence;
        numSamplesToRead -= silence;
        startSampleInSource = 0;
    }

    if (numSamplesToRead <= 0)
        return true;

    if (! readSamples (const_cast<int**> (destSamples),
                       jmin ((int) numChannels, numDestChannels), startOffsetInDestBuffer,
                       startSampleInSource, numSamplesToRead))
        return false;

    if (numDestChannels > (int) numChannels)
    {
        if (fillLeftoverChannelsWithCopies)
        {
            int* lastFullChannel = destSamples[0];

            for (int i = (int) numChannels; --i > 0;)
            {
                if (destSamples[i] != nullptr)
                {
                    lastFullChannel = destSamples[i];
                    break;
                }
            }

            if (lastFullChannel != nullptr)
                for (int i = (int) numChannels; i < numDestChannels; ++i)
                    if (destSamples[i] != nullptr)
                        memcpy (destSamples[i], lastFullChannel, sizeof (int) * originalNumSamplesToRead);
        }
        else
        {
            for (int i = (int) numChannels; i < numDestChannels; ++i)
                if (destSamples[i] != nullptr)
                    zeromem (destSamples[i], sizeof (int) * originalNumSamplesToRead);
        }
    }

    return true;
}

static void readChannels (AudioFormatReader& reader,
                          int** const chans, AudioSampleBuffer* const buffer,
                          const int startSample, const int numSamples,
                          const int64 readerStartSample, const int numTargetChannels)
{
    for (int j = 0; j < numTargetChannels; ++j)
        chans[j] = reinterpret_cast<int*> (buffer->getWritePointer (j, startSample));

    chans[numTargetChannels] = nullptr;
    reader.read (chans, numTargetChannels, readerStartSample, numSamples, true);
}

    #define JUCE_PERFORM_VEC_OP_SRC_DEST(normalOp, vecOp, locals, increment, setupOp) \
        for (int i = 0; i < num; ++i) normalOp;

static void convertFixedToFloat (float* dest, const int* src, float multiplier, int num) noexcept
{
    for (int i = 0; i < num; ++i)
        dest[i] = src[i] * multiplier;
}

void AudioFormatReader::read (AudioSampleBuffer* buffer,
                              int startSample,
                              int numSamples,
                              int64 readerStartSample,
                              bool useReaderLeftChan,
                              bool useReaderRightChan)
{
    jassert (buffer != nullptr);
    jassert (startSample >= 0 && startSample + numSamples <= buffer->getNumSamples());

    if (numSamples > 0)
    {
        const int numTargetChannels = buffer->getNumChannels();

        if (numTargetChannels <= 2)
        {
            int* const dest0 = reinterpret_cast<int*> (buffer->getWritePointer (0, startSample));
            int* const dest1 = reinterpret_cast<int*> (numTargetChannels > 1 ? buffer->getWritePointer (1, startSample) : nullptr);
            int* chans[3];

            if (useReaderLeftChan == useReaderRightChan)
            {
                chans[0] = dest0;
                chans[1] = numChannels > 1 ? dest1 : nullptr;
            }
            else if (useReaderLeftChan || (numChannels == 1))
            {
                chans[0] = dest0;
                chans[1] = nullptr;
            }
            else if (useReaderRightChan)
            {
                chans[0] = nullptr;
                chans[1] = dest0;
            }

            chans[2] = nullptr;
            read (chans, 2, readerStartSample, numSamples, true);

            // if the target's stereo and the source is mono, dupe the first channel..
            if (numTargetChannels > 1 && (chans[0] == nullptr || chans[1] == nullptr))
                memcpy (dest1, dest0, sizeof (float) * (size_t) numSamples);
        }
        else if (numTargetChannels <= 64)
        {
            int* chans[65];
            readChannels (*this, chans, buffer, startSample, numSamples, readerStartSample, numTargetChannels);
        }
        else
        {
            HeapBlock<int*> chans;
            chans.malloc ((size_t) numTargetChannels + 1);
            // FIXME, check malloc return
            readChannels (*this, chans, buffer, startSample, numSamples, readerStartSample, numTargetChannels);
        }

        if (! usesFloatingPointData)
            for (int j = 0; j < numTargetChannels; ++j)
                if (float* const d = buffer->getWritePointer (j, startSample))
                    convertFixedToFloat (d, reinterpret_cast<const int*> (d), 1.0f / 0x7fffffff, numSamples);
    }
}

}
