/*
 * Simple Queue, specially developed for Atom types
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

#ifndef LV2_ATOM_QUEUE_HPP_INCLUDED
#define LV2_ATOM_QUEUE_HPP_INCLUDED

#include "CarlaMathUtils.hpp"
#include "CarlaMutex.hpp"
#include "CarlaRingBuffer.hpp"

#include "lv2/atom.h"

// -----------------------------------------------------------------------

class Lv2AtomRingBufferControl : public RingBufferControl<HeapBuffer>
{
public:
    Lv2AtomRingBufferControl() noexcept
        : RingBufferControl<HeapBuffer>(nullptr),
          fIsDummy(false)
    {
        fLv2Buffer.size = 0;
        fLv2Buffer.buf  = nullptr;
    }

    ~Lv2AtomRingBufferControl() noexcept
    {
        if (fLv2Buffer.buf != nullptr && ! fIsDummy)
        {
            delete[] fLv2Buffer.buf;
            fLv2Buffer.buf = nullptr;
        }
    }

    // -------------------------------------------------------------------

    void createBuffer(const uint32_t size) noexcept
    {
        if (fLv2Buffer.size == size && ! fIsDummy)
            return;

        if (fLv2Buffer.buf != nullptr)
        {
            if (! fIsDummy)
                delete[] fLv2Buffer.buf;
            fLv2Buffer.buf = nullptr;
        }

        // shouldn't really happen please...
        CARLA_SAFE_ASSERT_RETURN(size > 0,);

        const uint32_t p2size(carla_nextPowerOf2(size));

        try {
            fLv2Buffer.buf = new char[p2size];
        } CARLA_SAFE_EXCEPTION_RETURN("Lv2AtomRingBufferControl::createBuffer",);

        fLv2Buffer.size = p2size;
        setRingBuffer(&fLv2Buffer, true);
        lockMemory();
    }

    // used for tmp buffers only
    void copyDump(HeapBuffer& rb, char dumpBuf[]) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(fLv2Buffer.size == 0,);
        CARLA_SAFE_ASSERT_RETURN(fLv2Buffer.buf == nullptr,);

        fLv2Buffer.buf  = dumpBuf;
        fLv2Buffer.size = rb.size;
        fLv2Buffer.head = rb.head;
        fLv2Buffer.tail = rb.tail;
        fLv2Buffer.written = rb.written;
        fLv2Buffer.invalidateCommit = rb.invalidateCommit;
        fIsDummy = true;

        std::memcpy(dumpBuf, rb.buf, rb.size);

        setRingBuffer(&fLv2Buffer, false);
    }

    // -------------------------------------------------------------------

    const LV2_Atom* readAtom(uint32_t* const portIndex) noexcept
    {
        fRetAtom.atom.size = 0;
        fRetAtom.atom.type = 0;
        tryRead(&fRetAtom.atom, sizeof(LV2_Atom));

        if (fRetAtom.atom.size == 0 || fRetAtom.atom.type == 0)
            return nullptr;

        CARLA_SAFE_ASSERT_RETURN(fRetAtom.atom.size < kMaxDataSize, nullptr);

        int32_t index = -1;
        tryRead(&index, sizeof(int32_t));

        if (index < 0)
            return nullptr;

        if (portIndex != nullptr)
            *portIndex = static_cast<uint32_t>(index);

        carla_zeroChar(fRetAtom.data, fRetAtom.atom.size);
        tryRead(fRetAtom.data, fRetAtom.atom.size);

        return &fRetAtom.atom;
    }

    // -------------------------------------------------------------------

    bool writeAtom(const LV2_Atom* const atom, const int32_t portIndex) noexcept
    {
        tryWrite(atom,       sizeof(LV2_Atom));
        tryWrite(&portIndex, sizeof(int32_t));
        tryWrite(LV2_ATOM_BODY_CONST(atom), atom->size);
        return commitWrite();
    }

    bool writeAtomChunk(const LV2_Atom* const atom, const void* const data, const int32_t portIndex) noexcept
    {
        tryWrite(atom,       sizeof(LV2_Atom));
        tryWrite(&portIndex, sizeof(int32_t));
        tryWrite(data,       atom->size);
        return commitWrite();
    }

    // -------------------------------------------------------------------

private:
    HeapBuffer fLv2Buffer;
    bool fIsDummy;

    static const size_t kMaxDataSize = 8192;

    struct RetAtom {
        LV2_Atom atom;
        char     data[kMaxDataSize];
    } fRetAtom;

    friend class Lv2AtomQueue;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(Lv2AtomRingBufferControl)
};

// -----------------------------------------------------------------------

class Lv2AtomQueue
{
public:
    Lv2AtomQueue() noexcept {}

    // -------------------------------------------------------------------

    void createBuffer(const uint32_t size) noexcept
    {
        fRingBufferCtrl.createBuffer(size);
    }

    // -------------------------------------------------------------------

    uint32_t getSize() const noexcept
    {
        return fRingBufferCtrl.fLv2Buffer.size;
    }

    bool isEmpty() const noexcept
    {
        return (fRingBufferCtrl.fLv2Buffer.buf == nullptr || !fRingBufferCtrl.isDataAvailable());
    }

    // must have been locked before
    bool get(const LV2_Atom** const atom, uint32_t* const portIndex) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(atom != nullptr && portIndex != nullptr, false);

        if (! fRingBufferCtrl.isDataAvailable())
            return false;

        if (const LV2_Atom* const retAtom = fRingBufferCtrl.readAtom(portIndex))
        {
            *atom = retAtom;
            return true;
        }

        return false;
    }

    // must NOT been locked, we do that here
    bool put(const LV2_Atom* const atom, const uint32_t portIndex) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(atom != nullptr && atom->size > 0, false);

        const CarlaMutexLocker cml(fMutex);

        return fRingBufferCtrl.writeAtom(atom, static_cast<int32_t>(portIndex));
    }

    // must NOT been locked, we do that here
    bool putChunk(const LV2_Atom* const atom, const void* const data, const uint32_t portIndex) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(atom != nullptr && atom->size > 0, false);
        CARLA_SAFE_ASSERT_RETURN(data != nullptr, false);

        const CarlaMutexLocker cml(fMutex);

        return fRingBufferCtrl.writeAtomChunk(atom, data, static_cast<int32_t>(portIndex));
    }

    // -------------------------------------------------------------------

    void lock() const noexcept
    {
        fMutex.lock();
    }

    bool tryLock() const noexcept
    {
        return fMutex.tryLock();
    }

    void unlock() const noexcept
    {
        fMutex.unlock();
    }

    // -------------------------------------------------------------------

    void copyDataFromQueue(Lv2AtomQueue& queue) noexcept
    {
        // lock source
        const CarlaMutexLocker cml1(queue.fMutex);

        {
            // copy data from source
            const CarlaMutexLocker cml2(fMutex);
            fRingBufferCtrl.fLv2Buffer = queue.fRingBufferCtrl.fLv2Buffer;
        }

        // clear source
        queue.fRingBufferCtrl.clear();
    }

    // used for tmp buffers only
    void copyAndDumpDataFromQueue(Lv2AtomQueue& queue, char dumpBuf[]) noexcept
    {
        // lock source
        const CarlaMutexLocker cml1(queue.fMutex);

        {
            // copy data from source
            const CarlaMutexLocker cml2(fMutex);
            fRingBufferCtrl.copyDump(queue.fRingBufferCtrl.fLv2Buffer, dumpBuf);
        }

        // clear source
        queue.fRingBufferCtrl.clear();
    }

    // -------------------------------------------------------------------

private:
    CarlaMutex fMutex;
    Lv2AtomRingBufferControl fRingBufferCtrl;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPY_CLASS(Lv2AtomQueue)
};

// -----------------------------------------------------------------------

#endif // LV2_ATOM_QUEUE_HPP_INCLUDED
