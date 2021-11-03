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

#include "MemoryOutputStream.h"
#include "InputStream.h"

namespace water {

MemoryOutputStream::MemoryOutputStream (const size_t initialSize)
  : internalBlock(), blockToUse (internalBlock),
    position (0), size (0),
    usingInternalBlock (true)
{
    internalBlock.setSize (initialSize, false);
}

MemoryOutputStream::MemoryOutputStream (MemoryBlock& memoryBlockToWriteTo,
                                        const bool appendToExistingBlockContent)
  : internalBlock(), blockToUse (memoryBlockToWriteTo),
    position (0), size (0),
    usingInternalBlock (false)
{
    if (appendToExistingBlockContent)
        position = size = memoryBlockToWriteTo.getSize();
}

MemoryOutputStream::~MemoryOutputStream()
{
    trimExternalBlockSize();
}

void MemoryOutputStream::flush()
{
    trimExternalBlockSize();
}

void MemoryOutputStream::trimExternalBlockSize()
{
    if (! usingInternalBlock)
        blockToUse.setSize (size, false);
}

void MemoryOutputStream::preallocate (const size_t bytesToPreallocate)
{
    blockToUse.ensureSize (bytesToPreallocate + 1);
}

void MemoryOutputStream::reset() noexcept
{
    position = 0;
    size = 0;
}

char* MemoryOutputStream::prepareToWrite (size_t numBytes)
{
    CARLA_SAFE_ASSERT_RETURN ((ssize_t) numBytes >= 0, nullptr);

    const size_t storageNeeded = position + numBytes;

    if (storageNeeded >= blockToUse.getSize())
        blockToUse.ensureSize ((storageNeeded + jmin (storageNeeded / 2, (size_t) (1024 * 1024)) + 32) & ~31u);

    char* const data = static_cast<char*> (blockToUse.getData());
    char* const writePointer = data + position;
    position += numBytes;
    size = jmax (size, position);
    return writePointer;
}

bool MemoryOutputStream::write (const void* const buffer, size_t howMany)
{
    CARLA_SAFE_ASSERT_RETURN (buffer != nullptr, false);

    if (howMany == 0)
        return true;

    if (char* const dest = prepareToWrite (howMany))
    {
        std::memcpy (dest, buffer, howMany);
        return true;
    }

    return false;
}

bool MemoryOutputStream::writeRepeatedByte (uint8 byte, size_t howMany)
{
    if (howMany == 0)
        return true;

    if (char* dest = prepareToWrite (howMany))
    {
        memset (dest, byte, howMany);
        return true;
    }

    return false;
}

bool MemoryOutputStream::appendUTF8Char (water_uchar c)
{
    if (char* dest = prepareToWrite (CharPointer_UTF8::getBytesRequiredFor (c)))
    {
        CharPointer_UTF8 (dest).write (c);
        return true;
    }

    return false;
}

MemoryBlock MemoryOutputStream::getMemoryBlock() const
{
    return MemoryBlock (getData(), getDataSize());
}

const void* MemoryOutputStream::getData() const noexcept
{
    if (blockToUse.getSize() > size)
        static_cast<char*> (blockToUse.getData()) [size] = 0;

    return blockToUse.getData();
}

void* MemoryOutputStream::getDataAndRelease() noexcept
{
    if (blockToUse.getSize() > size)
        static_cast<char*> (blockToUse.getData()) [size] = 0;

    return blockToUse.release();
}

bool MemoryOutputStream::setPosition (int64 newPosition)
{
    if (newPosition <= (int64) size)
    {
        // ok to seek backwards
        position = jlimit ((size_t) 0, size, (size_t) newPosition);
        return true;
    }

    // can't move beyond the end of the stream..
    return false;
}

int64 MemoryOutputStream::writeFromInputStream (InputStream& source, int64 maxNumBytesToWrite)
{
    // before writing from an input, see if we can preallocate to make it more efficient..
    int64 availableData = source.getTotalLength() - source.getPosition();

    if (availableData > 0)
    {
        if (maxNumBytesToWrite > availableData || maxNumBytesToWrite < 0)
            maxNumBytesToWrite = availableData;

        preallocate (blockToUse.getSize() + (size_t) maxNumBytesToWrite);
    }

    return OutputStream::writeFromInputStream (source, maxNumBytesToWrite);
}

String MemoryOutputStream::toUTF8() const
{
    const char* const d = static_cast<const char*> (getData());
    return String (CharPointer_UTF8 (d), CharPointer_UTF8 (d + getDataSize()));
}

String MemoryOutputStream::toString() const
{
    return String::createStringFromData (getData(), (int) getDataSize());
}

OutputStream&  operator<< (OutputStream& stream, const MemoryOutputStream& streamToRead)
{
    const size_t dataSize = streamToRead.getDataSize();

    if (dataSize > 0)
        stream.write (streamToRead.getData(), dataSize);

    return stream;
}

}
