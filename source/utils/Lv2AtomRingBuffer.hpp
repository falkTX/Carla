/*
 * LV2 Atom Ring Buffer
 * Copyright (C) 2012-2023 Filipe Coelho <falktx@falktx.com>
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

// --------------------------------------------------------------------------------------------------------------------

class Lv2AtomRingBuffer : public CarlaRingBufferControl<HeapBuffer>
{
public:
    Lv2AtomRingBuffer() noexcept
        : fMutex(),
          fHeapBuffer{0, 0, 0, 0, false, nullptr},
          fNeedsDataDelete(true)
    {
    }

    Lv2AtomRingBuffer(Lv2AtomRingBuffer& ringBuf, uint8_t buf[]) noexcept
        : fMutex(),
          fHeapBuffer{0, 0, 0, 0, false, nullptr},
          fNeedsDataDelete(false)
    {
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

    // ----------------------------------------------------------------------------------------------------------------

    void createBuffer(const uint32_t size, const bool mlock) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fHeapBuffer.buf == nullptr,);
        CARLA_SAFE_ASSERT_RETURN(fNeedsDataDelete,);
        CARLA_SAFE_ASSERT_RETURN(size > 0,);

        const uint32_t p2size = carla_nextPowerOf2(size);

        try {
            fHeapBuffer.buf = new uint8_t[p2size];
        } CARLA_SAFE_EXCEPTION_RETURN("Lv2AtomRingBuffer::createBuffer",);

        fHeapBuffer.size = p2size;
        setRingBuffer(&fHeapBuffer, true);

        if (mlock)
        {
            carla_mlock(&fHeapBuffer, sizeof(fHeapBuffer));
            carla_mlock(fHeapBuffer.buf, p2size);
        }
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

    // ----------------------------------------------------------------------------------------------------------------

    bool tryLock() const noexcept
    {
        return fMutex.tryLock();
    }

    void unlock() const noexcept
    {
        fMutex.unlock();
    }

    // ----------------------------------------------------------------------------------------------------------------

    // NOTE: must have been locked before
    bool get(uint32_t& portIndex, LV2_Atom* const retAtom) noexcept
    {
        if (readAtom(portIndex, retAtom))
            return true;

        retAtom->size = retAtom->type = 0;
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
    // ----------------------------------------------------------------------------------------------------------------

    bool readAtom(uint32_t& portIndex, LV2_Atom* const retAtom) noexcept
    {
        const uint32_t maxAtomSize = retAtom->size - sizeof(LV2_Atom);

        LV2_Atom atom = {};

        if (! tryRead(&atom, sizeof(LV2_Atom)))
            return false;
        if (atom.size == 0 || atom.type == 0)
            return false;

        CARLA_SAFE_ASSERT_UINT2_RETURN(atom.size < maxAtomSize, atom.size, maxAtomSize, false);

        int32_t index = -1;
        if (! tryRead(&index, sizeof(int32_t)))
            return false;
        if (index < 0)
            return false;

        if (! tryRead(retAtom + 1, atom.size))
            return false;

        portIndex = static_cast<uint32_t>(index);
        retAtom->size = atom.size;
        retAtom->type = atom.type;
        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------

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

    // ----------------------------------------------------------------------------------------------------------------

private:
    CarlaMutex fMutex;
    HeapBuffer fHeapBuffer;
    const bool fNeedsDataDelete;

    friend class Lv2AtomQueue;

    CARLA_PREVENT_VIRTUAL_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPYABLE(Lv2AtomRingBuffer)
};

// --------------------------------------------------------------------------------------------------------------------

#endif // LV2_ATOM_RING_BUFFER_HPP_INCLUDED
