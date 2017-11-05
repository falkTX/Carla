/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2017 Filipe Coelho <falktx@falktx.com>

   Permission is granted to use this software under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license/

   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH REGARD
   TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
   FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
   OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
   USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
   OF THIS SOFTWARE.

  ==============================================================================
*/

#ifndef WATER_AUDIOSAMPLEBUFFER_H_INCLUDED
#define WATER_AUDIOSAMPLEBUFFER_H_INCLUDED

#include "../memory/HeapBlock.h"

#include "CarlaMathUtils.hpp"

namespace water {

//==============================================================================
/**
    A multi-channel buffer of floating point audio samples.

    @see AudioSampleBuffer
*/
class AudioSampleBuffer
{
public:
    //==============================================================================
    /** Creates an empty buffer with 0 channels and 0 length. */
    AudioSampleBuffer() noexcept
       : numChannels (0), size (0), allocatedBytes (0),
         channels (static_cast<float**> (preallocatedChannelSpace)),
         isClear (false)
    {
    }

    //==============================================================================
    /** Creates a buffer with a specified number of channels and samples.

        The contents of the buffer will initially be undefined, so use clear() to
        set all the samples to zero.

        The buffer will allocate its memory internally, and this will be released
        when the buffer is deleted. If the memory can't be allocated, this will
        throw a std::bad_alloc exception.
    */
    AudioSampleBuffer (int numChannelsToAllocate,
                 int numSamplesToAllocate) noexcept
       : numChannels (numChannelsToAllocate),
         size (numSamplesToAllocate)
    {
        CARLA_SAFE_ASSERT_RETURN (size >= 0,);
        CARLA_SAFE_ASSERT_RETURN (numChannels >= 0,);

        allocateData();
    }

    /** Creates a buffer using a pre-allocated block of memory.

        Note that if the buffer is resized or its number of channels is changed, it
        will re-allocate memory internally and copy the existing data to this new area,
        so it will then stop directly addressing this memory.

        @param dataToReferTo    a pre-allocated array containing pointers to the data
                                for each channel that should be used by this buffer. The
                                buffer will only refer to this memory, it won't try to delete
                                it when the buffer is deleted or resized.
        @param numChannelsToUse the number of channels to use - this must correspond to the
                                number of elements in the array passed in
        @param numSamples       the number of samples to use - this must correspond to the
                                size of the arrays passed in
    */
    AudioSampleBuffer (float* const* dataToReferTo,
                 int numChannelsToUse,
                 int numSamples) noexcept
        : numChannels (numChannelsToUse),
          size (numSamples),
          allocatedBytes (0)
    {
        CARLA_SAFE_ASSERT_RETURN (dataToReferTo != nullptr,);
        CARLA_SAFE_ASSERT_RETURN (numChannelsToUse >= 0 && numSamples >= 0,);

        allocateChannels (dataToReferTo, 0);
    }

    /** Creates a buffer using a pre-allocated block of memory.

        Note that if the buffer is resized or its number of channels is changed, it
        will re-allocate memory internally and copy the existing data to this new area,
        so it will then stop directly addressing this memory.

        @param dataToReferTo    a pre-allocated array containing pointers to the data
                                for each channel that should be used by this buffer. The
                                buffer will only refer to this memory, it won't try to delete
                                it when the buffer is deleted or resized.
        @param numChannelsToUse the number of channels to use - this must correspond to the
                                number of elements in the array passed in
        @param startSample      the offset within the arrays at which the data begins
        @param numSamples       the number of samples to use - this must correspond to the
                                size of the arrays passed in
    */
    AudioSampleBuffer (float* const* dataToReferTo,
                 int numChannelsToUse,
                 int startSample,
                 int numSamples) noexcept
        : numChannels (numChannelsToUse),
          size (numSamples),
          allocatedBytes (0),
          isClear (false)
    {
        CARLA_SAFE_ASSERT_RETURN (dataToReferTo != nullptr,);
        CARLA_SAFE_ASSERT_RETURN (numChannelsToUse >= 0 && startSample >= 0 && numSamples >= 0,);

        allocateChannels (dataToReferTo, startSample);
    }

    /** Copies another buffer.

        This buffer will make its own copy of the other's data, unless the buffer was created
        using an external data buffer, in which case boths buffers will just point to the same
        shared block of data.
    */
    AudioSampleBuffer (const AudioSampleBuffer& other) noexcept
       : numChannels (other.numChannels),
         size (other.size),
         allocatedBytes (other.allocatedBytes)
    {
        if (allocatedBytes == 0)
        {
            allocateChannels (other.channels, 0);
        }
        else
        {
            allocateData();

            if (other.isClear)
            {
                clear();
            }
            else
            {
                for (int i = 0; i < numChannels; ++i)
                    carla_copyFloats (channels[i], other.channels[i], size);
            }
        }
    }

    /** Copies another buffer onto this one.
        This buffer's size will be changed to that of the other buffer.
    */
    AudioSampleBuffer& operator= (const AudioSampleBuffer& other) noexcept
    {
        if (this != &other)
        {
            setSize (other.getNumChannels(), other.getNumSamples(), false, false, false);

            if (other.isClear)
            {
                clear();
            }
            else
            {
                isClear = false;

                for (int i = 0; i < numChannels; ++i)
                    carla_copyFloats (channels[i], other.channels[i], size);
            }
        }

        return *this;
    }

    /** Destructor.
        This will free any memory allocated by the buffer.
    */
    ~AudioSampleBuffer() noexcept {}

   #if WATER_COMPILER_SUPPORTS_MOVE_SEMANTICS
    /** Move constructor */
    AudioSampleBuffer (AudioSampleBuffer&& other) noexcept
        : numChannels (other.numChannels),
          size (other.size),
          allocatedBytes (other.allocatedBytes),
          channels (other.channels),
          allocatedData (static_cast<HeapBlock<char, true>&&> (other.allocatedData)),
          isClear (other.isClear)
    {
        memcpy (preallocatedChannelSpace, other.preallocatedChannelSpace, sizeof (preallocatedChannelSpace));
        other.numChannels = 0;
        other.size = 0;
        other.allocatedBytes = 0;
    }

    /** Move assignment */
    AudioSampleBuffer& operator= (AudioSampleBuffer&& other) noexcept
    {
        numChannels = other.numChannels;
        size = other.size;
        allocatedBytes = other.allocatedBytes;
        channels = other.channels;
        allocatedData = static_cast<HeapBlock<char, true>&&> (other.allocatedData);
        isClear = other.isClear;
        memcpy (preallocatedChannelSpace, other.preallocatedChannelSpace, sizeof (preallocatedChannelSpace));
        other.numChannels = 0;
        other.size = 0;
        other.allocatedBytes = 0;
        return *this;
    }
   #endif

    //==============================================================================
    /** Returns the number of channels of audio data that this buffer contains.
        @see getSampleData
    */
    int getNumChannels() const noexcept                             { return numChannels; }

    /** Returns the number of samples allocated in each of the buffer's channels.
        @see getSampleData
    */
    int getNumSamples() const noexcept                              { return size; }

    /** Returns a pointer to an array of read-only samples in one of the buffer's channels.
        For speed, this doesn't check whether the channel number is out of range,
        so be careful when using it!
        If you need to write to the data, do NOT call this method and const_cast the
        result! Instead, you must call getWritePointer so that the buffer knows you're
        planning on modifying the data.
    */
    const float* getReadPointer (int channelNumber) const noexcept
    {
        jassert (isPositiveAndBelow (channelNumber, numChannels));
        return channels [channelNumber];
    }

    /** Returns a pointer to an array of read-only samples in one of the buffer's channels.
        For speed, this doesn't check whether the channel number or index are out of range,
        so be careful when using it!
        If you need to write to the data, do NOT call this method and const_cast the
        result! Instead, you must call getWritePointer so that the buffer knows you're
        planning on modifying the data.
    */
    const float* getReadPointer (int channelNumber, int sampleIndex) const noexcept
    {
        jassert (isPositiveAndBelow (channelNumber, numChannels));
        jassert (isPositiveAndBelow (sampleIndex, size));
        return channels [channelNumber] + sampleIndex;
    }

    /** Returns a writeable pointer to one of the buffer's channels.
        For speed, this doesn't check whether the channel number is out of range,
        so be careful when using it!
        Note that if you're not planning on writing to the data, you should always
        use getReadPointer instead.
    */
    float* getWritePointer (int channelNumber) noexcept
    {
        jassert (isPositiveAndBelow (channelNumber, numChannels));
        isClear = false;
        return channels [channelNumber];
    }

    /** Returns a writeable pointer to one of the buffer's channels.
        For speed, this doesn't check whether the channel number or index are out of range,
        so be careful when using it!
        Note that if you're not planning on writing to the data, you should
        use getReadPointer instead.
    */
    float* getWritePointer (int channelNumber, int sampleIndex) noexcept
    {
        jassert (isPositiveAndBelow (channelNumber, numChannels));
        jassert (isPositiveAndBelow (sampleIndex, size));
        isClear = false;
        return channels [channelNumber] + sampleIndex;
    }

    /** Returns an array of pointers to the channels in the buffer.

        Don't modify any of the pointers that are returned, and bear in mind that
        these will become invalid if the buffer is resized.
    */
    const float** getArrayOfReadPointers() const noexcept            { return const_cast<const float**> (channels); }

    /** Returns an array of pointers to the channels in the buffer.

        Don't modify any of the pointers that are returned, and bear in mind that
        these will become invalid if the buffer is resized.
    */
    float** getArrayOfWritePointers() noexcept                       { isClear = false; return channels; }

    //==============================================================================
    /** Changes the buffer's size or number of channels.

        This can expand or contract the buffer's length, and add or remove channels.

        If keepExistingContent is true, it will try to preserve as much of the
        old data as it can in the new buffer.

        If clearExtraSpace is true, then any extra channels or space that is
        allocated will be also be cleared. If false, then this space is left
        uninitialised.

        If avoidReallocating is true, then changing the buffer's size won't reduce the
        amount of memory that is currently allocated (but it will still increase it if
        the new size is bigger than the amount it currently has). If this is false, then
        a new allocation will be done so that the buffer uses takes up the minimum amount
        of memory that it needs.

        If the required memory can't be allocated, this will throw a std::bad_alloc exception.
    */
    void setSize (int newNumChannels,
                  int newNumSamples,
                  bool keepExistingContent = false,
                  bool clearExtraSpace = false,
                  bool avoidReallocating = false) noexcept
    {
        jassert (newNumChannels >= 0);
        jassert (newNumSamples >= 0);

        if (newNumSamples != size || newNumChannels != numChannels)
        {
            const size_t allocatedSamplesPerChannel = ((size_t) newNumSamples + 3) & ~3u;
            const size_t channelListSize = ((sizeof (float*) * (size_t) (newNumChannels + 1)) + 15) & ~15u;
            const size_t newTotalBytes = ((size_t) newNumChannels * (size_t) allocatedSamplesPerChannel * sizeof (float))
                                            + channelListSize + 32;

            if (keepExistingContent)
            {
                HeapBlock<char, true> newData;
                newData.allocate (newTotalBytes, clearExtraSpace || isClear);

                const size_t numSamplesToCopy = (size_t) jmin (newNumSamples, size);

                float** const newChannels = reinterpret_cast<float**> (newData.getData());
                float* newChan = reinterpret_cast<float*> (newData + channelListSize);

                for (int j = 0; j < newNumChannels; ++j)
                {
                    newChannels[j] = newChan;
                    newChan += allocatedSamplesPerChannel;
                }

                if (! isClear)
                {
                    const int numChansToCopy = jmin (numChannels, newNumChannels);
                    for (int i = 0; i < numChansToCopy; ++i)
                        carla_copyFloats (newChannels[i], channels[i], (int) numSamplesToCopy);
                }

                allocatedData.swapWith (newData);
                allocatedBytes = newTotalBytes;
                channels = newChannels;
            }
            else
            {
                if (avoidReallocating && allocatedBytes >= newTotalBytes)
                {
                    if (clearExtraSpace || isClear)
                        allocatedData.clear (newTotalBytes);
                }
                else
                {
                    allocatedBytes = newTotalBytes;
                    allocatedData.allocate (newTotalBytes, clearExtraSpace || isClear);
                    channels = reinterpret_cast<float**> (allocatedData.getData());
                }

                float* chan = reinterpret_cast<float*> (allocatedData + channelListSize);
                for (int i = 0; i < newNumChannels; ++i)
                {
                    channels[i] = chan;
                    chan += allocatedSamplesPerChannel;
                }
            }

            channels [newNumChannels] = 0;
            size = newNumSamples;
            numChannels = newNumChannels;
        }
    }

    /** Makes this buffer point to a pre-allocated set of channel data arrays.

        There's also a constructor that lets you specify arrays like this, but this
        lets you change the channels dynamically.

        Note that if the buffer is resized or its number of channels is changed, it
        will re-allocate memory internally and copy the existing data to this new area,
        so it will then stop directly addressing this memory.

        @param dataToReferTo    a pre-allocated array containing pointers to the data
                                for each channel that should be used by this buffer. The
                                buffer will only refer to this memory, it won't try to delete
                                it when the buffer is deleted or resized.
        @param newNumChannels   the number of channels to use - this must correspond to the
                                number of elements in the array passed in
        @param newNumSamples    the number of samples to use - this must correspond to the
                                size of the arrays passed in
    */
    void setDataToReferTo (float** dataToReferTo,
                           const int newNumChannels,
                           const int newNumSamples) noexcept
    {
        jassert (dataToReferTo != nullptr);
        jassert (newNumChannels >= 0 && newNumSamples >= 0);

        if (allocatedBytes != 0)
        {
            allocatedBytes = 0;
            allocatedData.free();
        }

        numChannels = newNumChannels;
        size = newNumSamples;

        allocateChannels (dataToReferTo, 0);
        jassert (! isClear);
    }

    /** Resizes this buffer to match the given one, and copies all of its content across.
        The source buffer can contain a different floating point type, so this can be used to
        convert between 32 and 64 bit float buffer types.
    */
    void makeCopyOf (const AudioSampleBuffer& other, bool avoidReallocating = false)
    {
        setSize (other.getNumChannels(), other.getNumSamples(), false, false, avoidReallocating);

        if (other.hasBeenCleared())
        {
            clear();
        }
        else
        {
            for (int chan = 0; chan < numChannels; ++chan)
            {
                float* const dest = channels[chan];
                const float* const src = other.getReadPointer (chan);

                for (int i = 0; i < size; ++i)
                    dest[i] = static_cast<float> (src[i]);
            }
        }
    }

    //==============================================================================
    /** Clears all the samples in all channels. */
    void clear() noexcept
    {
        if (! isClear)
        {
            for (int i = 0; i < numChannels; ++i)
                carla_zeroFloats (channels[i], size);

            isClear = true;
        }
    }

    /** Clears a specified region of all the channels.

        For speed, this doesn't check whether the channel and sample number
        are in-range, so be careful!
    */
    void clear (int startSample,
                int numSamples) noexcept
    {
        jassert (startSample >= 0 && startSample + numSamples <= size);

        if (! isClear)
        {
            if (startSample == 0 && numSamples == size)
                isClear = true;

            for (int i = 0; i < numChannels; ++i)
                carla_zeroFloats (channels[i] + startSample, numSamples);
        }
    }

    /** Clears a specified region of just one channel.

        For speed, this doesn't check whether the channel and sample number
        are in-range, so be careful!
    */
    void clear (int channel,
                int startSample,
                int numSamples) noexcept
    {
        jassert (isPositiveAndBelow (channel, numChannels));
        jassert (startSample >= 0 && startSample + numSamples <= size);

        if (! isClear)
            carla_zeroFloats (channels [channel] + startSample, numSamples);
    }

    /** Returns true if the buffer has been entirely cleared.
        Note that this does not actually measure the contents of the buffer - it simply
        returns a flag that is set when the buffer is cleared, and which is reset whenever
        functions like getWritePointer() are invoked. That means the method does not take
        any time, but it may return false negatives when in fact the buffer is still empty.
    */
    bool hasBeenCleared() const noexcept                            { return isClear; }

    //==============================================================================
    /** Returns a sample from the buffer.
        The channel and index are not checked - they are expected to be in-range. If not,
        an assertion will be thrown, but in a release build, you're into 'undefined behaviour'
        territory.
    */
    float getSample (int channel, int sampleIndex) const noexcept
    {
        jassert (isPositiveAndBelow (channel, numChannels));
        jassert (isPositiveAndBelow (sampleIndex, size));
        return *(channels [channel] + sampleIndex);
    }

    /** Sets a sample in the buffer.
        The channel and index are not checked - they are expected to be in-range. If not,
        an assertion will be thrown, but in a release build, you're into 'undefined behaviour'
        territory.
    */
    void setSample (int destChannel, int destSample, float newValue) noexcept
    {
        jassert (isPositiveAndBelow (destChannel, numChannels));
        jassert (isPositiveAndBelow (destSample, size));
        *(channels [destChannel] + destSample) = newValue;
        isClear = false;
    }

    /** Adds a value to a sample in the buffer.
        The channel and index are not checked - they are expected to be in-range. If not,
        an assertion will be thrown, but in a release build, you're into 'undefined behaviour'
        territory.
    */
    void addSample (int destChannel, int destSample, float valueToAdd) noexcept
    {
        jassert (isPositiveAndBelow (destChannel, numChannels));
        jassert (isPositiveAndBelow (destSample, size));
        *(channels [destChannel] + destSample) += valueToAdd;
        isClear = false;
    }

    /** Applies a gain multiple to a region of one channel.

        For speed, this doesn't check whether the channel and sample number
        are in-range, so be careful!
    */
    void applyGain (int channel,
                    int startSample,
                    int numSamples,
                    float gain) noexcept
    {
        jassert (isPositiveAndBelow (channel, numChannels));
        jassert (startSample >= 0 && startSample + numSamples <= size);

        if (gain != 1.0f && ! isClear)
        {
            float* const d = channels [channel] + startSample;

            if (carla_isZero (gain))
                carla_zeroFloats (d, numSamples);
            else
                carla_multiply (d, gain, numSamples);
        }
    }

    /** Applies a gain multiple to a region of all the channels.

        For speed, this doesn't check whether the sample numbers
        are in-range, so be careful!
    */
    void applyGain (int startSample,
                    int numSamples,
                    float gain) noexcept
    {
        for (int i = 0; i < numChannels; ++i)
            applyGain (i, startSample, numSamples, gain);
    }

    /** Applies a gain multiple to all the audio data. */
    void applyGain (float gain) noexcept
    {
        applyGain (0, size, gain);
    }

    /** Applies a range of gains to a region of a channel.

        The gain that is applied to each sample will vary from
        startGain on the first sample to endGain on the last Sample,
        so it can be used to do basic fades.

        For speed, this doesn't check whether the sample numbers
        are in-range, so be careful!
    */
    void applyGainRamp (int channel,
                        int startSample,
                        int numSamples,
                        float startGain,
                        float endGain) noexcept
    {
        if (! isClear)
        {
            if (startGain == endGain)
            {
                applyGain (channel, startSample, numSamples, startGain);
            }
            else
            {
                jassert (isPositiveAndBelow (channel, numChannels));
                jassert (startSample >= 0 && startSample + numSamples <= size);

                const float increment = (endGain - startGain) / numSamples;
                float* d = channels [channel] + startSample;

                while (--numSamples >= 0)
                {
                    *d++ *= startGain;
                    startGain += increment;
                }
            }
        }
    }

    /** Applies a range of gains to a region of all channels.

        The gain that is applied to each sample will vary from
        startGain on the first sample to endGain on the last Sample,
        so it can be used to do basic fades.

        For speed, this doesn't check whether the sample numbers
        are in-range, so be careful!
    */
    void applyGainRamp (int startSample,
                        int numSamples,
                        float startGain,
                        float endGain) noexcept
    {
        for (int i = 0; i < numChannels; ++i)
            applyGainRamp (i, startSample, numSamples, startGain, endGain);
    }

    /** Adds samples from another buffer to this one.

        @param destChannel          the channel within this buffer to add the samples to
        @param destStartSample      the start sample within this buffer's channel
        @param source               the source buffer to add from
        @param sourceChannel        the channel within the source buffer to read from
        @param sourceStartSample    the offset within the source buffer's channel to start reading samples from
        @param numSamples           the number of samples to process
        @param gainToApplyToSource  an optional gain to apply to the source samples before they are
                                    added to this buffer's samples

        @see copyFrom
    */
    void addFrom (int destChannel,
                  int destStartSample,
                  const AudioSampleBuffer& source,
                  int sourceChannel,
                  int sourceStartSample,
                  int numSamples,
                  float gainToApplyToSource = (float) 1) noexcept
    {
        jassert (&source != this || sourceChannel != destChannel);
        jassert (isPositiveAndBelow (destChannel, numChannels));
        jassert (destStartSample >= 0 && destStartSample + numSamples <= size);
        jassert (isPositiveAndBelow (sourceChannel, source.numChannels));
        jassert (sourceStartSample >= 0 && sourceStartSample + numSamples <= source.size);

        if (gainToApplyToSource != 0.0f && numSamples > 0 && ! source.isClear)
        {
            float* const d = channels [destChannel] + destStartSample;
            const float* const s  = source.channels [sourceChannel] + sourceStartSample;

            if (isClear)
            {
                isClear = false;

                if (gainToApplyToSource != 1.0f)
                    carla_copyWithMultiply (d, s, gainToApplyToSource, numSamples);
                else
                    carla_copyFloats (d, s, numSamples);
            }
            else
            {
                if (gainToApplyToSource != 1.0f)
                    carla_addWithMultiply (d, s, gainToApplyToSource, numSamples);
                else
                    carla_add (d, s, numSamples);
            }
        }
    }


    /** Adds samples from an array of floats to one of the channels.

        @param destChannel          the channel within this buffer to add the samples to
        @param destStartSample      the start sample within this buffer's channel
        @param source               the source data to use
        @param numSamples           the number of samples to process
        @param gainToApplyToSource  an optional gain to apply to the source samples before they are
                                    added to this buffer's samples

        @see copyFrom
    */
    void addFrom (int destChannel,
                  int destStartSample,
                  const float* source,
                  int numSamples,
                  float gainToApplyToSource = (float) 1) noexcept
    {
        jassert (isPositiveAndBelow (destChannel, numChannels));
        jassert (destStartSample >= 0 && destStartSample + numSamples <= size);
        jassert (source != nullptr);

        if (gainToApplyToSource != 0.0f && numSamples > 0)
        {
            float* const d = channels [destChannel] + destStartSample;

            if (isClear)
            {
                isClear = false;

                if (gainToApplyToSource != 1.0f)
                    carla_copyWithMultiply (d, source, gainToApplyToSource, numSamples);
                else
                    carla_copyFloats (d, source, numSamples);
            }
            else
            {
                if (gainToApplyToSource != 1.0f)
                    carla_addWithMultiply (d, source, gainToApplyToSource, numSamples);
                else
                    carla_add (d, source, numSamples);
            }
        }
    }


    /** Adds samples from an array of floats, applying a gain ramp to them.

        @param destChannel          the channel within this buffer to add the samples to
        @param destStartSample      the start sample within this buffer's channel
        @param source               the source data to use
        @param numSamples           the number of samples to process
        @param startGain            the gain to apply to the first sample (this is multiplied with
                                    the source samples before they are added to this buffer)
        @param endGain              the gain to apply to the final sample. The gain is linearly
                                    interpolated between the first and last samples.
    */
    void addFromWithRamp (int destChannel,
                          int destStartSample,
                          const float* source,
                          int numSamples,
                          float startGain,
                          float endGain) noexcept
    {
        jassert (isPositiveAndBelow (destChannel, numChannels));
        jassert (destStartSample >= 0 && destStartSample + numSamples <= size);
        jassert (source != nullptr);

        if (startGain == endGain)
        {
            addFrom (destChannel, destStartSample, source, numSamples, startGain);
        }
        else
        {
            if (numSamples > 0 && (startGain != 0.0f || endGain != 0.0f))
            {
                isClear = false;
                const float increment = (endGain - startGain) / numSamples;
                float* d = channels [destChannel] + destStartSample;

                while (--numSamples >= 0)
                {
                    *d++ += startGain * *source++;
                    startGain += increment;
                }
            }
        }
    }

    /** Copies samples from another buffer to this one.

        @param destChannel          the channel within this buffer to copy the samples to
        @param destStartSample      the start sample within this buffer's channel
        @param source               the source buffer to read from
        @param sourceChannel        the channel within the source buffer to read from
        @param sourceStartSample    the offset within the source buffer's channel to start reading samples from
        @param numSamples           the number of samples to process

        @see addFrom
    */
    void copyFrom (int destChannel,
                   int destStartSample,
                   const AudioSampleBuffer& source,
                   int sourceChannel,
                   int sourceStartSample,
                   int numSamples) noexcept
    {
        jassert (&source != this || sourceChannel != destChannel);
        jassert (isPositiveAndBelow (destChannel, numChannels));
        jassert (destStartSample >= 0 && destStartSample + numSamples <= size);
        jassert (isPositiveAndBelow (sourceChannel, source.numChannels));
        jassert (sourceStartSample >= 0 && sourceStartSample + numSamples <= source.size);

        if (numSamples > 0)
        {
            if (source.isClear)
            {
                if (! isClear)
                    carla_zeroFloats (channels [destChannel] + destStartSample, numSamples);
            }
            else
            {
                isClear = false;
                carla_copyFloats (channels [destChannel] + destStartSample,
                                             source.channels [sourceChannel] + sourceStartSample,
                                             numSamples);
            }
        }
    }

    /** Copies samples from an array of floats into one of the channels.

        @param destChannel          the channel within this buffer to copy the samples to
        @param destStartSample      the start sample within this buffer's channel
        @param source               the source buffer to read from
        @param numSamples           the number of samples to process

        @see addFrom
    */
    void copyFrom (int destChannel,
                   int destStartSample,
                   const float* source,
                   int numSamples) noexcept
    {
        jassert (isPositiveAndBelow (destChannel, numChannels));
        jassert (destStartSample >= 0 && destStartSample + numSamples <= size);
        jassert (source != nullptr);

        if (numSamples > 0)
        {
            isClear = false;
            carla_copyFloats (channels [destChannel] + destStartSample, source, numSamples);
        }
    }

    /** Copies samples from an array of floats into one of the channels, applying a gain to it.

        @param destChannel          the channel within this buffer to copy the samples to
        @param destStartSample      the start sample within this buffer's channel
        @param source               the source buffer to read from
        @param numSamples           the number of samples to process
        @param gain                 the gain to apply

        @see addFrom
    */
    void copyFrom (int destChannel,
                   int destStartSample,
                   const float* source,
                   int numSamples,
                   float gain) noexcept
    {
        jassert (isPositiveAndBelow (destChannel, numChannels));
        jassert (destStartSample >= 0 && destStartSample + numSamples <= size);
        jassert (source != nullptr);

        if (numSamples > 0)
        {
            float* const d = channels [destChannel] + destStartSample;

            if (gain != 1.0f)
            {
                if (gain == 0)
                {
                    if (! isClear)
                        carla_zeroFloats (d, numSamples);
                }
                else
                {
                    isClear = false;
                    carla_copyWithMultiply (d, source, gain, numSamples);
                }
            }
            else
            {
                isClear = false;
                carla_copyFloats (d, source, numSamples);
            }
        }
    }

    /** Copies samples from an array of floats into one of the channels, applying a gain ramp.

        @param destChannel          the channel within this buffer to copy the samples to
        @param destStartSample      the start sample within this buffer's channel
        @param source               the source buffer to read from
        @param numSamples           the number of samples to process
        @param startGain            the gain to apply to the first sample (this is multiplied with
                                    the source samples before they are copied to this buffer)
        @param endGain              the gain to apply to the final sample. The gain is linearly
                                    interpolated between the first and last samples.

        @see addFrom
    */
    void copyFromWithRamp (int destChannel,
                           int destStartSample,
                           const float* source,
                           int numSamples,
                           float startGain,
                           float endGain) noexcept
    {
        jassert (isPositiveAndBelow (destChannel, numChannels));
        jassert (destStartSample >= 0 && destStartSample + numSamples <= size);
        jassert (source != nullptr);

        if (startGain == endGain)
        {
            copyFrom (destChannel, destStartSample, source, numSamples, startGain);
        }
        else
        {
            if (numSamples > 0 && (startGain != 0.0f || endGain != 0.0f))
            {
                isClear = false;
                const float increment = (endGain - startGain) / numSamples;
                float* d = channels [destChannel] + destStartSample;

                while (--numSamples >= 0)
                {
                    *d++ = startGain * *source++;
                    startGain += increment;
                }
            }
        }
    }

#if 0
    /** Returns a Range indicating the lowest and highest sample values in a given section.

        @param channel      the channel to read from
        @param startSample  the start sample within the channel
        @param numSamples   the number of samples to check
    */
    Range<float> findMinMax (int channel,
                            int startSample,
                            int numSamples) const noexcept
    {
        jassert (isPositiveAndBelow (channel, numChannels));
        jassert (startSample >= 0 && startSample + numSamples <= size);

        if (isClear)
            return Range<float>();

        return FloatVectorOperations::findMinAndMax (channels [channel] + startSample, numSamples);
    }


    /** Finds the highest absolute sample value within a region of a channel. */
    float getMagnitude (int channel,
                       int startSample,
                       int numSamples) const noexcept
    {
        jassert (isPositiveAndBelow (channel, numChannels));
        jassert (startSample >= 0 && startSample + numSamples <= size);

        if (isClear)
            return 0.0f;

        const Range<float> r (findMinMax (channel, startSample, numSamples));

        return jmax (r.getStart(), -r.getStart(), r.getEnd(), -r.getEnd());
    }

    /** Finds the highest absolute sample value within a region on all channels. */
    float getMagnitude (int startSample,
                       int numSamples) const noexcept
    {
        float mag = 0.0f;

        if (! isClear)
            for (int i = 0; i < numChannels; ++i)
                mag = jmax (mag, getMagnitude (i, startSample, numSamples));

        return mag;
    }

    /** Returns the root mean squared level for a region of a channel. */
    float getRMSLevel (int channel,
                      int startSample,
                      int numSamples) const noexcept
    {
        jassert (isPositiveAndBelow (channel, numChannels));
        jassert (startSample >= 0 && startSample + numSamples <= size);

        if (numSamples <= 0 || channel < 0 || channel >= numChannels || isClear)
            return 0.0f;

        const float* const data = channels [channel] + startSample;
        double sum = 0.0;

        for (int i = 0; i < numSamples; ++i)
        {
            const float sample = data [i];
            sum += sample * sample;
        }

        return (float) std::sqrt (sum / numSamples);
    }

    /** Reverses a part of a channel. */
    void reverse (int channel, int startSample, int numSamples) const noexcept
    {
        jassert (isPositiveAndBelow (channel, numChannels));
        jassert (startSample >= 0 && startSample + numSamples <= size);

        if (! isClear)
            std::reverse (channels[channel] + startSample,
                          channels[channel] + startSample + numSamples);
    }

    /** Reverses a part of the buffer. */
    void reverse (int startSample, int numSamples) const noexcept
    {
        for (int i = 0; i < numChannels; ++i)
            reverse (i, startSample, numSamples);
    }
#endif

private:
    //==============================================================================
    int numChannels, size;
    size_t allocatedBytes;
    float** channels;
    HeapBlock<char, true> allocatedData;
    float* preallocatedChannelSpace [32];
    bool isClear;

    void allocateData()
    {
        const size_t channelListSize = sizeof (float*) * (size_t) (numChannels + 1);
        allocatedBytes = (size_t) numChannels * (size_t) size * sizeof (float) + channelListSize + 32;
        allocatedData.malloc (allocatedBytes);
        channels = reinterpret_cast<float**> (allocatedData.getData());

        float* chan = (float*) (allocatedData + channelListSize);
        for (int i = 0; i < numChannels; ++i)
        {
            channels[i] = chan;
            chan += size;
        }

        channels [numChannels] = nullptr;
        isClear = false;
    }

    void allocateChannels (float* const* const dataToReferTo, int offset)
    {
        jassert (offset >= 0);

        // (try to avoid doing a malloc here, as that'll blow up things like Pro-Tools)
        if (numChannels < (int) numElementsInArray (preallocatedChannelSpace))
        {
            channels = static_cast<float**> (preallocatedChannelSpace);
        }
        else
        {
            allocatedData.malloc ((size_t) numChannels + 1, sizeof (float*));
            channels = reinterpret_cast<float**> (allocatedData.getData());
        }

        for (int i = 0; i < numChannels; ++i)
        {
            // you have to pass in the same number of valid pointers as numChannels
            jassert (dataToReferTo[i] != nullptr);

            channels[i] = dataToReferTo[i] + offset;
        }

        channels [numChannels] = nullptr;
        isClear = false;
    }
};

}

#endif // WATER_AUDIOSAMPLEBUFFER_H_INCLUDED
