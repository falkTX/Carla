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

#include "MidiMessage.h"
#include "../maths/MathsFunctions.h"
#include "../memory/HeapBlock.h"

namespace water {

namespace MidiHelpers
{
    inline uint8 initialByte (const int type, const int channel) noexcept
    {
        return (uint8) (type | jlimit (0, 15, channel - 1));
    }

    inline uint8 validVelocity (const int v) noexcept
    {
        return (uint8) jlimit (0, 127, v);
    }
}

//==============================================================================
uint8 MidiMessage::floatValueToMidiByte (const float v) noexcept
{
    return MidiHelpers::validVelocity (roundToInt (v * 127.0f));
}

uint16 MidiMessage::pitchbendToPitchwheelPos (const float pitchbend,
                                 const float pitchbendRange) noexcept
{
    // can't translate a pitchbend value that is outside of the given range!
    wassert (std::abs (pitchbend) <= pitchbendRange);

    return static_cast<uint16> (pitchbend > 0.0f
                                  ? jmap (pitchbend, 0.0f, pitchbendRange, 8192.0f, 16383.0f)
                                  : jmap (pitchbend, -pitchbendRange, 0.0f, 0.0f, 8192.0f));
}

//==============================================================================
int MidiMessage::readVariableLengthVal (const uint8* data, int& numBytesUsed) noexcept
{
    numBytesUsed = 0;
    int v = 0, i;

    do
    {
        i = (int) *data++;

        if (++numBytesUsed > 6)
            break;

        v = (v << 7) + (i & 0x7f);

    } while (i & 0x80);

    return v;
}

int MidiMessage::getMessageLengthFromFirstByte (const uint8 firstByte) noexcept
{
    // this method only works for valid starting bytes of a short midi message
    wassert (firstByte >= 0x80 && firstByte != 0xf0 && firstByte != 0xf7);

    static const char messageLengths[] =
    {
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        1, 2, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    };

    return messageLengths [firstByte & 0x7f];
}

//==============================================================================
MidiMessage::MidiMessage() noexcept
   : timeStamp (0), size (2)
{
    packedData.asBytes[0] = 0xf0;
    packedData.asBytes[1] = 0xf7;
}

MidiMessage::MidiMessage (const void* const d, const int dataSize, const double t)
   : timeStamp (t), size (dataSize)
{
    wassert (dataSize > 0);
    // this checks that the length matches the data..
    wassert (dataSize > 3 || *(uint8*)d >= 0xf0 || getMessageLengthFromFirstByte (*(uint8*)d) == size);

    memcpy (allocateSpace (dataSize), d, (size_t) dataSize);
}

MidiMessage::MidiMessage (const int byte1, const double t) noexcept
   : timeStamp (t), size (1)
{
    packedData.asBytes[0] = (uint8) byte1;

    // check that the length matches the data..
    wassert (byte1 >= 0xf0 || getMessageLengthFromFirstByte ((uint8) byte1) == 1);
}

MidiMessage::MidiMessage (const int byte1, const int byte2, const double t) noexcept
   : timeStamp (t), size (2)
{
    packedData.asBytes[0] = (uint8) byte1;
    packedData.asBytes[1] = (uint8) byte2;

    // check that the length matches the data..
    wassert (byte1 >= 0xf0 || getMessageLengthFromFirstByte ((uint8) byte1) == 2);
}

MidiMessage::MidiMessage (const int byte1, const int byte2, const int byte3, const double t) noexcept
   : timeStamp (t), size (3)
{
    packedData.asBytes[0] = (uint8) byte1;
    packedData.asBytes[1] = (uint8) byte2;
    packedData.asBytes[2] = (uint8) byte3;

    // check that the length matches the data..
    wassert (byte1 >= 0xf0 || getMessageLengthFromFirstByte ((uint8) byte1) == 3);
}

MidiMessage::MidiMessage (const MidiMessage& other)
   : timeStamp (other.timeStamp), size (other.size)
{
    if (isHeapAllocated())
        memcpy (allocateSpace (size), other.getData(), (size_t) size);
    else
        packedData.allocatedData = other.packedData.allocatedData;
}

MidiMessage::MidiMessage (const MidiMessage& other, const double newTimeStamp)
   : timeStamp (newTimeStamp), size (other.size)
{
    if (isHeapAllocated())
        memcpy (allocateSpace (size), other.getData(), (size_t) size);
    else
        packedData.allocatedData = other.packedData.allocatedData;
}

MidiMessage::MidiMessage (const void* srcData, int sz, int& numBytesUsed, const uint8 lastStatusByte,
                          double t, bool sysexHasEmbeddedLength)
    : timeStamp (t)
{
    const uint8* src = static_cast<const uint8*> (srcData);
    unsigned int byte = (unsigned int) *src;

    if (byte < 0x80)
    {
        byte = (unsigned int) (uint8) lastStatusByte;
        numBytesUsed = -1;
    }
    else
    {
        numBytesUsed = 0;
        --sz;
        ++src;
    }

    if (byte >= 0x80)
    {
        if (byte == 0xf0)
        {
            const uint8* d = src;
            bool haveReadAllLengthBytes = ! sysexHasEmbeddedLength;
            int numVariableLengthSysexBytes = 0;

            while (d < src + sz)
            {
                if (*d >= 0x80)
                {
                    if (*d == 0xf7)
                    {
                        ++d;  // include the trailing 0xf7 when we hit it
                        break;
                    }

                    if (haveReadAllLengthBytes) // if we see a 0x80 bit set after the initial data length
                        break;                  // bytes, assume it's the end of the sysex

                    ++numVariableLengthSysexBytes;
                }
                else if (! haveReadAllLengthBytes)
                {
                    haveReadAllLengthBytes = true;
                    ++numVariableLengthSysexBytes;
                }

                ++d;
            }

            src += numVariableLengthSysexBytes;
            size = 1 + (int) (d - src);

            uint8* dest = allocateSpace (size);
            *dest = (uint8) byte;
            memcpy (dest + 1, src, (size_t) (size - 1));

            numBytesUsed += numVariableLengthSysexBytes;  // (these aren't counted in the size)
        }
        else if (byte == 0xff)
        {
            int n;
            const int bytesLeft = readVariableLengthVal (src + 1, n);
            size = jmin (sz + 1, n + 2 + bytesLeft);

            uint8* dest = allocateSpace (size);
            *dest = (uint8) byte;
            memcpy (dest + 1, src, (size_t) size - 1);
        }
        else
        {
            size = getMessageLengthFromFirstByte ((uint8) byte);
            packedData.asBytes[0] = (uint8) byte;

            if (size > 1)
            {
                packedData.asBytes[1] = src[0];

                if (size > 2)
                    packedData.asBytes[2] = src[1];
            }
        }

        numBytesUsed += size;
    }
    else
    {
        packedData.allocatedData = nullptr;
        size = 0;
    }
}

MidiMessage& MidiMessage::operator= (const MidiMessage& other)
{
    if (this != &other)
    {
        if (other.isHeapAllocated())
        {
            if (isHeapAllocated())
                packedData.allocatedData = static_cast<uint8*> (std::realloc (packedData.allocatedData, (size_t) other.size));
            else
                packedData.allocatedData = static_cast<uint8*> (std::malloc ((size_t) other.size));

            memcpy (packedData.allocatedData, other.packedData.allocatedData, (size_t) other.size);
        }
        else
        {
            if (isHeapAllocated())
                std::free (packedData.allocatedData);

            packedData.allocatedData = other.packedData.allocatedData;
        }

        timeStamp = other.timeStamp;
        size = other.size;
    }

    return *this;
}

#if WATER_COMPILER_SUPPORTS_MOVE_SEMANTICS
MidiMessage::MidiMessage (MidiMessage&& other) noexcept
   : timeStamp (other.timeStamp), size (other.size)
{
    packedData.allocatedData = other.packedData.allocatedData;
    other.size = 0;
}

MidiMessage& MidiMessage::operator= (MidiMessage&& other) noexcept
{
    packedData.allocatedData = other.packedData.allocatedData;
    timeStamp = other.timeStamp;
    size = other.size;
    other.size = 0;
    return *this;
}
#endif

MidiMessage::~MidiMessage() noexcept
{
    if (isHeapAllocated())
        std::free (packedData.allocatedData);
}

uint8* MidiMessage::allocateSpace (int bytes)
{
    if (bytes > (int) sizeof (packedData))
    {
        uint8* d = static_cast<uint8*> (std::malloc ((size_t) bytes));
        packedData.allocatedData = d;
        return d;
    }

    return packedData.asBytes;
}

int MidiMessage::getChannel() const noexcept
{
    const uint8* const data = getData();

    if ((data[0] & 0xf0) != 0xf0)
        return (data[0] & 0xf) + 1;

    return 0;
}

bool MidiMessage::isForChannel (const int channel) const noexcept
{
    wassert (channel > 0 && channel <= 16); // valid channels are numbered 1 to 16

    const uint8* const data = getData();

    return ((data[0] & 0xf) == channel - 1)
             && ((data[0] & 0xf0) != 0xf0);
}

void MidiMessage::setChannel (const int channel) noexcept
{
    wassert (channel > 0 && channel <= 16); // valid channels are numbered 1 to 16

    uint8* const data = getData();

    if ((data[0] & 0xf0) != (uint8) 0xf0)
        data[0] = (uint8) ((data[0] & (uint8) 0xf0)
                            | (uint8)(channel - 1));
}

bool MidiMessage::isNoteOn (const bool returnTrueForVelocity0) const noexcept
{
    const uint8* const data = getData();

    return ((data[0] & 0xf0) == 0x90)
             && (returnTrueForVelocity0 || data[2] != 0);
}

bool MidiMessage::isNoteOff (const bool returnTrueForNoteOnVelocity0) const noexcept
{
    const uint8* const data = getData();

    return ((data[0] & 0xf0) == 0x80)
            || (returnTrueForNoteOnVelocity0 && (data[2] == 0) && ((data[0] & 0xf0) == 0x90));
}

bool MidiMessage::isNoteOnOrOff() const noexcept
{
    const uint8* const data = getData();

    const int d = data[0] & 0xf0;
    return (d == 0x90) || (d == 0x80);
}

int MidiMessage::getNoteNumber() const noexcept
{
    return getData()[1];
}

void MidiMessage::setNoteNumber (const int newNoteNumber) noexcept
{
    if (isNoteOnOrOff() || isAftertouch())
        getData()[1] = (uint8) (newNoteNumber & 127);
}

uint8 MidiMessage::getVelocity() const noexcept
{
    if (isNoteOnOrOff())
        return getData()[2];

    return 0;
}

float MidiMessage::getFloatVelocity() const noexcept
{
    return getVelocity() * (1.0f / 127.0f);
}

void MidiMessage::setVelocity (const float newVelocity) noexcept
{
    if (isNoteOnOrOff())
        getData()[2] = floatValueToMidiByte (newVelocity);
}

void MidiMessage::multiplyVelocity (const float scaleFactor) noexcept
{
    if (isNoteOnOrOff())
    {
        uint8* const data = getData();
        data[2] = MidiHelpers::validVelocity (roundToInt (scaleFactor * data[2]));
    }
}

bool MidiMessage::isAftertouch() const noexcept
{
    return (getData()[0] & 0xf0) == 0xa0;
}

int MidiMessage::getAfterTouchValue() const noexcept
{
    wassert (isAftertouch());
    return getData()[2];
}

MidiMessage MidiMessage::aftertouchChange (const int channel,
                                           const int noteNum,
                                           const int aftertouchValue) noexcept
{
    wassert (channel > 0 && channel <= 16); // valid channels are numbered 1 to 16
    wassert (isPositiveAndBelow (noteNum, (int) 128));
    wassert (isPositiveAndBelow (aftertouchValue, (int) 128));

    return MidiMessage (MidiHelpers::initialByte (0xa0, channel),
                        noteNum & 0x7f,
                        aftertouchValue & 0x7f);
}

bool MidiMessage::isChannelPressure() const noexcept
{
    return (getData()[0] & 0xf0) == 0xd0;
}

int MidiMessage::getChannelPressureValue() const noexcept
{
    wassert (isChannelPressure());
    return getData()[1];
}

MidiMessage MidiMessage::channelPressureChange (const int channel, const int pressure) noexcept
{
    wassert (channel > 0 && channel <= 16); // valid channels are numbered 1 to 16
    wassert (isPositiveAndBelow (pressure, (int) 128));

    return MidiMessage (MidiHelpers::initialByte (0xd0, channel), pressure & 0x7f);
}

bool MidiMessage::isSustainPedalOn() const noexcept     { return isControllerOfType (0x40) && getData()[2] >= 64; }
bool MidiMessage::isSustainPedalOff() const noexcept    { return isControllerOfType (0x40) && getData()[2] <  64; }

bool MidiMessage::isSostenutoPedalOn() const noexcept   { return isControllerOfType (0x42) && getData()[2] >= 64; }
bool MidiMessage::isSostenutoPedalOff() const noexcept  { return isControllerOfType (0x42) && getData()[2] <  64; }

bool MidiMessage::isSoftPedalOn() const noexcept        { return isControllerOfType (0x43) && getData()[2] >= 64; }
bool MidiMessage::isSoftPedalOff() const noexcept       { return isControllerOfType (0x43) && getData()[2] <  64; }


bool MidiMessage::isProgramChange() const noexcept
{
    return (getData()[0] & 0xf0) == 0xc0;
}

int MidiMessage::getProgramChangeNumber() const noexcept
{
    wassert (isProgramChange());
    return getData()[1];
}

MidiMessage MidiMessage::programChange (const int channel, const int programNumber) noexcept
{
    wassert (channel > 0 && channel <= 16); // valid channels are numbered 1 to 16

    return MidiMessage (MidiHelpers::initialByte (0xc0, channel), programNumber & 0x7f);
}

bool MidiMessage::isPitchWheel() const noexcept
{
    return (getData()[0] & 0xf0) == 0xe0;
}

int MidiMessage::getPitchWheelValue() const noexcept
{
    wassert (isPitchWheel());
    const uint8* const data = getData();
    return data[1] | (data[2] << 7);
}

MidiMessage MidiMessage::pitchWheel (const int channel, const int position) noexcept
{
    wassert (channel > 0 && channel <= 16); // valid channels are numbered 1 to 16
    wassert (isPositiveAndBelow (position, (int) 0x4000));

    return MidiMessage (MidiHelpers::initialByte (0xe0, channel),
                        position & 127, (position >> 7) & 127);
}

bool MidiMessage::isController() const noexcept
{
    return (getData()[0] & 0xf0) == 0xb0;
}

bool MidiMessage::isControllerOfType (const int controllerType) const noexcept
{
    const uint8* const data = getData();
    return (data[0] & 0xf0) == 0xb0 && data[1] == controllerType;
}

int MidiMessage::getControllerNumber() const noexcept
{
    wassert (isController());
    return getData()[1];
}

int MidiMessage::getControllerValue() const noexcept
{
    wassert (isController());
    return getData()[2];
}

MidiMessage MidiMessage::controllerEvent (const int channel, const int controllerType, const int value) noexcept
{
    // the channel must be between 1 and 16 inclusive
    wassert (channel > 0 && channel <= 16);

    return MidiMessage (MidiHelpers::initialByte (0xb0, channel),
                        controllerType & 127, value & 127);
}

MidiMessage MidiMessage::noteOn (const int channel, const int noteNumber, const uint8 velocity) noexcept
{
    wassert (channel > 0 && channel <= 16);
    wassert (isPositiveAndBelow (noteNumber, (int) 128));

    return MidiMessage (MidiHelpers::initialByte (0x90, channel),
                        noteNumber & 127, MidiHelpers::validVelocity (velocity));
}

MidiMessage MidiMessage::noteOn (const int channel, const int noteNumber, const float velocity) noexcept
{
    return noteOn (channel, noteNumber, floatValueToMidiByte (velocity));
}

MidiMessage MidiMessage::noteOff (const int channel, const int noteNumber, uint8 velocity) noexcept
{
    wassert (channel > 0 && channel <= 16);
    wassert (isPositiveAndBelow (noteNumber, (int) 128));

    return MidiMessage (MidiHelpers::initialByte (0x80, channel),
                        noteNumber & 127, MidiHelpers::validVelocity (velocity));
}

MidiMessage MidiMessage::noteOff (const int channel, const int noteNumber, float velocity) noexcept
{
    return noteOff (channel, noteNumber, floatValueToMidiByte (velocity));
}

MidiMessage MidiMessage::noteOff (const int channel, const int noteNumber) noexcept
{
    wassert (channel > 0 && channel <= 16);
    wassert (isPositiveAndBelow (noteNumber, (int) 128));

    return MidiMessage (MidiHelpers::initialByte (0x80, channel), noteNumber & 127, 0);
}

MidiMessage MidiMessage::allNotesOff (const int channel) noexcept
{
    return controllerEvent (channel, 123, 0);
}

bool MidiMessage::isAllNotesOff() const noexcept
{
    const uint8* const data = getData();
    return (data[0] & 0xf0) == 0xb0 && data[1] == 123;
}

MidiMessage MidiMessage::allSoundOff (const int channel) noexcept
{
    return controllerEvent (channel, 120, 0);
}

bool MidiMessage::isAllSoundOff() const noexcept
{
    const uint8* const data = getData();
    return (data[0] & 0xf0) == 0xb0 && data[1] == 120;
}

MidiMessage MidiMessage::allControllersOff (const int channel) noexcept
{
    return controllerEvent (channel, 121, 0);
}

MidiMessage MidiMessage::masterVolume (const float volume)
{
    const int vol = jlimit (0, 0x3fff, roundToInt (volume * 0x4000));

    const uint8 buf[] = { 0xf0, 0x7f, 0x7f, 0x04, 0x01,
                          (uint8) (vol & 0x7f),
                          (uint8) (vol >> 7),
                          0xf7 };

    return MidiMessage (buf, 8);
}

//==============================================================================
bool MidiMessage::isSysEx() const noexcept
{
    return *getData() == 0xf0;
}

MidiMessage MidiMessage::createSysExMessage (const void* sysexData, const int dataSize)
{
    HeapBlock<uint8> m;
    CARLA_SAFE_ASSERT_RETURN(m.malloc((size_t) dataSize + 2U), MidiMessage());

    m[0] = 0xf0;
    memcpy (m + 1, sysexData, (size_t) dataSize);
    m[dataSize + 1] = 0xf7;

    return MidiMessage (m, dataSize + 2);
}

const uint8* MidiMessage::getSysExData() const noexcept
{
    return isSysEx() ? getData() + 1 : nullptr;
}

int MidiMessage::getSysExDataSize() const noexcept
{
    return isSysEx() ? size - 2 : 0;
}

//==============================================================================
bool MidiMessage::isMetaEvent() const noexcept      { return *getData() == 0xff; }
bool MidiMessage::isActiveSense() const noexcept    { return *getData() == 0xfe; }

int MidiMessage::getMetaEventType() const noexcept
{
    const uint8* const data = getData();
    return *data != 0xff ? -1 : data[1];
}

int MidiMessage::getMetaEventLength() const noexcept
{
    const uint8* const data = getData();
    if (*data == 0xff)
    {
        int n;
        return jmin (size - 2, readVariableLengthVal (data + 2, n));
    }

    return 0;
}

const uint8* MidiMessage::getMetaEventData() const noexcept
{
    wassert (isMetaEvent());

    int n;
    const uint8* d = getData() + 2;
    readVariableLengthVal (d, n);
    return d + n;
}

bool MidiMessage::isTrackMetaEvent() const noexcept         { return getMetaEventType() == 0; }
bool MidiMessage::isEndOfTrackMetaEvent() const noexcept    { return getMetaEventType() == 47; }

bool MidiMessage::isTextMetaEvent() const noexcept
{
    const int t = getMetaEventType();
    return t > 0 && t < 16;
}

String MidiMessage::getTextFromTextMetaEvent() const
{
    const char* const textData = reinterpret_cast<const char*> (getMetaEventData());
    return String (CharPointer_UTF8 (textData),
                   CharPointer_UTF8 (textData + getMetaEventLength()));
}

MidiMessage MidiMessage::textMetaEvent (int type, StringRef text)
{
    wassert (type > 0 && type < 16);

    MidiMessage result;

    const size_t textSize = text.text.sizeInBytes() - 1;

    uint8 header[8];
    size_t n = sizeof (header);

    header[--n] = (uint8) (textSize & 0x7f);

    for (size_t i = textSize; (i >>= 7) != 0;)
        header[--n] = (uint8) ((i & 0x7f) | 0x80);

    header[--n] = (uint8) type;
    header[--n] = 0xff;

    const size_t headerLen = sizeof (header) - n;
    const int totalSize = (int) (headerLen + textSize);

    uint8* const dest = result.allocateSpace (totalSize);
    result.size = totalSize;

    memcpy (dest, header + n, headerLen);
    memcpy (dest + headerLen, text.text.getAddress(), textSize);

    return result;
}

bool MidiMessage::isTrackNameEvent() const noexcept         { const uint8* data = getData(); return (data[1] == 3)    && (*data == 0xff); }
bool MidiMessage::isTempoMetaEvent() const noexcept         { const uint8* data = getData(); return (data[1] == 81)   && (*data == 0xff); }
bool MidiMessage::isMidiChannelMetaEvent() const noexcept   { const uint8* data = getData(); return (data[1] == 0x20) && (*data == 0xff) && (data[2] == 1); }

int MidiMessage::getMidiChannelMetaEventChannel() const noexcept
{
    wassert (isMidiChannelMetaEvent());
    return getData()[3] + 1;
}

double MidiMessage::getTempoSecondsPerQuarterNote() const noexcept
{
    if (! isTempoMetaEvent())
        return 0.0;

    const uint8* const d = getMetaEventData();

    return (((unsigned int) d[0] << 16)
             | ((unsigned int) d[1] << 8)
             | d[2])
            / 1000000.0;
}

double MidiMessage::getTempoMetaEventTickLength (const short timeFormat) const noexcept
{
    if (timeFormat > 0)
    {
        if (! isTempoMetaEvent())
            return 0.5 / timeFormat;

        return getTempoSecondsPerQuarterNote() / timeFormat;
    }
    else
    {
        const int frameCode = (-timeFormat) >> 8;
        double framesPerSecond;

        switch (frameCode)
        {
            case 24: framesPerSecond = 24.0;   break;
            case 25: framesPerSecond = 25.0;   break;
            case 29: framesPerSecond = 29.97;  break;
            case 30: framesPerSecond = 30.0;   break;
            default: framesPerSecond = 30.0;   break;
        }

        return (1.0 / framesPerSecond) / (timeFormat & 0xff);
    }
}

MidiMessage MidiMessage::tempoMetaEvent (int microsecondsPerQuarterNote) noexcept
{
    const uint8 d[] = { 0xff, 81, 3,
                        (uint8) (microsecondsPerQuarterNote >> 16),
                        (uint8) (microsecondsPerQuarterNote >> 8),
                        (uint8) microsecondsPerQuarterNote };

    return MidiMessage (d, 6, 0.0);
}

bool MidiMessage::isTimeSignatureMetaEvent() const noexcept
{
    const uint8* const data = getData();
    return (data[1] == 0x58) && (*data == (uint8) 0xff);
}

void MidiMessage::getTimeSignatureInfo (int& numerator, int& denominator) const noexcept
{
    if (isTimeSignatureMetaEvent())
    {
        const uint8* const d = getMetaEventData();
        numerator = d[0];
        denominator = 1 << d[1];
    }
    else
    {
        numerator = 4;
        denominator = 4;
    }
}

MidiMessage MidiMessage::timeSignatureMetaEvent (const int numerator, const int denominator)
{
    int n = 1;
    int powerOfTwo = 0;

    while (n < denominator)
    {
        n <<= 1;
        ++powerOfTwo;
    }

    const uint8 d[] = { 0xff, 0x58, 0x04, (uint8) numerator, (uint8) powerOfTwo, 1, 96 };
    return MidiMessage (d, 7, 0.0);
}

MidiMessage MidiMessage::midiChannelMetaEvent (const int channel) noexcept
{
    const uint8 d[] = { 0xff, 0x20, 0x01, (uint8) jlimit (0, 0xff, channel - 1) };
    return MidiMessage (d, 4, 0.0);
}

bool MidiMessage::isKeySignatureMetaEvent() const noexcept
{
    return getMetaEventType() == 0x59;
}

int MidiMessage::getKeySignatureNumberOfSharpsOrFlats() const noexcept
{
    return (int) (int8) getMetaEventData()[0];
}

bool MidiMessage::isKeySignatureMajorKey() const noexcept
{
    return getMetaEventData()[1] == 0;
}

MidiMessage MidiMessage::keySignatureMetaEvent (int numberOfSharpsOrFlats, bool isMinorKey)
{
    wassert (numberOfSharpsOrFlats >= -7 && numberOfSharpsOrFlats <= 7);

    const uint8 d[] = { 0xff, 0x59, 0x02, (uint8) numberOfSharpsOrFlats, isMinorKey ? (uint8) 1 : (uint8) 0 };
    return MidiMessage (d, 5, 0.0);
}

MidiMessage MidiMessage::endOfTrack() noexcept
{
    return MidiMessage (0xff, 0x2f, 0, 0.0);
}

//==============================================================================
bool MidiMessage::isSongPositionPointer() const noexcept         { return *getData() == 0xf2; }
int MidiMessage::getSongPositionPointerMidiBeat() const noexcept { const uint8* data = getData(); return data[1] | (data[2] << 7); }

MidiMessage MidiMessage::songPositionPointer (const int positionInMidiBeats) noexcept
{
    return MidiMessage (0xf2,
                        positionInMidiBeats & 127,
                        (positionInMidiBeats >> 7) & 127);
}

bool MidiMessage::isMidiStart() const noexcept            { return *getData() == 0xfa; }
MidiMessage MidiMessage::midiStart() noexcept             { return MidiMessage (0xfa); }

bool MidiMessage::isMidiContinue() const noexcept         { return *getData() == 0xfb; }
MidiMessage MidiMessage::midiContinue() noexcept          { return MidiMessage (0xfb); }

bool MidiMessage::isMidiStop() const noexcept             { return *getData() == 0xfc; }
MidiMessage MidiMessage::midiStop() noexcept              { return MidiMessage (0xfc); }

bool MidiMessage::isMidiClock() const noexcept            { return *getData() == 0xf8; }
MidiMessage MidiMessage::midiClock() noexcept             { return MidiMessage (0xf8); }

bool MidiMessage::isQuarterFrame() const noexcept               { return *getData() == 0xf1; }
int MidiMessage::getQuarterFrameSequenceNumber() const noexcept { return ((int) getData()[1]) >> 4; }
int MidiMessage::getQuarterFrameValue() const noexcept          { return ((int) getData()[1]) & 0x0f; }

MidiMessage MidiMessage::quarterFrame (const int sequenceNumber, const int value) noexcept
{
    return MidiMessage (0xf1, (sequenceNumber << 4) | value);
}

bool MidiMessage::isFullFrame() const noexcept
{
    const uint8* const data = getData();

    return data[0] == 0xf0
            && data[1] == 0x7f
            && size >= 10
            && data[3] == 0x01
            && data[4] == 0x01;
}

void MidiMessage::getFullFrameParameters (int& hours, int& minutes, int& seconds, int& frames,
                                          MidiMessage::SmpteTimecodeType& timecodeType) const noexcept
{
    wassert (isFullFrame());

    const uint8* const data = getData();
    timecodeType = (SmpteTimecodeType) (data[5] >> 5);
    hours   = data[5] & 0x1f;
    minutes = data[6];
    seconds = data[7];
    frames  = data[8];
}

MidiMessage MidiMessage::fullFrame (const int hours, const int minutes,
                                    const int seconds, const int frames,
                                    MidiMessage::SmpteTimecodeType timecodeType)
{
    const uint8 d[] = { 0xf0, 0x7f, 0x7f, 0x01, 0x01,
                        (uint8) ((hours & 0x01f) | (timecodeType << 5)),
                        (uint8) minutes,
                        (uint8) seconds,
                        (uint8) frames,
                        0xf7 };

    return MidiMessage (d, 10, 0.0);
}

bool MidiMessage::isMidiMachineControlMessage() const noexcept
{
    const uint8* const data = getData();
    return data[0] == 0xf0
        && data[1] == 0x7f
        && data[3] == 0x06
        && size > 5;
}

MidiMessage::MidiMachineControlCommand MidiMessage::getMidiMachineControlCommand() const noexcept
{
    wassert (isMidiMachineControlMessage());

    return (MidiMachineControlCommand) getData()[4];
}

MidiMessage MidiMessage::midiMachineControlCommand (MidiMessage::MidiMachineControlCommand command)
{
    const uint8 d[] = { 0xf0, 0x7f, 0, 6, (uint8) command, 0xf7 };

    return MidiMessage (d, 6, 0.0);
}

//==============================================================================
bool MidiMessage::isMidiMachineControlGoto (int& hours, int& minutes, int& seconds, int& frames) const noexcept
{
    const uint8* const data = getData();
    if (size >= 12
         && data[0] == 0xf0
         && data[1] == 0x7f
         && data[3] == 0x06
         && data[4] == 0x44
         && data[5] == 0x06
         && data[6] == 0x01)
    {
        hours = data[7] % 24;   // (that some machines send out hours > 24)
        minutes = data[8];
        seconds = data[9];
        frames = data[10];

        return true;
    }

    return false;
}

MidiMessage MidiMessage::midiMachineControlGoto (int hours, int minutes, int seconds, int frames)
{
    const uint8 d[] = { 0xf0, 0x7f, 0, 6, 0x44, 6, 1,
                        (uint8) hours,
                        (uint8) minutes,
                        (uint8) seconds,
                        (uint8) frames,
                        0xf7 };

    return MidiMessage (d, 12, 0.0);
}

//==============================================================================
String MidiMessage::getMidiNoteName (int note, bool useSharps, bool includeOctaveNumber, int octaveNumForMiddleC)
{
    static const char* const sharpNoteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    static const char* const flatNoteNames[]  = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };

    if (isPositiveAndBelow (note, (int) 128))
    {
        String s (useSharps ? sharpNoteNames [note % 12]
                            : flatNoteNames  [note % 12]);

        if (includeOctaveNumber)
            s << (note / 12 + (octaveNumForMiddleC - 5));

        return s;
    }

    return String();
}

double MidiMessage::getMidiNoteInHertz (const int noteNumber, const double frequencyOfA) noexcept
{
    return frequencyOfA * pow (2.0, (noteNumber - 69) / 12.0);
}

bool MidiMessage::isMidiNoteBlack (int noteNumber) noexcept
{
    return ((1 << (noteNumber % 12)) & 0x054a) != 0;
}

}
