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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __LV2_ATOM_QUEUE_HPP__
#define __LV2_ATOM_QUEUE_HPP__

#include "CarlaLv2Utils.hpp"
#include "CarlaMutex.hpp"

class Lv2AtomQueue
{
public:
    Lv2AtomQueue()
        : fIndex(0),
          fIndexPool(0),
          fEmpty(true),
          fFull(false)
    {
        std::memset(fDataPool, 0, sizeof(unsigned char)*MAX_POOL_SIZE);
    }

    void copyDataFrom(Lv2AtomQueue* const queue)
    {
        // lock mutexes
        queue->lock();
        lock();

        // copy data from queue
        std::memcpy(fData, queue->fData, sizeof(DataType)*MAX_SIZE);
        std::memcpy(fDataPool, queue->fDataPool, sizeof(unsigned char)*MAX_POOL_SIZE);
        fIndex = queue->fIndex;
        fIndexPool = queue->fIndexPool;
        fEmpty = queue->fEmpty;
        fFull  = queue->fFull;

        // unlock our mutex, no longer needed
        unlock();

        // reset queque
        std::memset(queue->fData, 0, sizeof(DataType)*MAX_SIZE);
        std::memset(queue->fDataPool, 0, sizeof(unsigned char)*MAX_POOL_SIZE);
        queue->fIndex = queue->fIndexPool = 0;
        queue->fEmpty = true;
        queue->fFull  = false;

        // unlock queque mutex
        queue->unlock();
    }

    bool isEmpty() const
    {
        return fEmpty;
    }

    bool isFull() const
    {
        return fFull;
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

    bool put(const uint32_t portIndex, const LV2_Atom* const atom)
    {
        CARLA_ASSERT(atom != nullptr && atom->size > 0);
        CARLA_ASSERT(fIndexPool + atom->size < MAX_POOL_SIZE); // overflow

        if (fFull || atom == nullptr || fIndexPool + atom->size >= MAX_POOL_SIZE)
            return false;
        if (atom->size == 0)
            return true;

        bool ret = false;

        lock();

        for (unsigned short i=0; i < MAX_SIZE; ++i)
        {
            if (fData[i].size == 0)
            {
                fData[i].portIndex  = portIndex;
                fData[i].size       = atom->size;
                fData[i].type       = atom->type;
                fData[i].poolOffset = fIndexPool;
                std::memcpy(fDataPool + fIndexPool, LV2NV_ATOM_BODY_CONST(atom), atom->size);
                fEmpty = false;
                fFull  = (i == MAX_SIZE-1);
                fIndexPool += atom->size;
                ret = true;
                break;
            }
        }

        unlock();

        return ret;
    }

    // needs to be locked first!
    bool get(uint32_t* const portIndex, const LV2_Atom** const atom)
    {
        CARLA_ASSERT(portIndex != nullptr && atom != nullptr);

        if (fEmpty || portIndex == nullptr || atom == nullptr)
            return false;

        fFull = false;

        if (fData[fIndex].size == 0)
        {
            fIndex = fIndexPool = 0;
            fEmpty = true;

            unlock();
            return false;
        }

        fRetAtom.atom.size = fData[fIndex].size;
        fRetAtom.atom.type = fData[fIndex].type;
        std::memcpy(fRetAtom.data, fDataPool + fData[fIndex].poolOffset, fData[fIndex].size);

        *portIndex = fData[index].portIndex;
        *atom      = (LV2_Atom*)&fRetAtom;

        fData[fIndex].portIndex  = 0;
        fData[fIndex].size       = 0;
        fData[fIndex].type       = 0;
        fData[fIndex].poolOffset = 0;
        fEmpty = false;
        ++fIndex;

        return true;
    }

private:
    struct DataType {
        size_t   size;
        uint32_t type;
        uint32_t portIndex;
        uint32_t poolOffset;

        DataType()
            : size(0),
              type(0),
              portIndex(0),
              poolOffset(0) {}
    };

    static const unsigned short MAX_SIZE = 128;
    static const unsigned short MAX_POOL_SIZE = 8192;

    DataType fData[MAX_SIZE];
    unsigned char fDataPool[MAX_POOL_SIZE];

    struct RetAtom {
        LV2_Atom atom;
        unsigned char data[MAX_POOL_SIZE];
#ifdef CARLA_PROPER_CPP11_SUPPORT
        RetAtom() = delete;
        RetAtom(RetAtom&) = delete;
        RetAtom(const RetAtom&) = delete;
#endif
    } fRetAtom;

    unsigned short fIndex, fIndexPool;
    bool fEmpty, fFull;

    CarlaMutex fMutex;
};

#endif // __LV2_ATOM_QUEUE_HPP__
