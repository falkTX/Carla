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

//#include "CarlaLv2Utils.hpp"
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

    const RetAtom* readAtom(uint32_t* const portIndex)
    {
        fRetAtom.atom.size = 0;
        fRetAtom.atom.type = 0;
        tryRead(&fRetAtom.atom, sizeof(LV2_Atom));

        if (fRetAtom.atom.size == 0 || fRetAtom.atom.type == 0)
            return nullptr;

        int32_t index = -1;
        tryRead(&index, sizeof(int32_t));

        if (index == -1)
            return nullptr;

        *portIndex = index;

        tryRead(fRetAtom.data, fRetAtom.atom.size);
        return &RetAtom;
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
        unsigned char data[RING_BUFFER_SIZE];
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

        const CarlaMutex::ScopedLocker sl(fMutex);

        return fRingBufferCtrl.writeAtom(atom, portIndex);
    }

    bool get(const LV2_Atom** const atom, uint32_t* const portIndex)
    {
        CARLA_SAFE_ASSERT_RETURN(portIndex != nullptr && atom != nullptr, false);

        if (! fRingBufferCtrl.dataAvailable())
            return false;

        const LV2_Atom atom     = fRingBufferCtrl.readAtom();
        const int32_t portIndex = fRingBufferCtrl.readAndCheckInt();
        const uint8_t* atomBody = fRingBufferCtrl.readAtomBody();

//         fFull = false;
//
//         if (fData[fIndex].size == 0)
//         {
//             fIndex = fIndexPool = 0;
//             fEmpty = true;
//
//             unlock();
//             return false;
//         }
//
//         fRetAtom.atom.size = fData[fIndex].size;
//         fRetAtom.atom.type = fData[fIndex].type;
//         std::memcpy(fRetAtom.data, fDataPool + fData[fIndex].poolOffset, fData[fIndex].size);
//
//         *portIndex = fData[fIndex].portIndex;
//         *atom      = (LV2_Atom*)&fRetAtom;
//
//         fData[fIndex].portIndex  = 0;
//         fData[fIndex].size       = 0;
//         fData[fIndex].type       = 0;
//         fData[fIndex].poolOffset = 0;
//         fEmpty = false;
//         ++fIndex;

        return true;
    }

private:
    CarlaMutex fMutex;
    Lv2AtomRingBufferControl fRingBufferCtrl;

    CARLA_PREVENT_HEAP_ALLOCATION

};

// -----------------------------------------------------------------------

#endif // LV2_ATOM_QUEUE_HPP_INCLUDED
