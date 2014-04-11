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

#ifndef CARLA_OS_WIN
# include <sys/mman.h>
# ifdef CARLA_OS_MAC
#  include <libkern/OSAtomic.h>
# endif
#endif

// -----------------------------------------------------------------------
// RingBuffer structs

struct HeapBuffer {
    uint32_t size;
    int32_t  head, tail, written;
    bool     invalidateCommit;
    char*    buf;

    HeapBuffer& operator=(const HeapBuffer& rb) noexcept
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

struct StackBuffer {
    static const uint32_t size = 4096;
    int32_t head, tail, written;
    bool    invalidateCommit;
    char    buf[size];
};

// -----------------------------------------------------------------------
// RingBufferControl templated class

template <class BufferStruct>
class RingBufferControl
{
public:
    RingBufferControl(BufferStruct* const ringBuf) noexcept
        : fBuffer(ringBuf)
    {
        if (ringBuf != nullptr)
            clear();
    }

    void clear() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);

        fBuffer->head = 0;
        fBuffer->tail = 0;
        fBuffer->written = 0;
        fBuffer->invalidateCommit = false;

        if (fBuffer->size > 0)
            carla_zeroChar(fBuffer->buf, fBuffer->size);
    }

    void setRingBuffer(BufferStruct* const ringBuf, const bool reset) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(ringBuf != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(ringBuf != fBuffer,);

        fBuffer = ringBuf;

        if (reset)
            clear();
    }

    // -------------------------------------------------------------------

    bool commitWrite() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);

        if (fBuffer->invalidateCommit)
        {
            memoryBarrier();
            fBuffer->written = fBuffer->head;
            fBuffer->invalidateCommit = false;
            return false;
        }
        else
        {
            fBuffer->head = fBuffer->written;
            return true;
        }
    }

    bool isDataAvailable() const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);

        return (fBuffer->head != fBuffer->tail);
    }

    void lockMemory() const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);

#ifdef CARLA_OS_WIN
        ::VirtualLock(fBuffer, sizeof(BufferStruct));
        ::VirtualLock(fBuffer->buf, fBuffer->size);
#else
        ::mlock(fBuffer, sizeof(BufferStruct));
        ::mlock(fBuffer->buf, fBuffer->size);
#endif
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
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(buf != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(size != 0,);
        CARLA_SAFE_ASSERT_RETURN(size < fBuffer->size,);

        // this should not happen
        CARLA_ASSERT(fBuffer->head >= 0);
        CARLA_ASSERT(fBuffer->tail >= 0);
        CARLA_ASSERT(fBuffer->written >= 0);

        // empty
        if (fBuffer->head == fBuffer->tail)
            return;

        char* const charbuf(static_cast<char*>(buf));

        const size_t head(static_cast<size_t>(fBuffer->head));
        const size_t tail(static_cast<size_t>(fBuffer->tail));
        const size_t wrap((head < tail) ? fBuffer->size : 0);

        if (head - tail + wrap < size)
        {
            carla_stderr2("RingBufferControl::tryRead() - failed");
            return;
        }

        size_t readto = tail + size;

        if (readto >= fBuffer->size)
        {
            readto -= fBuffer->size;
            const size_t firstpart(fBuffer->size - tail);
            std::memcpy(charbuf, fBuffer->buf + tail, firstpart);
            std::memcpy(charbuf + firstpart, fBuffer->buf, readto);
        }
        else
        {
            std::memcpy(charbuf, fBuffer->buf + tail, size);
        }

        memoryBarrier();
        fBuffer->tail = static_cast<int32_t>(readto);
    }

    void tryWrite(const void* const buf, const size_t size) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(buf != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(size != 0,);
        CARLA_SAFE_ASSERT_RETURN(size < fBuffer->size,);

        // this should not happen
        CARLA_ASSERT(fBuffer->head >= 0);
        CARLA_ASSERT(fBuffer->tail >= 0);
        CARLA_ASSERT(fBuffer->written >= 0);

        const char* const charbuf(static_cast<const char*>(buf));

        const size_t tail(static_cast<size_t>(fBuffer->tail));
        const size_t wrtn(static_cast<size_t>(fBuffer->written));
        const size_t wrap((tail <= wrtn) ? fBuffer->size : 0);

        if (tail - wrtn + wrap <= size)
        {
            carla_stderr2("RingBufferControl::tryWrite() - buffer full!");
            fBuffer->invalidateCommit = true;
            return;
        }

        size_t writeto = wrtn + size;

        if (writeto >= fBuffer->size)
        {
            writeto -= fBuffer->size;
            const size_t firstpart(fBuffer->size - wrtn);
            std::memcpy(fBuffer->buf + wrtn, charbuf, firstpart);
            std::memcpy(fBuffer->buf, charbuf + firstpart, writeto);
        }
        else
        {
            std::memcpy(fBuffer->buf + wrtn, charbuf, size);
        }

        memoryBarrier();
        fBuffer->written = static_cast<int32_t>(writeto);
    }

private:
    BufferStruct* fBuffer;

    static void memoryBarrier()
    {
#if defined(CARLA_OS_MAC)
        ::OSMemoryBarrier();
#elif defined(CARLA_OS_WIN)
        ::MemoryBarrier();
#elif (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)
        ::__sync_synchronize();
#endif
    }

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(RingBufferControl)
};

// -----------------------------------------------------------------------

#endif // CARLA_RING_BUFFER_HPP_INCLUDED
