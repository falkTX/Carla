/*
 * LV2 Atom Ring Buffer
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef LV2_ATOM_RING_BUFFER_HPP_INCLUDED
#define LV2_ATOM_RING_BUFFER_HPP_INCLUDED

#include "CarlaMutex.hpp"
#include "CarlaRingBuffer.hpp"

#include "lv2/atom.h"

// -----------------------------------------------------------------------

class Lv2AtomRingBuffer : public CarlaRingBufferControl<HeapBuffer>
{
public:
    Lv2AtomRingBuffer() noexcept
        : fMutex(),
          fHeapBuffer(HeapBuffer_INIT),
          fNeedsDataDelete(true)
#ifdef CARLA_PROPER_CPP11_SUPPORT
        , fRetAtom{{0,0}, {0}}
#endif
    {
        carla_zeroStruct(fHeapBuffer);
    }

    Lv2AtomRingBuffer(Lv2AtomRingBuffer& ringBuf, uint8_t buf[]) noexcept
        : fMutex(),
          fHeapBuffer(HeapBuffer_INIT),
          fNeedsDataDelete(false)
#ifdef CARLA_PROPER_CPP11_SUPPORT
        , fRetAtom{{0,0}, {0}}
#endif
    {
        carla_zeroStruct(fHeapBuffer);

        fHeapBuffer.buf  = buf;
        fHeapBuffer.size = ringBuf.fHeapBuffer.size;

        {
            const CarlaMutexLocker cml(ringBuf.fMutex);
            fHeapBuffer.copyDataFrom(ringBuf.fHeapBuffer);
            ringBuf.clearData();
        }

        setRingBuffer(&fHeapBuffer, false);
    }

    ~Lv2AtomRingBuffer() noexcept
    {
        if (fHeapBuffer.buf == nullptr || ! fNeedsDataDelete)
            return;

        delete[] fHeapBuffer.buf;
        fHeapBuffer.buf = nullptr;
    }

    // -------------------------------------------------------------------

    void createBuffer(const uint32_t size) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fHeapBuffer.buf == nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fNeedsDataDelete,);
        CARLA_SAFE_ASSERT_RETURN(size > 0,);

        const uint32_t p2size(carla_nextPowerOf2(size));

        try {
            fHeapBuffer.buf = new uint8_t[p2size];
        } CARLA_SAFE_EXCEPTION_RETURN("Lv2AtomRingBuffer::createBuffer",);

        fHeapBuffer.size = p2size;
        setRingBuffer(&fHeapBuffer, true);
    }

    void deleteBuffer() noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fHeapBuffer.buf != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fNeedsDataDelete,);

        setRingBuffer(nullptr, false);

        delete[] fHeapBuffer.buf;
        fHeapBuffer.buf  = nullptr;
        fHeapBuffer.size = 0;
    }

    uint32_t getSize() const noexcept
    {
        return fHeapBuffer.size;
    }

    // -------------------------------------------------------------------

    bool tryLock() const noexcept
    {
        return fMutex.tryLock();
    }

    void unlock() const noexcept
    {
        fMutex.unlock();
    }

    // -------------------------------------------------------------------

    // NOTE: must have been locked before
    bool get(const LV2_Atom*& atom, uint32_t& portIndex) noexcept
    {
        if (const LV2_Atom* const retAtom = readAtom(portIndex))
        {
            atom = retAtom;
            return true;
        }

        return false;
    }

    // NOTE: must NOT been locked, we do that here
    bool put(const LV2_Atom* const atom, const uint32_t portIndex) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(atom != nullptr && atom->size > 0, false);

        const CarlaMutexLocker cml(fMutex);

        return writeAtom(atom, static_cast<int32_t>(portIndex));
    }

    // NOTE: must NOT been locked, we do that here
    bool putChunk(const LV2_Atom* const atom, const void* const data, const uint32_t portIndex) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(atom != nullptr && atom->size > 0, false);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, false);

        const CarlaMutexLocker cml(fMutex);

        return writeAtomChunk(atom, data, static_cast<int32_t>(portIndex));
    }

protected:
    // -------------------------------------------------------------------

    const LV2_Atom* readAtom(uint32_t& portIndex) noexcept
    {
        fRetAtom.atom.size = 0;
        fRetAtom.atom.type = 0;

        if (! tryRead(&fRetAtom.atom, sizeof(LV2_Atom)))
            return nullptr;
        if (fRetAtom.atom.size == 0 || fRetAtom.atom.type == 0)
            return nullptr;

        CARLA_SAFE_ASSERT_RETURN(fRetAtom.atom.size < kMaxAtomDataSize, nullptr);

        int32_t index = -1;
        if (! tryRead(&index, sizeof(int32_t)))
            return nullptr;
        if (index < 0)
            return nullptr;

        if (! tryRead(fRetAtom.data, fRetAtom.atom.size))
            return nullptr;

        portIndex = static_cast<uint32_t>(index);
        return &fRetAtom.atom;
    }

    // -------------------------------------------------------------------

    bool writeAtom(const LV2_Atom* const atom, const int32_t portIndex) noexcept
    {
        if (tryWrite(atom, sizeof(LV2_Atom)) && tryWrite(&portIndex, sizeof(int32_t)))
            tryWrite(LV2_ATOM_BODY_CONST(atom), atom->size);
        return commitWrite();
    }

    bool writeAtomChunk(const LV2_Atom* const atom, const void* const data, const int32_t portIndex) noexcept
    {
        if (tryWrite(atom, sizeof(LV2_Atom)) && tryWrite(&portIndex, sizeof(int32_t)))
            tryWrite(data, atom->size);
        return commitWrite();
    }

    // -------------------------------------------------------------------

private:
    CarlaMutex fMutex;
    HeapBuffer fHeapBuffer;
    const bool fNeedsDataDelete;

    static const std::size_t kMaxAtomDataSize = 32768 - sizeof(LV2_Atom);

    struct RetAtom {
        LV2_Atom atom;
        char     data[kMaxAtomDataSize];
    } fRetAtom;

    friend class Lv2AtomQueue;

    CARLA_PREVENT_VIRTUAL_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(Lv2AtomRingBuffer)
};

// -----------------------------------------------------------------------

#endif // LV2_ATOM_RING_BUFFER_HPP_INCLUDED
