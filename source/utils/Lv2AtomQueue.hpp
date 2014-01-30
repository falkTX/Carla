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

#include "CarlaMutex.hpp"
#include "CarlaRingBuffer.hpp"

#include "lv2/atom.h"

// -----------------------------------------------------------------------

class Lv2AtomRingBufferControl : public RingBufferControlTemplate<HeapRingBuffer>
{
public:
    Lv2AtomRingBufferControl()
        : RingBufferControlTemplate(nullptr)
    {
        fBuffer.size = 0;
        fBuffer.buf  = nullptr;
    }

    // -------------------------------------------------------------------

    void createBuffer(const uint32_t size)
    {
        if (fBuffer.buf != nullptr)
            delete[] fBuffer.buf;

        fBuffer.size = size;
        fBuffer.buf  = new char[size];
        setRingBuffer(&fBuffer, true);
    }

    // used for tmp buffers only
    void copyDump(HeapRingBuffer& rb, char dumpBuf[])
    {
        CARLA_SAFE_ASSERT_RETURN(fBuffer.size == 0,);
        CARLA_SAFE_ASSERT_RETURN(fBuffer.buf == nullptr,);

        fBuffer.buf  = dumpBuf;
        fBuffer.size = rb.size;
        fBuffer.head = rb.head;
        fBuffer.tail = rb.tail;
        fBuffer.written = rb.written;
        fBuffer.invalidateCommit = rb.invalidateCommit;

        std::memcpy(dumpBuf, rb.buf, rb.size);
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

    // -------------------------------------------------------------------

private:
    HeapRingBuffer fBuffer;

    static const size_t kMaxDataSize = 2048;

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
    Lv2AtomQueue()
    {
    }

    // -------------------------------------------------------------------

    void createBuffer(const uint32_t size)
    {
        fRingBufferCtrl.createBuffer(size);
    }

    // -------------------------------------------------------------------

    uint32_t getSize() const noexcept
    {
        return fRingBufferCtrl.fBuffer.size;
    }

    bool isEmpty() const noexcept
    {
        return !fRingBufferCtrl.isDataAvailable();
    }

    // must have been locked before
    bool get(const LV2_Atom** const atom, uint32_t* const portIndex)
    {
        CARLA_SAFE_ASSERT_RETURN(atom != nullptr && portIndex != nullptr, false);

        if (! fRingBufferCtrl.isDataAvailable())
            return false;

        if (const LV2_Atom* retAtom = fRingBufferCtrl.readAtom(portIndex))
        {
            *atom = retAtom;
            return true;
        }

        return false;
    }

    // must NOT been locked, we do that here
    bool put(const LV2_Atom* const atom, const uint32_t portIndex)
    {
        CARLA_SAFE_ASSERT_RETURN(atom != nullptr && atom->size > 0, false);

        const CarlaMutex::ScopedLocker sl(fMutex);

        return fRingBufferCtrl.writeAtom(atom, static_cast<int32_t>(portIndex));
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

    void copyDataFromQueue(Lv2AtomQueue& queue)
    {
        // lock source
        const CarlaMutex::ScopedLocker qsl(queue.fMutex);

        {
            // copy data from source
            const CarlaMutex::ScopedLocker sl(fMutex);
            fRingBufferCtrl.fBuffer = queue.fRingBufferCtrl.fBuffer;
        }

        // clear source
        queue.fRingBufferCtrl.clear();
    }

    void copyAndDumpDataFromQueue(Lv2AtomQueue& queue, char dumpBuf[])
    {
        // lock source
        const CarlaMutex::ScopedLocker qsl(queue.fMutex);

        {
            // copy data from source
            const CarlaMutex::ScopedLocker sl(fMutex);
            fRingBufferCtrl.copyDump(queue.fRingBufferCtrl.fBuffer, dumpBuf);
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
