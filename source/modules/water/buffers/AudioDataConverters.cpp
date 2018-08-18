/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 - ROLI Ltd.
   Copyright (C) 2018 Filipe Coelho <falktx@falktx.com>

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

#include "AudioDataConverters.h"

namespace water {

void AudioDataConverters::convertFloatToInt16LE (const float* source, void* dest, int numSamples, const int destBytesPerSample)
{
    const double maxVal = (double) 0x7fff;
    char* intData = static_cast<char*> (dest);

    if (dest != (void*) source || destBytesPerSample <= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            *(uint16*) intData = ByteOrder::swapIfBigEndian ((uint16) (short) roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])));
            intData += destBytesPerSample;
        }
    }
    else
    {
        intData += destBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= destBytesPerSample;
            *(uint16*) intData = ByteOrder::swapIfBigEndian ((uint16) (short) roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])));
        }
    }
}

void AudioDataConverters::convertFloatToInt16BE (const float* source, void* dest, int numSamples, const int destBytesPerSample)
{
    const double maxVal = (double) 0x7fff;
    char* intData = static_cast<char*> (dest);

    if (dest != (void*) source || destBytesPerSample <= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            *(uint16*) intData = ByteOrder::swapIfLittleEndian ((uint16) (short) roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])));
            intData += destBytesPerSample;
        }
    }
    else
    {
        intData += destBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= destBytesPerSample;
            *(uint16*) intData = ByteOrder::swapIfLittleEndian ((uint16) (short) roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])));
        }
    }
}

void AudioDataConverters::convertFloatToInt24LE (const float* source, void* dest, int numSamples, const int destBytesPerSample)
{
    const double maxVal = (double) 0x7fffff;
    char* intData = static_cast<char*> (dest);

    if (dest != (void*) source || destBytesPerSample <= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            ByteOrder::littleEndian24BitToChars (roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])), intData);
            intData += destBytesPerSample;
        }
    }
    else
    {
        intData += destBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= destBytesPerSample;
            ByteOrder::littleEndian24BitToChars (roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])), intData);
        }
    }
}

void AudioDataConverters::convertFloatToInt24BE (const float* source, void* dest, int numSamples, const int destBytesPerSample)
{
    const double maxVal = (double) 0x7fffff;
    char* intData = static_cast<char*> (dest);

    if (dest != (void*) source || destBytesPerSample <= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            ByteOrder::bigEndian24BitToChars (roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])), intData);
            intData += destBytesPerSample;
        }
    }
    else
    {
        intData += destBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= destBytesPerSample;
            ByteOrder::bigEndian24BitToChars (roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])), intData);
        }
    }
}

void AudioDataConverters::convertFloatToInt32LE (const float* source, void* dest, int numSamples, const int destBytesPerSample)
{
    const double maxVal = (double) 0x7fffffff;
    char* intData = static_cast<char*> (dest);

    if (dest != (void*) source || destBytesPerSample <= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            *(uint32*)intData = ByteOrder::swapIfBigEndian ((uint32) roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])));
            intData += destBytesPerSample;
        }
    }
    else
    {
        intData += destBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= destBytesPerSample;
            *(uint32*)intData = ByteOrder::swapIfBigEndian ((uint32) roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])));
        }
    }
}

void AudioDataConverters::convertFloatToInt32BE (const float* source, void* dest, int numSamples, const int destBytesPerSample)
{
    const double maxVal = (double) 0x7fffffff;
    char* intData = static_cast<char*> (dest);

    if (dest != (void*) source || destBytesPerSample <= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            *(uint32*)intData = ByteOrder::swapIfLittleEndian ((uint32) roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])));
            intData += destBytesPerSample;
        }
    }
    else
    {
        intData += destBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= destBytesPerSample;
            *(uint32*)intData = ByteOrder::swapIfLittleEndian ((uint32) roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])));
        }
    }
}

void AudioDataConverters::convertFloatToFloat32LE (const float* source, void* dest, int numSamples, const int destBytesPerSample)
{
    jassert (dest != (void*) source || destBytesPerSample <= 4); // This op can't be performed on in-place data!

    char* d = static_cast<char*> (dest);

    for (int i = 0; i < numSamples; ++i)
    {
        *(float*) d = source[i];

       #if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
        *(uint32*) d = ByteOrder::swap (*(uint32*) d);
       #endif

        d += destBytesPerSample;
    }
}

void AudioDataConverters::convertFloatToFloat32BE (const float* source, void* dest, int numSamples, const int destBytesPerSample)
{
    jassert (dest != (void*) source || destBytesPerSample <= 4); // This op can't be performed on in-place data!

    char* d = static_cast<char*> (dest);

    for (int i = 0; i < numSamples; ++i)
    {
        *(float*) d = source[i];

       #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        *(uint32*) d = ByteOrder::swap (*(uint32*) d);
       #endif

        d += destBytesPerSample;
    }
}

//==============================================================================
void AudioDataConverters::convertInt16LEToFloat (const void* const source, float* const dest, int numSamples, const int srcBytesPerSample)
{
    const float scale = 1.0f / 0x7fff;
    const char* intData = static_cast<const char*> (source);

    if (source != (void*) dest || srcBytesPerSample >= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            dest[i] = scale * (short) ByteOrder::swapIfBigEndian (*(uint16*)intData);
            intData += srcBytesPerSample;
        }
    }
    else
    {
        intData += srcBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= srcBytesPerSample;
            dest[i] = scale * (short) ByteOrder::swapIfBigEndian (*(uint16*)intData);
        }
    }
}

void AudioDataConverters::convertInt16BEToFloat (const void* const source, float* const dest, int numSamples, const int srcBytesPerSample)
{
    const float scale = 1.0f / 0x7fff;
    const char* intData = static_cast<const char*> (source);

    if (source != (void*) dest || srcBytesPerSample >= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            dest[i] = scale * (short) ByteOrder::swapIfLittleEndian (*(uint16*)intData);
            intData += srcBytesPerSample;
        }
    }
    else
    {
        intData += srcBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= srcBytesPerSample;
            dest[i] = scale * (short) ByteOrder::swapIfLittleEndian (*(uint16*)intData);
        }
    }
}

void AudioDataConverters::convertInt24LEToFloat (const void* const source, float* const dest, int numSamples, const int srcBytesPerSample)
{
    const float scale = 1.0f / 0x7fffff;
    const char* intData = static_cast<const char*> (source);

    if (source != (void*) dest || srcBytesPerSample >= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            dest[i] = scale * (short) ByteOrder::littleEndian24Bit (intData);
            intData += srcBytesPerSample;
        }
    }
    else
    {
        intData += srcBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= srcBytesPerSample;
            dest[i] = scale * (short) ByteOrder::littleEndian24Bit (intData);
        }
    }
}

void AudioDataConverters::convertInt24BEToFloat (const void* const source, float* const dest, int numSamples, const int srcBytesPerSample)
{
    const float scale = 1.0f / 0x7fffff;
    const char* intData = static_cast<const char*> (source);

    if (source != (void*) dest || srcBytesPerSample >= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            dest[i] = scale * (short) ByteOrder::bigEndian24Bit (intData);
            intData += srcBytesPerSample;
        }
    }
    else
    {
        intData += srcBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= srcBytesPerSample;
            dest[i] = scale * (short) ByteOrder::bigEndian24Bit (intData);
        }
    }
}

void AudioDataConverters::convertInt32LEToFloat (const void* const source, float* const dest, int numSamples, const int srcBytesPerSample)
{
    const float scale = 1.0f / 0x7fffffff;
    const char* intData = static_cast<const char*> (source);

    if (source != (void*) dest || srcBytesPerSample >= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            dest[i] = scale * (int) ByteOrder::swapIfBigEndian (*(uint32*) intData);
            intData += srcBytesPerSample;
        }
    }
    else
    {
        intData += srcBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= srcBytesPerSample;
            dest[i] = scale * (int) ByteOrder::swapIfBigEndian (*(uint32*) intData);
        }
    }
}

void AudioDataConverters::convertInt32BEToFloat (const void* const source, float* const dest, int numSamples, const int srcBytesPerSample)
{
    const float scale = 1.0f / 0x7fffffff;
    const char* intData = static_cast<const char*> (source);

    if (source != (void*) dest || srcBytesPerSample >= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            dest[i] = scale * (int) ByteOrder::swapIfLittleEndian (*(uint32*) intData);
            intData += srcBytesPerSample;
        }
    }
    else
    {
        intData += srcBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= srcBytesPerSample;
            dest[i] = scale * (int) ByteOrder::swapIfLittleEndian (*(uint32*) intData);
        }
    }
}

void AudioDataConverters::convertFloat32LEToFloat (const void* const source, float* const dest, int numSamples, const int srcBytesPerSample)
{
    const char* s = static_cast<const char*> (source);

    for (int i = 0; i < numSamples; ++i)
    {
        dest[i] = *(float*)s;

       #if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
        uint32* const d = (uint32*) (dest + i);
        *d = ByteOrder::swap (*d);
       #endif

        s += srcBytesPerSample;
    }
}

void AudioDataConverters::convertFloat32BEToFloat (const void* const source, float* const dest, int numSamples, const int srcBytesPerSample)
{
    const char* s = static_cast<const char*> (source);

    for (int i = 0; i < numSamples; ++i)
    {
        dest[i] = *(float*)s;

       #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        uint32* const d = (uint32*) (dest + i);
        *d = ByteOrder::swap (*d);
       #endif

        s += srcBytesPerSample;
    }
}


//==============================================================================
void AudioDataConverters::convertFloatToFormat (const DataFormat destFormat,
                                                const float* const source,
                                                void* const dest,
                                                const int numSamples)
{
    switch (destFormat)
    {
        case int16LE:       convertFloatToInt16LE   (source, dest, numSamples); break;
        case int16BE:       convertFloatToInt16BE   (source, dest, numSamples); break;
        case int24LE:       convertFloatToInt24LE   (source, dest, numSamples); break;
        case int24BE:       convertFloatToInt24BE   (source, dest, numSamples); break;
        case int32LE:       convertFloatToInt32LE   (source, dest, numSamples); break;
        case int32BE:       convertFloatToInt32BE   (source, dest, numSamples); break;
        case float32LE:     convertFloatToFloat32LE (source, dest, numSamples); break;
        case float32BE:     convertFloatToFloat32BE (source, dest, numSamples); break;
        default:            jassertfalse; break;
    }
}

void AudioDataConverters::convertFormatToFloat (const DataFormat sourceFormat,
                                                const void* const source,
                                                float* const dest,
                                                const int numSamples)
{
    switch (sourceFormat)
    {
        case int16LE:       convertInt16LEToFloat   (source, dest, numSamples); break;
        case int16BE:       convertInt16BEToFloat   (source, dest, numSamples); break;
        case int24LE:       convertInt24LEToFloat   (source, dest, numSamples); break;
        case int24BE:       convertInt24BEToFloat   (source, dest, numSamples); break;
        case int32LE:       convertInt32LEToFloat   (source, dest, numSamples); break;
        case int32BE:       convertInt32BEToFloat   (source, dest, numSamples); break;
        case float32LE:     convertFloat32LEToFloat (source, dest, numSamples); break;
        case float32BE:     convertFloat32BEToFloat (source, dest, numSamples); break;
        default:            jassertfalse; break;
    }
}

//==============================================================================
void AudioDataConverters::interleaveSamples (const float** const source,
                                             float* const dest,
                                             const int numSamples,
                                             const int numChannels)
{
    for (int chan = 0; chan < numChannels; ++chan)
    {
        int i = chan;
        const float* src = source [chan];

        for (int j = 0; j < numSamples; ++j)
        {
            dest [i] = src [j];
            i += numChannels;
        }
    }
}

void AudioDataConverters::deinterleaveSamples (const float* const source,
                                               float** const dest,
                                               const int numSamples,
                                               const int numChannels)
{
    for (int chan = 0; chan < numChannels; ++chan)
    {
        int i = chan;
        float* dst = dest [chan];

        for (int j = 0; j < numSamples; ++j)
        {
            dst [j] = source [i];
            i += numChannels;
        }
    }
}

}
