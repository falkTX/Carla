/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2017-2022 Filipe Coelho <falktx@falktx.com>

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

#ifndef WATER_MEMORYOUTPUTSTREAM_H_INCLUDED
#define WATER_MEMORYOUTPUTSTREAM_H_INCLUDED

#include "OutputStream.h"
#include "../memory/MemoryBlock.h"

namespace water {

//==============================================================================
/**
    Writes data to an internal memory buffer, which grows as required.

    The data that was written into the stream can then be accessed later as
    a contiguous block of memory.
*/
class MemoryOutputStream  : public OutputStream
{
public:
    //==============================================================================
    /** Creates an empty memory stream, ready to be written into.
        @param initialSize  the intial amount of capacity to allocate for writing into
    */
    MemoryOutputStream (size_t initialSize = 256);

    /** Creates a memory stream for writing into into a pre-existing MemoryBlock object.

        Note that the destination block will always be larger than the amount of data
        that has been written to the stream, because the MemoryOutputStream keeps some
        spare capactity at its end. To trim the block's size down to fit the actual
        data, call flush(), or delete the MemoryOutputStream.

        @param memoryBlockToWriteTo             the block into which new data will be written.
        @param appendToExistingBlockContent     if this is true, the contents of the block will be
                                                kept, and new data will be appended to it. If false,
                                                the block will be cleared before use
    */
    MemoryOutputStream (MemoryBlock& memoryBlockToWriteTo,
                        bool appendToExistingBlockContent);

    /** Destructor.
        This will free any data that was written to it.
    */
    ~MemoryOutputStream();

    //==============================================================================
    /** Returns a pointer to the data that has been written to the stream.
        @see getDataSize
    */
    const void* getData() const noexcept;

    /** Returns a pointer to the data that has been written to the stream and releases the buffer pointer.
        @see getDataSize
    */
    void* getDataAndRelease() noexcept;

    /** Returns the number of bytes of data that have been written to the stream.
        @see getData
    */
    size_t getDataSize() const noexcept                 { return size; }

    /** Resets the stream, clearing any data that has been written to it so far. */
    void reset() noexcept;

    /** Increases the internal storage capacity to be able to contain at least the specified
        amount of data without needing to be resized.
    */
    void preallocate (size_t bytesToPreallocate);

    /** Appends the utf-8 bytes for a unicode character */
    bool appendUTF8Char (water_uchar character);

    /** Returns a String created from the (UTF8) data that has been written to the stream. */
    String toUTF8() const;

    /** Attempts to detect the encoding of the data and convert it to a string.
        @see String::createStringFromData
    */
    String toString() const;

    /** Returns a copy of the stream's data as a memory block. */
    MemoryBlock getMemoryBlock() const;

    //==============================================================================
    /** If the stream is writing to a user-supplied MemoryBlock, this will trim any excess
        capacity off the block, so that its length matches the amount of actual data that
        has been written so far.
    */
    void flush() override;

    bool write (const void*, size_t) override;
    int64 getPosition() override                                 { return (int64) position; }
    bool setPosition (int64) override;
    int64 writeFromInputStream (InputStream&, int64 maxNumBytesToWrite) override;
    bool writeRepeatedByte (uint8 byte, size_t numTimesToRepeat) override;

private:
    //==============================================================================
    MemoryBlock internalBlock;
    MemoryBlock& blockToUse;
    size_t position, size;
    bool usingInternalBlock;

    void trimExternalBlockSize();
    char* prepareToWrite (size_t);

    CARLA_DECLARE_NON_COPYABLE (MemoryOutputStream)
};

/** Copies all the data that has been written to a MemoryOutputStream into another stream. */
OutputStream& operator<< (OutputStream& stream, const MemoryOutputStream& streamToRead);

}

#endif // WATER_MEMORYOUTPUTSTREAM_H_INCLUDED
