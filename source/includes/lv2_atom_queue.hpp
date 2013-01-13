/*
 * Simple Queue, specially developed for Atom types
 * Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the COPYING file
 */

#ifndef __LV2_ATOM_QUEUE_HPP__
#define __LV2_ATOM_QUEUE_HPP__

#include "carla_utils.hpp"
#include "lv2/atom.h"

#include <cstring> // memcpy, memset
#include <pthread.h>

class Lv2AtomQueue
{
public:
    Lv2AtomQueue()
        : mutex(PTHREAD_MUTEX_INITIALIZER)
    {
        index = indexPool = 0;
        empty = true;
        full  = false;

        ::memset(dataPool, 0, sizeof(unsigned char)*MAX_POOL_SIZE);
    }

    void copyDataFrom(Lv2AtomQueue* const queue)
    {
        // lock mutexes
        queue->lock();
        lock();

        // copy data from queue
        ::memcpy(data, queue->data, sizeof(datatype)*MAX_SIZE);
        ::memcpy(dataPool, queue->dataPool, sizeof(unsigned char)*MAX_POOL_SIZE);
        index = queue->index;
        indexPool = queue->indexPool;
        empty = queue->empty;
        full  = queue->full;

        // unlock our mutex, no longer needed
        unlock();

        // reset queque
        ::memset(queue->data, 0, sizeof(datatype)*MAX_SIZE);
        ::memset(queue->dataPool, 0, sizeof(unsigned char)*MAX_POOL_SIZE);
        queue->index = queue->indexPool = 0;
        queue->empty = true;
        queue->full  = false;

        // unlock queque mutex
        queue->unlock();
    }

    bool isEmpty()
    {
        return empty;
    }

    bool isFull()
    {
        return full;
    }

    bool lock()
    {
        return (pthread_mutex_lock(&mutex) == 0);
    }

    bool tryLock()
    {
        return (pthread_mutex_trylock(&mutex) == 0);
    }

    bool unlock()
    {
        return (pthread_mutex_unlock(&mutex) == 0);
    }

    void put(const uint32_t portIndex, const LV2_Atom* const atom)
    {
        CARLA_ASSERT(atom && atom->size > 0);
        CARLA_ASSERT(indexPool + atom->size < MAX_POOL_SIZE); //  overflow

        if (full || atom->size == 0 || indexPool + atom->size >= MAX_POOL_SIZE)
            return;

        lock();

        for (unsigned short i=0; i < MAX_SIZE; i++)
        {
            if (data[i].size == 0)
            {
                data[i].portIndex  = portIndex;
                data[i].size       = atom->size;
                data[i].type       = atom->type;
                data[i].poolOffset = indexPool;
                ::memcpy(dataPool + indexPool, (const unsigned char*)LV2_ATOM_BODY_CONST(atom), atom->size);
                empty = false;
                full  = (i == MAX_SIZE-1);
                indexPool += atom->size;
                break;
            }
        }

        unlock();
    }

    bool get(uint32_t* const portIndex, const LV2_Atom** const atom)
    {
        CARLA_ASSERT(portIndex && atom);

        if (empty || ! (portIndex && atom))
            return false;

        if (! tryLock())
            return false;

        full = false;

        if (data[index].size == 0)
        {
            index = indexPool = 0;
            empty = true;

            unlock();
            return false;
        }

        retAtom.atom.size = data[index].size;
        retAtom.atom.type = data[index].type;
        ::memcpy(retAtom.data, dataPool + data[index].poolOffset, data[index].size);

        *portIndex = data[index].portIndex;
        *atom      = (LV2_Atom*)&retAtom;

        data[index].portIndex  = 0;
        data[index].size       = 0;
        data[index].type       = 0;
        data[index].poolOffset = 0;
        index++;
        empty = false;

        unlock();
        return true;
    }

private:
    struct datatype {
        size_t   size;
        uint32_t type;
        uint32_t portIndex;
        uint32_t poolOffset;

        datatype()
            : size(0),
              type(0),
              portIndex(0),
              poolOffset(0) {}
    };

    static const unsigned short MAX_SIZE = 128;
    static const unsigned short MAX_POOL_SIZE = 8192;

    datatype data[MAX_SIZE];
    unsigned char dataPool[MAX_POOL_SIZE];

    struct {
        LV2_Atom atom;
        unsigned char data[MAX_POOL_SIZE];
    } retAtom;

    unsigned short index, indexPool;
    bool empty, full;

    pthread_mutex_t mutex;
};

#endif // __LV2_ATOM_QUEUE_HPP__
