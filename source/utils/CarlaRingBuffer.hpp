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

#include "CarlaMathUtils.hpp"

// -----------------------------------------------------------------------
// Buffer structs

/*
   head:
    current writing position, headmost position of the buffer.
    increments when writing.

   tail:
    current reading position, last used position of the buffer.
    increments when reading.
    head == tail means empty buffer

   wrtn:
    temporary position of head until a commitWrite() is called.
    if buffer writing fails, wrtn will be back to head position thus ignoring the last operation(s).
    if buffer writing succeeds, head will be set to this variable.

   invalidateCommit:
    boolean used to check if a write operation failed.
    this ensures we don't get incomplete writes.
  */

struct HeapBuffer {
    uint32_t size;
    uint32_t head, tail, wrtn;
    bool     invalidateCommit;
    uint8_t* buf;

    void copyDataFrom(const HeapBuffer& rb) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(size == rb.size,);

        head = rb.head;
        tail = rb.tail;
        wrtn = rb.wrtn;
        invalidateCommit = rb.invalidateCommit;
        std::memcpy(buf, rb.buf, size);
    }
};

struct StackBuffer {
    static const uint32_t size = 4096;
    uint32_t head, tail, wrtn;
    bool     invalidateCommit;
    uint8_t  buf[size];
};

struct BigStackBuffer {
    static const uint32_t size = 16384;
    uint32_t head, tail, wrtn;
    bool     invalidateCommit;
    uint8_t  buf[size];
};

#define HeapBuffer_INIT  {0, 0, 0, 0, false, nullptr}
#define StackBuffer_INIT {0, 0, 0, false, {0}}

// -----------------------------------------------------------------------
// CarlaRingBuffer templated class

template <class BufferStruct>
class CarlaRingBuffer
{
public:
    CarlaRingBuffer() noexcept
        : fBuffer(nullptr) {}

    virtual ~CarlaRingBuffer() noexcept {}

    // -------------------------------------------------------------------

    void clear() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);

        fBuffer->head = 0;
        fBuffer->tail = 0;
        fBuffer->wrtn = 0;
        fBuffer->invalidateCommit = false;

        carla_zeroBytes(fBuffer->buf, fBuffer->size);
    }

    // -------------------------------------------------------------------

    bool commitWrite() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);

        if (fBuffer->invalidateCommit)
        {
            fBuffer->wrtn = fBuffer->head;
            fBuffer->invalidateCommit = false;
            return false;
        }

        // nothing to commit?
        CARLA_SAFE_ASSERT_RETURN(fBuffer->head != fBuffer->wrtn, false);

        // all ok
        fBuffer->head = fBuffer->wrtn;
        return true;
    }

    bool isDataAvailableForReading() const noexcept
    {
        return (fBuffer != nullptr && fBuffer->buf != nullptr && fBuffer->head != fBuffer->tail);
    }

    bool isEmpty() const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);

        return (fBuffer->buf == nullptr || fBuffer->head == fBuffer->tail);
    }

    // -------------------------------------------------------------------

    bool readBool() noexcept
    {
        bool b = false;
        return tryRead(&b, sizeof(bool)) ? b : false;
    }

    uint8_t readByte() noexcept
    {
        uint8_t B = 0;
        return tryRead(&B, sizeof(uint8_t)) ? B : 0;
    }

    int16_t readShort() noexcept
    {
        int16_t s = 0;
        return tryRead(&s, sizeof(int16_t)) ? s : 0;
    }

    uint16_t readUShort() noexcept
    {
        uint16_t us = 0;
        return tryRead(&us, sizeof(uint16_t)) ? us : 0;
    }

    int32_t readInt() noexcept
    {
        int32_t i = 0;
        return tryRead(&i, sizeof(int32_t)) ? i : 0;
    }

    uint32_t readUInt() noexcept
    {
        uint32_t ui = 0;
        return tryRead(&ui, sizeof(int32_t)) ? ui : 0;
    }

    int64_t readLong() noexcept
    {
        int64_t l = 0;
        return tryRead(&l, sizeof(int64_t)) ? l : 0;
    }

    uint64_t readULong() noexcept
    {
        uint64_t ul = 0;
        return tryRead(&ul, sizeof(int64_t)) ? ul : 0;
    }

    float readFloat() noexcept
    {
        float f = 0.0f;
        return tryRead(&f, sizeof(float)) ? f : 0.0f;
    }

    double readDouble() noexcept
    {
        double d = 0.0;
        return tryRead(&d, sizeof(double)) ? d : 0.0;
    }

    void readCustomData(void* const data, const uint32_t size) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(size > 0,);

        if (! tryRead(data, size))
            carla_zeroBytes(data, size);
    }

    template <typename T>
    void readCustomType(T& type) noexcept
    {
        if (! tryRead(&type, sizeof(T)))
            carla_zeroStruct(type);
    }

    // -------------------------------------------------------------------

    void writeBool(const bool value) noexcept
    {
        tryWrite(&value, sizeof(bool));
    }

    void writeByte(const uint8_t value) noexcept
    {
        tryWrite(&value, sizeof(uint8_t));
    }

    void writeShort(const int16_t value) noexcept
    {
        tryWrite(&value, sizeof(int16_t));
    }

    void writeUShort(const uint16_t value) noexcept
    {
        tryWrite(&value, sizeof(uint16_t));
    }

    void writeInt(const int32_t value) noexcept
    {
        tryWrite(&value, sizeof(int32_t));
    }

    void writeUInt(const uint32_t value) noexcept
    {
        tryWrite(&value, sizeof(uint32_t));
    }

    void writeLong(const int64_t value) noexcept
    {
        tryWrite(&value, sizeof(int64_t));
    }

    void writeFloat(const float value) noexcept
    {
        tryWrite(&value, sizeof(float));
    }

    void writeDouble(const double value) noexcept
    {
        tryWrite(&value, sizeof(double));
    }

    void writeCustomData(const void* const value, const uint32_t size) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(size > 0,);

        tryWrite(value, size);
    }

    template <typename T>
    void writeCustomType(const T& value) noexcept
    {
        tryWrite(&value, sizeof(T));
    }

    // -------------------------------------------------------------------

protected:
    void setRingBuffer(BufferStruct* const ringBuf, const bool reset) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(ringBuf != fBuffer,);

        fBuffer = ringBuf;

        if (reset && ringBuf != nullptr)
            clear();
    }

    // -------------------------------------------------------------------

    bool tryRead(void* const buf, const uint32_t size) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(buf != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(size > 0, false);
        CARLA_SAFE_ASSERT_RETURN(size < fBuffer->size, false);

        // empty
        if (fBuffer->head == fBuffer->tail)
            return false;

        uint8_t* const bytebuf(static_cast<uint8_t*>(buf));

        const uint32_t head(fBuffer->head);
        const uint32_t tail(fBuffer->tail);
        const uint32_t wrap((head > tail) ? 0 : fBuffer->size);

        if (size > wrap + head - tail)
        {
            carla_stderr2("CarlaRingBuffer::tryRead(%p, " P_SIZE "): failed, not enough space", buf, size);
            return false;
        }

        uint32_t readto(tail + size);

        if (readto > fBuffer->size)
        {
            readto -= fBuffer->size;
            const uint32_t firstpart(fBuffer->size - tail);
            std::memcpy(bytebuf, fBuffer->buf + tail, firstpart);
            std::memcpy(bytebuf + firstpart, fBuffer->buf, readto);
        }
        else
        {
            std::memcpy(bytebuf, fBuffer->buf + tail, size);

            if (readto == fBuffer->size)
                readto = 0;
        }

        fBuffer->tail = readto;
        return true;
    }

    void tryWrite(const void* const buf, const uint32_t size) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(buf != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(size > 0,);
        CARLA_SAFE_ASSERT_RETURN(size < fBuffer->size,);

        const uint8_t* const bytebuf(static_cast<const uint8_t*>(buf));

        const uint32_t tail(fBuffer->tail);
        const uint32_t wrtn(fBuffer->wrtn);
        const uint32_t wrap((tail > wrtn) ? 0 : fBuffer->size);

        if (size >= wrap + tail - wrtn)
        {
            carla_stderr2("CarlaRingBuffer::tryWrite(%p, " P_SIZE "): failed, not enough space", buf, size);
            fBuffer->invalidateCommit = true;
            return;
        }

        uint32_t writeto(wrtn + size);

        if (writeto > fBuffer->size)
        {
            writeto -= fBuffer->size;
            const uint32_t firstpart(fBuffer->size - wrtn);
            std::memcpy(fBuffer->buf + wrtn, bytebuf, firstpart);
            std::memcpy(fBuffer->buf, bytebuf + firstpart, writeto);
        }
        else
        {
            std::memcpy(fBuffer->buf + wrtn, bytebuf, size);

            if (writeto == fBuffer->size)
                writeto = 0;
        }

        fBuffer->wrtn = writeto;
    }

private:
    BufferStruct* fBuffer;

    CARLA_PREVENT_VIRTUAL_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(CarlaRingBuffer)
};

// -----------------------------------------------------------------------
// CarlaRingBuffer using heap space

class CarlaHeapRingBuffer : public CarlaRingBuffer<HeapBuffer>
{
public:
    CarlaHeapRingBuffer() noexcept
        : fHeapBuffer(HeapBuffer_INIT) {}

    ~CarlaHeapRingBuffer() noexcept override
    {
        if (fHeapBuffer.buf == nullptr)
            return;

        delete[] fHeapBuffer.buf;
        fHeapBuffer.buf = nullptr;
    }

    void createBuffer(const uint32_t size) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fHeapBuffer.buf == nullptr,);
        CARLA_SAFE_ASSERT_RETURN(size > 0,);

        const uint32_t p2size(carla_nextPowerOf2(size));

        try {
            fHeapBuffer.buf = new uint8_t[p2size];
        } CARLA_SAFE_EXCEPTION_RETURN("CarlaHeapRingBuffer::createBuffer",);

        fHeapBuffer.size = p2size;
        setRingBuffer(&fHeapBuffer, true);
    }

    void deleteBuffer() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fHeapBuffer.buf != nullptr,);

        setRingBuffer(nullptr, false);

        delete[] fHeapBuffer.buf;
        fHeapBuffer.buf  = nullptr;
        fHeapBuffer.size = 0;
    }

private:
    HeapBuffer fHeapBuffer;

    CARLA_PREVENT_VIRTUAL_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(CarlaHeapRingBuffer)
};

// -----------------------------------------------------------------------
// CarlaRingBuffer using stack space

class CarlaStackRingBuffer : public CarlaRingBuffer<StackBuffer>
{
public:
    CarlaStackRingBuffer() noexcept
        : fStackBuffer(StackBuffer_INIT)
    {
        setRingBuffer(&fStackBuffer, true); // FIXME
    }

private:
    StackBuffer fStackBuffer;

    CARLA_PREVENT_VIRTUAL_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(CarlaStackRingBuffer)
};

// -----------------------------------------------------------------------

#endif // CARLA_RING_BUFFER_HPP_INCLUDED
