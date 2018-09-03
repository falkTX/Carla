/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2017-2018 Filipe Coelho <falktx@falktx.com>

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
                       int numSamplesToAllocate,
                       bool clearData = false) noexcept
       : numChannels (numChannelsToAllocate),
         size (numSamplesToAllocate),
         allocatedBytes (0),
         isClear (false)
    {
        CARLA_SAFE_ASSERT_RETURN (size >= 0,);
        CARLA_SAFE_ASSERT_RETURN (numChannels >= 0,);

        allocateData (clearData);
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
          allocatedBytes (0),
          isClear (false)
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
          allocatedData (static_cast<HeapBlock<char>&&> (other.allocatedData)),
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
        allocatedData = static_cast<HeapBlock<char>&&> (other.allocatedData);
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
        CARLA_SAFE_ASSERT_RETURN (isPositiveAndBelow (channelNumber, numChannels), nullptr);
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
        CARLA_SAFE_ASSERT_RETURN (isPositiveAndBelow (channelNumber, numChannels), nullptr);
        CARLA_SAFE_ASSERT_RETURN (isPositiveAndBelow (sampleIndex, size), nullptr);
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
        CARLA_SAFE_ASSERT_RETURN (isPositiveAndBelow (channelNumber, numChannels), nullptr);
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
        CARLA_SAFE_ASSERT_RETURN (isPositiveAndBelow (channelNumber, numChannels), nullptr);
        CARLA_SAFE_ASSERT_RETURN (isPositiveAndBelow (sampleIndex, size), nullptr);
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

        This can expand the buffer's length, and add or remove channels.
    */
    bool setSize (int newNumChannels, int newNumSamples) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN (newNumChannels >= 0, false);
        CARLA_SAFE_ASSERT_RETURN (newNumSamples >= 0, false);

        if (newNumSamples != size || newNumChannels != numChannels)
        {
            const size_t allocatedSamplesPerChannel = ((size_t) newNumSamples + 3) & ~3u;
            const size_t channelListSize = ((sizeof (float*) * (size_t) (newNumChannels + 1)) + 15) & ~15u;
            const size_t newTotalBytes = ((size_t) newNumChannels * (size_t) allocatedSamplesPerChannel * sizeof (float))
                                            + channelListSize + 32;

            if (allocatedBytes >= newTotalBytes)
            {
                if (isClear)
                    allocatedData.clear (newTotalBytes);
            }
            else
            {
                CARLA_SAFE_ASSERT_RETURN (allocatedData.allocate (newTotalBytes, isClear), false);
                allocatedBytes = newTotalBytes;
                channels = reinterpret_cast<float**> (allocatedData.getData());
            }

            float* chan = reinterpret_cast<float*> (allocatedData + channelListSize);
            for (int i = 0; i < newNumChannels; ++i)
            {
                channels[i] = chan;
                chan += allocatedSamplesPerChannel;
            }

            channels [newNumChannels] = 0;
            size = newNumSamples;
            numChannels = newNumChannels;
        }

        return true;
    }

    //==============================================================================
    /** Changes the buffer's size in a real-time safe manner.

        Returns true if the required memory is available.
    */
    bool setSizeRT (int newNumSamples) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN (newNumSamples >= 0, false);
        CARLA_SAFE_ASSERT_RETURN (numChannels >= 0, false);

        if (newNumSamples != size)
        {
            const size_t allocatedSamplesPerChannel = ((size_t) newNumSamples + 3) & ~3u;
            const size_t channelListSize = ((sizeof (float*) * (size_t) (numChannels + 1)) + 15) & ~15u;
            const size_t newTotalBytes = ((size_t) numChannels * (size_t) allocatedSamplesPerChannel * sizeof (float))
                                            + channelListSize + 32;

            CARLA_SAFE_ASSERT_RETURN(allocatedBytes >= newTotalBytes, false);

            float* chan = reinterpret_cast<float*> (allocatedData + channelListSize);
            for (int i = 0; i < numChannels; ++i)
            {
                channels[i] = chan;
                chan += allocatedSamplesPerChannel;
            }

            size = newNumSamples;
        }

        return true;
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
    bool setDataToReferTo (float** dataToReferTo,
                           const int newNumChannels,
                           const int newNumSamples) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(dataToReferTo != nullptr, false);
        CARLA_SAFE_ASSERT_INT2_RETURN(newNumChannels >= 0 && newNumSamples >= 0, newNumChannels, newNumSamples, false);

        if (allocatedBytes != 0)
        {
            allocatedBytes = 0;
            allocatedData.free();
        }

        numChannels = newNumChannels;
        size = newNumSamples;

        return allocateChannels (dataToReferTo, 0);
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
        CARLA_SAFE_ASSERT_INT2_RETURN(startSample >= 0 && startSample + numSamples <= size, numSamples, size,);

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
        CARLA_SAFE_ASSERT_INT2_RETURN(isPositiveAndBelow(channel, numChannels), channel, numChannels,);
        CARLA_SAFE_ASSERT_INT2_RETURN(startSample >= 0 && startSample + numSamples <= size, numSamples, size,);

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
        CARLA_SAFE_ASSERT_INT2_RETURN(&source != this || sourceChannel != destChannel, sourceChannel, destChannel,);
        CARLA_SAFE_ASSERT_INT2_RETURN(isPositiveAndBelow(destChannel, numChannels), destChannel, numChannels,);
        CARLA_SAFE_ASSERT_INT2_RETURN(isPositiveAndBelow(sourceChannel, source.numChannels), sourceChannel, source.numChannels,);
        CARLA_SAFE_ASSERT_INT2_RETURN(destStartSample >= 0 && destStartSample + numSamples <= size, numSamples, size,);
        CARLA_SAFE_ASSERT_INT2_RETURN(sourceStartSample >= 0 && sourceStartSample + numSamples <= source.size, numSamples, source.size,);

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
        CARLA_SAFE_ASSERT_INT2_RETURN(isPositiveAndBelow(destChannel, numChannels), destChannel, numChannels,);
        CARLA_SAFE_ASSERT_INT2_RETURN(destStartSample >= 0 && destStartSample + numSamples <= size, numSamples, size,);
        CARLA_SAFE_ASSERT_RETURN(source != nullptr,);

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
        CARLA_SAFE_ASSERT_INT2_RETURN(&source != this || sourceChannel != destChannel, sourceChannel, destChannel,);
        CARLA_SAFE_ASSERT_INT2_RETURN(isPositiveAndBelow(destChannel, numChannels), destChannel, numChannels,);
        CARLA_SAFE_ASSERT_INT2_RETURN(isPositiveAndBelow(sourceChannel, source.numChannels), sourceChannel, source.numChannels,);
        CARLA_SAFE_ASSERT_INT2_RETURN(destStartSample >= 0 && destStartSample + numSamples <= size, numSamples, size,);
        CARLA_SAFE_ASSERT_INT2_RETURN(sourceStartSample >= 0 && sourceStartSample + numSamples <= source.size, numSamples, source.size,);

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
        CARLA_SAFE_ASSERT_INT2_RETURN(isPositiveAndBelow(destChannel, numChannels), destChannel, numChannels,);
        CARLA_SAFE_ASSERT_INT2_RETURN(destStartSample >= 0 && destStartSample + numSamples <= size, numSamples, size,);
        CARLA_SAFE_ASSERT_RETURN(source != nullptr,);

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
        CARLA_SAFE_ASSERT_INT2_RETURN(isPositiveAndBelow(destChannel, numChannels), destChannel, numChannels,);
        CARLA_SAFE_ASSERT_INT2_RETURN(destStartSample >= 0 && destStartSample + numSamples <= size, numSamples, size,);
        CARLA_SAFE_ASSERT_RETURN(source != nullptr,);

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

    /** Copies samples from an array of floats into one of the channels.

        @param destChannel          the channel within this buffer to copy the samples to
        @param destStartSample      the start sample within this buffer's channel
        @param source               the source buffer to read from
        @param numSamples           the number of samples to process

        @see addFrom
    */
    void copyFromInterleavedSource (int destChannel,
                                    const float* source,
                                    int totalNumSamples) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(isPositiveAndBelow(destChannel, numChannels),);
        CARLA_SAFE_ASSERT_RETURN(source != nullptr,);

        if (const int numSamples = totalNumSamples / numChannels)
        {
            CARLA_SAFE_ASSERT_RETURN(numSamples <= size,);

            isClear = false;
            float* d = channels [destChannel];

            for (int i=numSamples; --i >= 0;)
                d[i] = source[i * numChannels + destChannel];
        }
    }

private:
    //==============================================================================
    int numChannels, size;
    size_t allocatedBytes;
    float** channels;
    HeapBlock<char> allocatedData;
    float* preallocatedChannelSpace [32];
    bool isClear;

    bool allocateData (bool clearData = false)
    {
        const size_t channelListSize = sizeof (float*) * (size_t) (numChannels + 1);
        const size_t nextAllocatedBytes = (size_t) numChannels * (size_t) size * sizeof (float) + channelListSize + 32;
        CARLA_SAFE_ASSERT_RETURN (allocatedData.allocate (nextAllocatedBytes, clearData), false);

        allocatedBytes = nextAllocatedBytes;
        channels = reinterpret_cast<float**> (allocatedData.getData());

        float* chan = (float*) (allocatedData + channelListSize);
        for (int i = 0; i < numChannels; ++i)
        {
            channels[i] = chan;
            chan += size;
        }

        channels [numChannels] = nullptr;
        isClear = clearData;
        return true;
    }

    bool allocateChannels (float* const* const dataToReferTo, int offset)
    {
        CARLA_SAFE_ASSERT_RETURN (offset >= 0, false);

        // (try to avoid doing a malloc here, as that'll blow up things like Pro-Tools)
        if (numChannels < (int) numElementsInArray (preallocatedChannelSpace))
        {
            channels = static_cast<float**> (preallocatedChannelSpace);
        }
        else
        {
            CARLA_SAFE_ASSERT_RETURN( allocatedData.malloc ((size_t) numChannels + 1, sizeof (float*)), false);
            channels = reinterpret_cast<float**> (allocatedData.getData());
        }

        for (int i = 0; i < numChannels; ++i)
        {
            // you have to pass in the same number of valid pointers as numChannels
            CARLA_SAFE_ASSERT_CONTINUE (dataToReferTo[i] != nullptr);

            channels[i] = dataToReferTo[i] + offset;
        }

        channels [numChannels] = nullptr;
        isClear = false;
        return true;
    }
};

}

#endif // WATER_AUDIOSAMPLEBUFFER_H_INCLUDED
