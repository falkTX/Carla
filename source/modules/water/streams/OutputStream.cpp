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

#include "OutputStream.h"
#include "../files/FileInputStream.h"
#include "../memory/ByteOrder.h"
#include "../memory/MemoryBlock.h"
#include "../text/NewLine.h"

namespace water {

//==============================================================================
OutputStream::OutputStream()
    : newLineString (NewLine::getDefault())
{
}

OutputStream::~OutputStream()
{
}

//==============================================================================
bool OutputStream::writeBool (const bool b)
{
    return writeByte (b ? (char) 1
                        : (char) 0);
}

bool OutputStream::writeByte (char byte)
{
    return write (&byte, 1);
}

bool OutputStream::writeRepeatedByte (uint8 byte, size_t numTimesToRepeat)
{
    for (size_t i = 0; i < numTimesToRepeat; ++i)
        if (! writeByte ((char) byte))
            return false;

    return true;
}

bool OutputStream::writeShort (short value)
{
    const unsigned short v = ByteOrder::swapIfBigEndian ((unsigned short) value);
    return write (&v, 2);
}

bool OutputStream::writeShortBigEndian (short value)
{
    const unsigned short v = ByteOrder::swapIfLittleEndian ((unsigned short) value);
    return write (&v, 2);
}

bool OutputStream::writeInt (int value)
{
    const unsigned int v = ByteOrder::swapIfBigEndian ((unsigned int) value);
    return write (&v, 4);
}

bool OutputStream::writeIntBigEndian (int value)
{
    const unsigned int v = ByteOrder::swapIfLittleEndian ((unsigned int) value);
    return write (&v, 4);
}

bool OutputStream::writeCompressedInt (int value)
{
    unsigned int un = (value < 0) ? (unsigned int) -value
                                  : (unsigned int) value;

    uint8 data[5];
    int num = 0;

    while (un > 0)
    {
        data[++num] = (uint8) un;
        un >>= 8;
    }

    data[0] = (uint8) num;

    if (value < 0)
        data[0] |= 0x80;

    return write (data, (size_t) num + 1);
}

bool OutputStream::writeInt64 (int64 value)
{
    const uint64 v = ByteOrder::swapIfBigEndian ((uint64) value);
    return write (&v, 8);
}

bool OutputStream::writeInt64BigEndian (int64 value)
{
    const uint64 v = ByteOrder::swapIfLittleEndian ((uint64) value);
    return write (&v, 8);
}

bool OutputStream::writeFloat (float value)
{
    union { int asInt; float asFloat; } n;
    n.asFloat = value;
    return writeInt (n.asInt);
}

bool OutputStream::writeFloatBigEndian (float value)
{
    union { int asInt; float asFloat; } n;
    n.asFloat = value;
    return writeIntBigEndian (n.asInt);
}

bool OutputStream::writeDouble (double value)
{
    union { int64 asInt; double asDouble; } n;
    n.asDouble = value;
    return writeInt64 (n.asInt);
}

bool OutputStream::writeDoubleBigEndian (double value)
{
    union { int64 asInt; double asDouble; } n;
    n.asDouble = value;
    return writeInt64BigEndian (n.asInt);
}

bool OutputStream::writeString (const String& text)
{
    return write (text.toRawUTF8(), text.getNumBytesAsUTF8() + 1);
}

bool OutputStream::writeText (const String& text, const bool asUTF16,
                              const bool writeUTF16ByteOrderMark)
{
    if (asUTF16)
    {
        if (writeUTF16ByteOrderMark)
            write ("\x0ff\x0fe", 2);

        CharPointer_UTF8 src (text.getCharPointer());
        bool lastCharWasReturn = false;

        for (;;)
        {
            const water_uchar c = src.getAndAdvance();

            if (c == 0)
                break;

            if (c == '\n' && ! lastCharWasReturn)
                writeShort ((short) '\r');

            lastCharWasReturn = (c == L'\r');

            if (! writeShort ((short) c))
                return false;
        }
    }
    else
    {
        const char* src = text.toUTF8();
        const char* t = src;

        for (;;)
        {
            if (*t == '\n')
            {
                if (t > src)
                    if (! write (src, (size_t) (t - src)))
                        return false;

                if (! write ("\r\n", 2))
                    return false;

                src = t + 1;
            }
            else if (*t == '\r')
            {
                if (t[1] == '\n')
                    ++t;
            }
            else if (*t == 0)
            {
                if (t > src)
                    if (! write (src, (size_t) (t - src)))
                        return false;

                break;
            }

            ++t;
        }
    }

    return true;
}

int64 OutputStream::writeFromInputStream (InputStream& source, int64 numBytesToWrite)
{
    if (numBytesToWrite < 0)
        numBytesToWrite = std::numeric_limits<int64>::max();

    int64 numWritten = 0;

    while (numBytesToWrite > 0)
    {
        char buffer [8192];
        const int num = source.read (buffer, (int) jmin (numBytesToWrite, (int64) sizeof (buffer)));

        if (num <= 0)
            break;

        write (buffer, (size_t) num);

        numBytesToWrite -= num;
        numWritten += num;
    }

    return numWritten;
}

//==============================================================================
void OutputStream::setNewLineString (const String& newLineString_)
{
    newLineString = newLineString_;
}

//==============================================================================
template <typename IntegerType>
static void writeIntToStream (OutputStream& stream, IntegerType number)
{
    char buffer [NumberToStringConverters::charsNeededForInt];
    char* end = buffer + numElementsInArray (buffer);
    const char* start = NumberToStringConverters::numberToString (end, number);
    stream.write (start, (size_t) (end - start - 1));
}

OutputStream& operator<< (OutputStream& stream, const int number)
{
    writeIntToStream (stream, number);
    return stream;
}

OutputStream& operator<< (OutputStream& stream, const int64 number)
{
    writeIntToStream (stream, number);
    return stream;
}

OutputStream& operator<< (OutputStream& stream, const double number)
{
    return stream << String (number);
}

OutputStream& operator<< (OutputStream& stream, const char character)
{
    stream.writeByte (character);
    return stream;
}

OutputStream& operator<< (OutputStream& stream, const char* const text)
{
    stream.write (text, strlen (text));
    return stream;
}

OutputStream& operator<< (OutputStream& stream, const MemoryBlock& data)
{
    if (data.getSize() > 0)
        stream.write (data.getData(), data.getSize());

    return stream;
}

OutputStream& operator<< (OutputStream& stream, const File& fileToRead)
{
    FileInputStream in (fileToRead);

    if (in.openedOk())
        return stream << in;

    return stream;
}

OutputStream& operator<< (OutputStream& stream, InputStream& streamToRead)
{
    stream.writeFromInputStream (streamToRead, -1);
    return stream;
}

OutputStream& operator<< (OutputStream& stream, const NewLine&)
{
    return stream << stream.getNewLineString();
}

}
