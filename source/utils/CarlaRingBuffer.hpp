/*
 * Carla Ring Buffer
 * Copyright (C) 2013-2023 Filipe Coelho <falktx@falktx.com>
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

struct SmallStackBuffer {
    static constexpr const uint32_t size = 4096;
    uint32_t head, tail, wrtn;
    bool     invalidateCommit;
    uint8_t  buf[size];
};

struct BigStackBuffer {
    static constexpr const uint32_t size = 16384;
    uint32_t head, tail, wrtn;
    bool     invalidateCommit;
    uint8_t  buf[size];
};

struct HugeStackBuffer {
    static constexpr const uint32_t size = 65536;
    uint32_t head, tail, wrtn;
    bool     invalidateCommit;
    uint8_t  buf[size];
};

#ifdef CARLA_PROPER_CPP11_SUPPORT
# define HeapBuffer_INIT  {0, 0, 0, 0, false, nullptr}
# define StackBuffer_INIT {0, 0, 0, false, {0}}
#else
# define HeapBuffer_INIT
# define StackBuffer_INIT
#endif

// -----------------------------------------------------------------------
// CarlaRingBufferControl templated class

template <class BufferStruct>
class CarlaRingBufferControl
{
public:
    CarlaRingBufferControl() noexcept
        : fBuffer(nullptr),
          fErrorReading(false),
          fErrorWriting(false) {}

    virtual ~CarlaRingBufferControl() noexcept {}

    // -------------------------------------------------------------------

    void clearData() noexcept
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
        fErrorWriting = false;
        return true;
    }

    bool isDataAvailableForReading() const noexcept;

    bool isEmpty() const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);

        return (fBuffer->buf == nullptr || fBuffer->head == fBuffer->tail);
    }

    uint32_t getAvailableDataSize() const noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, 0);

        const uint32_t wrap((fBuffer->tail > fBuffer->wrtn) ? 0 : fBuffer->size);

        return wrap + fBuffer->tail - fBuffer->wrtn;
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
        CARLA_SAFE_ASSERT_RETURN(data != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(size > 0,);

        if (! tryRead(data, size))
            std::memset(data, 0, size);
    }

    template <typename T>
    void readCustomType(T& type) noexcept
    {
        if (! tryRead(&type, sizeof(T)))
            std::memset(&type, 0, sizeof(T));
    }

    // -------------------------------------------------------------------

    void skipRead(const uint32_t size) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(size > 0,);
        CARLA_SAFE_ASSERT_RETURN(size < fBuffer->size,);

        // empty
        if (fBuffer->head == fBuffer->tail)
            return;

        const uint32_t head = fBuffer->head;
        const uint32_t tail = fBuffer->tail;
        const uint32_t wrap = head > tail ? 0 : fBuffer->size;

        if (size > wrap + head - tail)
        {
            if (! fErrorReading)
            {
                fErrorReading = true;
                carla_stderr2("CarlaRingBuffer::skipRead(%u): failed, not enough space", size);
            }
            return;
        }

        uint32_t readto = tail + size;

        if (readto >= fBuffer->size)
            readto -= fBuffer->size;

        fBuffer->tail = readto;
        fErrorReading = false;
        return;
    }

    // -------------------------------------------------------------------

    bool writeBool(const bool value) noexcept
    {
        return tryWrite(&value, sizeof(bool));
    }

    bool writeByte(const uint8_t value) noexcept
    {
        return tryWrite(&value, sizeof(uint8_t));
    }

    bool writeShort(const int16_t value) noexcept
    {
        return tryWrite(&value, sizeof(int16_t));
    }

    bool writeUShort(const uint16_t value) noexcept
    {
        return tryWrite(&value, sizeof(uint16_t));
    }

    bool writeInt(const int32_t value) noexcept
    {
        return tryWrite(&value, sizeof(int32_t));
    }

    bool writeUInt(const uint32_t value) noexcept
    {
        return tryWrite(&value, sizeof(uint32_t));
    }

    bool writeLong(const int64_t value) noexcept
    {
        return tryWrite(&value, sizeof(int64_t));
    }

    bool writeULong(const uint64_t value) noexcept
    {
        return tryWrite(&value, sizeof(uint64_t));
    }

    bool writeFloat(const float value) noexcept
    {
        return tryWrite(&value, sizeof(float));
    }

    bool writeDouble(const double value) noexcept
    {
        return tryWrite(&value, sizeof(double));
    }

    bool writeCustomData(const void* const data, const uint32_t size) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(size > 0, false);

        return tryWrite(data, size);
    }

    template <typename T>
    bool writeCustomType(const T& type) noexcept
    {
        return tryWrite(&type, sizeof(T));
    }

    // -------------------------------------------------------------------

protected:
    void setRingBuffer(BufferStruct* const ringBuf, const bool resetBuffer) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != ringBuf,);

        fBuffer = ringBuf;

        if (resetBuffer && ringBuf != nullptr)
            clearData();
    }

    // -------------------------------------------------------------------

    bool tryRead(void* const buf, const uint32_t size) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);
       #if defined(__clang__)
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wtautological-pointer-compare"
       #endif
        CARLA_SAFE_ASSERT_RETURN(fBuffer->buf != nullptr, false);
       #if defined(__clang__)
        #pragma clang diagnostic pop
       #endif
        CARLA_SAFE_ASSERT_RETURN(buf != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(size > 0, false);
        CARLA_SAFE_ASSERT_RETURN(size < fBuffer->size, false);

        // empty
        if (fBuffer->head == fBuffer->tail)
            return false;

        uint8_t* const bytebuf = static_cast<uint8_t*>(buf);

        const uint32_t head = fBuffer->head;
        const uint32_t tail = fBuffer->tail;
        const uint32_t wrap = head > tail ? 0 : fBuffer->size;

        if (size > wrap + head - tail)
        {
            if (! fErrorReading)
            {
                fErrorReading = true;
                carla_stderr2("CarlaRingBuffer::tryRead(%p, %u): failed, not enough space", buf, size);
            }
            return false;
        }

        uint32_t readto = tail + size;

        if (readto > fBuffer->size)
        {
            readto -= fBuffer->size;

            if (size == 1)
            {
                std::memcpy(bytebuf, fBuffer->buf + tail, 1);
            }
            else
            {
                const uint32_t firstpart = fBuffer->size - tail;
                std::memcpy(bytebuf, fBuffer->buf + tail, firstpart);
                std::memcpy(bytebuf + firstpart, fBuffer->buf, readto);
            }
        }
        else
        {
            std::memcpy(bytebuf, fBuffer->buf + tail, size);

            if (readto == fBuffer->size)
                readto = 0;
        }

        fBuffer->tail = readto;
        fErrorReading = false;
        return true;
    }

    bool tryWrite(const void* const buf, const uint32_t size) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(buf != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(size > 0, false);
        CARLA_SAFE_ASSERT_UINT2_RETURN(size < fBuffer->size, size, fBuffer->size, false);

        const uint8_t* const bytebuf = static_cast<const uint8_t*>(buf);

        const uint32_t tail = fBuffer->tail;
        const uint32_t wrtn = fBuffer->wrtn;
        const uint32_t wrap = tail > wrtn ? 0 : fBuffer->size;

        if (size >= wrap + tail - wrtn)
        {
            if (! fErrorWriting)
            {
                fErrorWriting = true;
                carla_stderr2("CarlaRingBuffer::tryWrite(%p, %u): failed, not enough space", buf, size);
            }
            fBuffer->invalidateCommit = true;
            return false;
        }

        uint32_t writeto = wrtn + size;

        if (writeto > fBuffer->size)
        {
            writeto -= fBuffer->size;

            if (size == 1)
            {
                std::memcpy(fBuffer->buf, bytebuf, 1);
            }
            else
            {
                const uint32_t firstpart = fBuffer->size - wrtn;
                std::memcpy(fBuffer->buf + wrtn, bytebuf, firstpart);
                std::memcpy(fBuffer->buf, bytebuf + firstpart, writeto);
            }
        }
        else
        {
            std::memcpy(fBuffer->buf + wrtn, bytebuf, size);

            if (writeto == fBuffer->size)
                writeto = 0;
        }

        fBuffer->wrtn = writeto;
        return true;
    }

private:
    BufferStruct* fBuffer;

    // wherever read/write errors have been printed to terminal
    bool fErrorReading;
    bool fErrorWriting;

    CARLA_PREVENT_VIRTUAL_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPYABLE(CarlaRingBufferControl)
};

template <class BufferStruct>
inline bool CarlaRingBufferControl<BufferStruct>::isDataAvailableForReading() const noexcept
{
	return (fBuffer != nullptr && fBuffer->head != fBuffer->tail);
}

template <>
inline bool CarlaRingBufferControl<HeapBuffer>::isDataAvailableForReading() const noexcept
{
	return (fBuffer != nullptr && fBuffer->buf != nullptr && fBuffer->head != fBuffer->tail);
}

// -----------------------------------------------------------------------
// CarlaRingBuffer using heap space

class CarlaHeapRingBuffer : public CarlaRingBufferControl<HeapBuffer>
{
public:
    CarlaHeapRingBuffer() noexcept
        : fHeapBuffer(HeapBuffer_INIT)
    {
        carla_zeroStruct(fHeapBuffer);
    }

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

        const uint32_t p2size = carla_nextPowerOf2(size);

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
    CARLA_DECLARE_NON_COPYABLE(CarlaHeapRingBuffer)
};

// -----------------------------------------------------------------------
// CarlaRingBuffer using small stack space

class CarlaSmallStackRingBuffer : public CarlaRingBufferControl<SmallStackBuffer>
{
public:
    CarlaSmallStackRingBuffer() noexcept
        : fStackBuffer(StackBuffer_INIT)
    {
        setRingBuffer(&fStackBuffer, true);
    }

private:
    SmallStackBuffer fStackBuffer;

    CARLA_PREVENT_VIRTUAL_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPYABLE(CarlaSmallStackRingBuffer)
};

// -----------------------------------------------------------------------

#endif // CARLA_RING_BUFFER_HPP_INCLUDED
