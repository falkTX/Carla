/*
 * Carla Ring Buffer
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#ifndef CARLA_RING_BUFFER_HPP_INCLUDED
#define CARLA_RING_BUFFER_HPP_INCLUDED

#include "CarlaUtils.hpp"

// -----------------------------------------------------------------------
// RingBuffer structs

struct HeapRingBuffer {
    uint32_t size;
    int32_t  head, tail, written;
    bool     invalidateCommit;
    char*    buf;

    HeapRingBuffer& operator=(const HeapRingBuffer& rb) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(size == rb.size, *this);

        size = rb.size;
        head = rb.head;
        tail = rb.tail;
        written = rb.written;
        invalidateCommit = rb.invalidateCommit;
        std::memcpy(buf, rb.buf, size);

        return *this;
    }
};

struct StackRingBuffer {
    static const uint32_t size = 4096;
    int32_t head, tail, written;
    bool    invalidateCommit;
    char    buf[size];
};

// -----------------------------------------------------------------------
// RingBufferControl templated class

template <class RingBufferStruct>
class RingBufferControl
{
public:
    RingBufferControl(RingBufferStruct* const ringBuf) noexcept
        : fRingBuf(ringBuf)
    {
        if (ringBuf != nullptr)
            clear();
    }

    void clear() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fRingBuf != nullptr,);

        fRingBuf->head = 0;
        fRingBuf->tail = 0;
        fRingBuf->written = 0;
        fRingBuf->invalidateCommit = false;

        if (fRingBuf->size > 0)
            carla_zeroChar(fRingBuf->buf, fRingBuf->size);
    }

    void setRingBuffer(RingBufferStruct* const ringBuf, const bool reset) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(ringBuf != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(ringBuf != fRingBuf,);

        fRingBuf = ringBuf;

        if (reset)
            clear();
    }

    // -------------------------------------------------------------------

    bool commitWrite() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fRingBuf != nullptr, false);

        if (fRingBuf->invalidateCommit)
        {
            fRingBuf->written = fRingBuf->head;
            fRingBuf->invalidateCommit = false;
            return false;
        }
        else
        {
            fRingBuf->head = fRingBuf->written;
            return true;
        }
    }

    bool isDataAvailable() const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fRingBuf != nullptr, false);

        return (fRingBuf->head != fRingBuf->tail);
    }

    // -------------------------------------------------------------------

    char readChar() noexcept
    {
        char c = '\0';
        tryRead(&c, sizeof(char));
        return c;
    }

    int32_t readInt() noexcept
    {
        int32_t i = 0;
        tryRead(&i, sizeof(int32_t));
        return i;
    }

    uint32_t readUInt() noexcept
    {
        int32_t i = -1;
        tryRead(&i, sizeof(int32_t));
        return (i >= 0) ? static_cast<uint32_t>(i) : 0;
    }

    int64_t readLong() noexcept
    {
        int64_t l = 0;
        tryRead(&l, sizeof(int64_t));
        return l;
    }

    float readFloat() noexcept
    {
        float f = 0.0f;
        tryRead(&f, sizeof(float));
        return f;
    }

    // -------------------------------------------------------------------

    void writeChar(const char value) noexcept
    {
        tryWrite(&value, sizeof(char));
    }

    void writeInt(const int32_t value) noexcept
    {
        tryWrite(&value, sizeof(int32_t));
    }

    void writeLong(const int64_t value) noexcept
    {
        tryWrite(&value, sizeof(int64_t));
    }

    void writeFloat(const float value) noexcept
    {
        tryWrite(&value, sizeof(float));
    }

    // -------------------------------------------------------------------

protected:
    void tryRead(void* const buf, const size_t size) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fRingBuf != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(buf != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(size != 0,);
        CARLA_SAFE_ASSERT_RETURN(size < fRingBuf->size,);

        // this should not happen
        CARLA_ASSERT(fRingBuf->head >= 0);
        CARLA_ASSERT(fRingBuf->tail >= 0);
        CARLA_ASSERT(fRingBuf->written >= 0);

        // empty
        if (fRingBuf->head == fRingBuf->tail)
            return;

        char* const charbuf(static_cast<char*>(buf));

        const size_t head(static_cast<size_t>(fRingBuf->head));
        const size_t tail(static_cast<size_t>(fRingBuf->tail));
        const size_t wrap((head < tail) ? fRingBuf->size : 0);

        if (head - tail + wrap < size)
        {
            carla_stderr2("RingBufferControl::tryRead() - failed");
            return;
        }

        size_t readto = tail + size;

        if (readto >= fRingBuf->size)
        {
            readto -= fRingBuf->size;
            const size_t firstpart(fRingBuf->size - tail);
            std::memcpy(charbuf, fRingBuf->buf + tail, firstpart);
            std::memcpy(charbuf + firstpart, fRingBuf->buf, readto);
        }
        else
        {
            std::memcpy(charbuf, fRingBuf->buf + tail, size);
        }

        fRingBuf->tail = static_cast<int32_t>(readto);
    }

    void tryWrite(const void* const buf, const size_t size) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fRingBuf != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(buf != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(size != 0,);
        CARLA_SAFE_ASSERT_RETURN(size < fRingBuf->size,);

        // this should not happen
        CARLA_ASSERT(fRingBuf->head >= 0);
        CARLA_ASSERT(fRingBuf->tail >= 0);
        CARLA_ASSERT(fRingBuf->written >= 0);

        const char* const charbuf(static_cast<const char*>(buf));

        const size_t tail(static_cast<size_t>(fRingBuf->tail));
        const size_t wrtn(static_cast<size_t>(fRingBuf->written));
        const size_t wrap((tail <= wrtn) ? fRingBuf->size : 0);

        if (tail - wrtn + wrap <= size)
        {
            carla_stderr2("RingBufferControl::tryWrite() - buffer full!");
            fRingBuf->invalidateCommit = true;
            return;
        }

        size_t writeto = wrtn + size;

        if (writeto >= fRingBuf->size)
        {
            writeto -= fRingBuf->size;
            const size_t firstpart(fRingBuf->size - wrtn);
            std::memcpy(fRingBuf->buf + wrtn, charbuf, firstpart);
            std::memcpy(fRingBuf->buf, charbuf + firstpart, writeto);
        }
        else
        {
            std::memcpy(fRingBuf->buf + wrtn, charbuf, size);
        }

        fRingBuf->written = static_cast<int32_t>(writeto);
    }

private:
    RingBufferStruct* fRingBuf;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(RingBufferControl)
};

// -----------------------------------------------------------------------

#endif // CARLA_RING_BUFFER_HPP_INCLUDED
