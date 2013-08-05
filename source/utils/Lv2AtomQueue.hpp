/*
 * Simple Queue, specially developed for Atom types
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

class Lv2AtomRingBufferControl : public RingBufferControl
{
public:
    Lv2AtomRingBufferControl()
        : RingBufferControl(&fBuffer)
    {
    }

    // -------------------------------------------------------------------

    const LV2_Atom* readAtom(uint32_t* const portIndex)
    {
        fRetAtom.atom.size = 0;
        fRetAtom.atom.type = 0;
        tryRead(&fRetAtom.atom, sizeof(LV2_Atom));

        if (fRetAtom.atom.size == 0 || fRetAtom.atom.type == 0)
            return nullptr;

        CARLA_SAFE_ASSERT_RETURN(fRetAtom.atom.size < RING_BUFFER_SIZE, nullptr);

        int32_t index = -1;
        tryRead(&index, sizeof(int32_t));

        if (index == -1)
            return nullptr;

        if (portIndex != nullptr)
            *portIndex = static_cast<uint32_t>(index);

        carla_zeroChar(fRetAtom.data, fRetAtom.atom.size);
        tryRead(fRetAtom.data, fRetAtom.atom.size);

        return &fRetAtom.atom;
    }

    // -------------------------------------------------------------------

    bool writeAtom(const LV2_Atom* const atom, const int32_t portIndex)
    {
        tryWrite(atom,       sizeof(LV2_Atom));
        tryWrite(&portIndex, sizeof(int32_t));
        tryWrite(LV2_ATOM_BODY_CONST(atom), atom->size);
        return commitWrite();
    }

    // -------------------------------------------------------------------

private:
    RingBuffer fBuffer;

    struct RetAtom {
        LV2_Atom atom;
        char     data[RING_BUFFER_SIZE];
    } fRetAtom;

    friend class Lv2AtomQueue;
};

// -----------------------------------------------------------------------

class Lv2AtomQueue
{
public:
    Lv2AtomQueue()
    {
    }

    void copyDataFrom(Lv2AtomQueue& queue)
    {
        // lock queue
        const CarlaMutex::ScopedLocker qsl(queue.fMutex);

        {
            // copy data from queue
            const CarlaMutex::ScopedLocker sl(fMutex);
            carla_copyStruct<RingBuffer>(fRingBufferCtrl.fBuffer, queue.fRingBufferCtrl.fBuffer);
        }

        // reset queque
        fRingBufferCtrl.clear();
    }

    void lock()
    {
        fMutex.lock();
    }

    bool tryLock()
    {
        return fMutex.tryLock();
    }

    void unlock()
    {
        fMutex.unlock();
    }

    bool put(const LV2_Atom* const atom, const uint32_t portIndex)
    {
        CARLA_SAFE_ASSERT_RETURN(atom != nullptr && atom->size > 0, false);

        return fRingBufferCtrl.writeAtom(atom, static_cast<int32_t>(portIndex));
    }

    bool get(const LV2_Atom** const atom, uint32_t* const portIndex)
    {
        CARLA_SAFE_ASSERT_RETURN(atom != nullptr && portIndex != nullptr, false);

        if (! fRingBufferCtrl.dataAvailable())
            return false;

        if (const LV2_Atom* retAtom = fRingBufferCtrl.readAtom(portIndex))
        {
            *atom = retAtom;
            return true;
        }

        return false;
    }

private:
    CarlaMutex fMutex;
    Lv2AtomRingBufferControl fRingBufferCtrl;

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Lv2AtomQueue)
};

// -----------------------------------------------------------------------

#endif // LV2_ATOM_QUEUE_HPP_INCLUDED
