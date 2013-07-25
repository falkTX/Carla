/*
 * Carla Ring Buffer imported from dssi-vst code
 * Copyright (C) 2004-2010 Chris Cannam <cannam@all-day-breakfast.com>
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef CARLA_RING_BUFFER_HPP_INCLUDED
#define CARLA_RING_BUFFER_HPP_INCLUDED

#include "CarlaJuceUtils.hpp"

#define RING_BUFFER_SIZE 2048

// -----------------------------------------------------------------------
// RingBuffer struct

struct RingBuffer {
    int head, tail, written;
    bool invalidateCommit;
    char buf[RING_BUFFER_SIZE];

    CARLA_DECLARE_NON_COPY_STRUCT(RingBuffer)
};

// -----------------------------------------------------------------------
// RingBufferControl class

class RingBufferControl
{
public:
    RingBufferControl(RingBuffer* const ringBuf)
        : fRingBuf(nullptr)
    {
        if (ringBuf != nullptr)
            setRingBuffer(ringBuf, true);
    }

    void setRingBuffer(RingBuffer* const ringBuf, const bool reset)
    {
        CARLA_ASSERT(ringBuf != nullptr);
        CARLA_ASSERT(ringBuf != fRingBuf);

        fRingBuf = ringBuf;

        if (! reset)
            return;

        fRingBuf->head = 0;
        fRingBuf->tail = 0;
        fRingBuf->written = 0;
        fRingBuf->invalidateCommit = false;

        carla_fill<char>(fRingBuf->buf, RING_BUFFER_SIZE, 0);
    }

    // ---------------------------------------------

    void commitWrite()
    {
        CARLA_ASSERT(fRingBuf != nullptr);

        if (fRingBuf == nullptr)
            return;

        if (fRingBuf->invalidateCommit)
        {
            fRingBuf->written = fRingBuf->head;
            fRingBuf->invalidateCommit = false;
        }
        else
        {
            fRingBuf->head = fRingBuf->written;
        }
    }

    bool dataAvailable() const
    {
        CARLA_ASSERT(fRingBuf != nullptr);

        if (fRingBuf == nullptr)
            return false;

        return (fRingBuf->tail != fRingBuf->head);
    }

    // ---------------------------------------------

    template<typename T>
    T read()
    {
        T t = 0;
        tryRead(&t, sizeof(T));
        return t;
    }

    char readChar()
    {
        char c = 0;
        tryRead(&c, sizeof(char));
        return c;
    }

    int readInt()
    {
        int i = 0;
        tryRead(&i, sizeof(int));
        return i;
    }

    long readLong()
    {
        long l = 0;
        tryRead(&l, sizeof(long));
        return l;
    }

    float readFloat()
    {
        float f = 0.0f;
        tryRead(&f, sizeof(float));
        return f;
    }

    // ---------------------------------------------

    template<typename T>
    void write(const T value)
    {
        tryWrite(&value, sizeof(T));
    }

    void writeChar(const char value)
    {
        tryWrite(&value, sizeof(char));
    }

    void writeInt(const int value)
    {
        tryWrite(&value, sizeof(int));
    }

    void writeLong(const long value)
    {
        tryWrite(&value, sizeof(long));
    }

    void writeFloat(const float value)
    {
        tryWrite(&value, sizeof(float));
    }

    // ---------------------------------------------

private:
    RingBuffer* fRingBuf;

    void tryRead(void* const buf, const size_t size)
    {
        CARLA_ASSERT(fRingBuf != nullptr);
        CARLA_ASSERT(buf != nullptr);
        CARLA_ASSERT(size != 0);

        if (fRingBuf == nullptr)
            return;
        if (buf == nullptr)
            return;
        if (size == 0)
            return;

        // this should not happen
        CARLA_ASSERT(fRingBuf->head >= 0);
        CARLA_ASSERT(fRingBuf->tail >= 0);
        CARLA_ASSERT(fRingBuf->written >= 0);

        char* const charbuf(static_cast<char*>(buf));

        const size_t head(fRingBuf->head);
        const size_t tail(fRingBuf->tail);
        const size_t wrap((head <= tail) ? RING_BUFFER_SIZE : 0);

        if (head - tail + wrap < size)
            return;

        size_t readto = tail + size;

        if (readto >= RING_BUFFER_SIZE)
        {
            readto -= RING_BUFFER_SIZE;
            const size_t firstpart(RING_BUFFER_SIZE - tail);
            std::memcpy(charbuf, fRingBuf->buf + tail, firstpart);
            std::memcpy(charbuf + firstpart, fRingBuf->buf, readto);
        }
        else
        {
            std::memcpy(charbuf, fRingBuf->buf + tail, size);
        }

        fRingBuf->tail = readto;
    }

    void tryWrite(const void* const buf, const size_t size)
    {
        CARLA_ASSERT(fRingBuf != nullptr);
        CARLA_ASSERT(buf != nullptr);
        CARLA_ASSERT(size != 0);

        if (fRingBuf == nullptr)
            return;
        if (buf == nullptr)
            return;
        if (size == 0)
            return;

        const char* const charbuf(static_cast<const char*>(buf));

        const size_t tail(fRingBuf->tail);
        const size_t written(fRingBuf->written);
        const size_t wrap((tail <= written) ? RING_BUFFER_SIZE : 0);

        if (tail - written + wrap < size)
        {
            carla_stderr2("RingBufferControl::tryWrite() - buffer full!");
            fRingBuf->invalidateCommit = true;
            return;
        }

        size_t writeto = written + size;

        if (writeto >= RING_BUFFER_SIZE)
        {
            writeto -= RING_BUFFER_SIZE;
            const size_t firstpart(RING_BUFFER_SIZE - written);
            std::memcpy(fRingBuf->buf + written, charbuf, firstpart);
            std::memcpy(fRingBuf->buf, charbuf + firstpart, writeto);
        }
        else
        {
            std::memcpy(fRingBuf->buf + written, charbuf, size);
        }

        fRingBuf->written = writeto;
    }
};

#endif // CARLA_RING_BUFFER_HPP_INCLUDED
