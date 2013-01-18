/*
 * RealTime Memory Pool, heavily based on work by Nedko Arnaudov
 * Copyright (C) 2006-2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#include "rtmempool.h"
#include "list.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ------------------------------------------------------------------------------------------------

typedef struct list_head k_list_head;

// ------------------------------------------------------------------------------------------------

typedef struct _RtMemPool
{
    char name[RTSAFE_MEMORY_POOL_NAME_MAX];

    size_t dataSize;
    size_t minPreallocated;
    size_t maxPreallocated;

    k_list_head used;
    unsigned int usedCount;

    k_list_head unused;
    unsigned int unusedCount;

    bool enforceThreadSafety;

    // next members are initialized/used only if enforceThreadSafety is true
    pthread_mutex_t mutex;
    unsigned int unusedCount2;
    k_list_head pending;
    size_t usedSize;

} RtMemPool;

// ------------------------------------------------------------------------------------------------
// adjust unused list size

void rtsafe_memory_pool_sleepy(RtMemPool* poolPtr)
{
    k_list_head* nodePtr;
    unsigned int count;

    if (poolPtr->enforceThreadSafety)
    {
        pthread_mutex_lock(&poolPtr->mutex);

        count = poolPtr->unusedCount2;

        assert(poolPtr->minPreallocated < poolPtr->maxPreallocated);

        while (count < poolPtr->minPreallocated)
        {
            nodePtr = malloc(sizeof(k_list_head) + poolPtr->dataSize);

            if (nodePtr == NULL)
            {
                break;
            }

            list_add_tail(nodePtr, &poolPtr->pending);

            count++;

            poolPtr->usedSize += poolPtr->dataSize;
        }

        while (count > poolPtr->maxPreallocated && ! list_empty(&poolPtr->pending))
        {
            nodePtr = poolPtr->pending.next;

            list_del(nodePtr);

            free(nodePtr);

            count--;

            poolPtr->usedSize -= poolPtr->dataSize;
        }

        pthread_mutex_unlock(&poolPtr->mutex);
    }
    else
    {
        while (poolPtr->unusedCount < poolPtr->minPreallocated)
        {
            nodePtr = malloc(sizeof(k_list_head) + poolPtr->dataSize);

            if (nodePtr == NULL)
            {
                return;
            }

            list_add_tail(nodePtr, &poolPtr->unused);
            poolPtr->unusedCount++;
            poolPtr->usedSize += poolPtr->dataSize;
        }

        while (poolPtr->unusedCount > poolPtr->maxPreallocated)
        {
            assert(! list_empty(&poolPtr->unused));

            nodePtr = poolPtr->unused.next;

            list_del(nodePtr);
            poolPtr->unusedCount--;

            free(nodePtr);
            poolPtr->usedSize -= poolPtr->dataSize;
        }
    }
}

// ------------------------------------------------------------------------------------------------

bool rtsafe_memory_pool_create2(RtMemPool_Handle* handlePtr,
                                const char* poolName,
                                size_t dataSize,
                                size_t minPreallocated,
                                size_t maxPreallocated,
                                bool enforceThreadSafety)
{
    assert(minPreallocated <= maxPreallocated);
    assert(poolName == NULL || strlen(poolName) < RTSAFE_MEMORY_POOL_NAME_MAX);

    RtMemPool* poolPtr;

    poolPtr = malloc(sizeof(RtMemPool));

    if (poolPtr == NULL)
    {
        return false;
    }

    if (poolName != NULL)
    {
        strcpy(poolPtr->name, poolName);
    }
    else
    {
        sprintf(poolPtr->name, "%p", poolPtr);
    }

    poolPtr->dataSize = dataSize;
    poolPtr->minPreallocated = minPreallocated;
    poolPtr->maxPreallocated = maxPreallocated;

    INIT_LIST_HEAD(&poolPtr->used);
    poolPtr->usedCount = 0;

    INIT_LIST_HEAD(&poolPtr->unused);
    poolPtr->unusedCount = 0;

    poolPtr->enforceThreadSafety = enforceThreadSafety;

    if (enforceThreadSafety)
    {
        if (pthread_mutex_init(&poolPtr->mutex, NULL) != 0)
        {
            free(poolPtr);
            return false;
        }

        INIT_LIST_HEAD(&poolPtr->pending);
    }

    poolPtr->unusedCount2 = 0;
    poolPtr->usedSize = 0;

    rtsafe_memory_pool_sleepy(poolPtr);
    *handlePtr = (RtMemPool_Handle)poolPtr;

    return true;
}

// ------------------------------------------------------------------------------------------------

bool rtsafe_memory_pool_create(RtMemPool_Handle* handlePtr,
                               const char* poolName,
                               size_t dataSize,
                               size_t minPreallocated,
                               size_t maxPreallocated)
{
    return rtsafe_memory_pool_create2(handlePtr, poolName, dataSize, minPreallocated, maxPreallocated, false);
}

// ------------------------------------------------------------------------------------------------

bool rtsafe_memory_pool_create_safe(RtMemPool_Handle* handlePtr,
                                    const char* poolName,
                                    size_t dataSize,
                                    size_t minPreallocated,
                                    size_t maxPreallocated)
{
    return rtsafe_memory_pool_create2(handlePtr, poolName, dataSize, minPreallocated, maxPreallocated, true);
}

// ------------------------------------------------------------------------------------------------

void rtsafe_memory_pool_destroy(RtMemPool_Handle handle)
{
    assert(handle);

    k_list_head* nodePtr;
    RtMemPool* poolPtr = (RtMemPool*)handle;

    // caller should deallocate all chunks prior releasing pool itself
    if (poolPtr->usedCount != 0)
    {
        assert(0);
    }

    while (poolPtr->unusedCount != 0)
    {
        assert(! list_empty(&poolPtr->unused));

        nodePtr = poolPtr->unused.next;

        list_del(nodePtr);
        poolPtr->unusedCount--;

        free(nodePtr);
    }

    assert(list_empty(&poolPtr->unused));

    if (poolPtr->enforceThreadSafety)
    {
        while (! list_empty(&poolPtr->pending))
        {
            nodePtr = poolPtr->pending.next;

            list_del(nodePtr);

            free(nodePtr);
        }

        int ret = pthread_mutex_destroy(&poolPtr->mutex);

#ifdef DEBUG
        assert(ret == 0);
#else
        // unused
        (void)ret;
#endif
    }

    free(poolPtr);
}

// ------------------------------------------------------------------------------------------------
// find entry in unused list, fail if it is empty

void* rtsafe_memory_pool_allocate_atomic(RtMemPool_Handle handle)
{
    assert(handle);

    k_list_head* nodePtr;
    RtMemPool* poolPtr = (RtMemPool*)handle;

    if (list_empty(&poolPtr->unused))
    {
        return NULL;
    }

    nodePtr = poolPtr->unused.next;
    list_del(nodePtr);

    poolPtr->unusedCount--;
    poolPtr->usedCount++;

    list_add_tail(nodePtr, &poolPtr->used);

    if (poolPtr->enforceThreadSafety && pthread_mutex_trylock(&poolPtr->mutex) == 0)
    {
        while (poolPtr->unusedCount < poolPtr->minPreallocated && ! list_empty(&poolPtr->pending))
        {
            nodePtr = poolPtr->pending.next;

            list_del(nodePtr);
            list_add_tail(nodePtr, &poolPtr->unused);

            poolPtr->unusedCount++;
        }

        poolPtr->unusedCount2 = poolPtr->unusedCount;

        pthread_mutex_unlock(&poolPtr->mutex);
    }

    return (nodePtr + 1);
}

// ------------------------------------------------------------------------------------------------

void* rtsafe_memory_pool_allocate_sleepy(RtMemPool_Handle handle)
{
    assert(handle);

    void* data;
    RtMemPool* poolPtr = (RtMemPool*)handle;

    do {
        rtsafe_memory_pool_sleepy(poolPtr);
        data = rtsafe_memory_pool_allocate_atomic((RtMemPool_Handle)poolPtr);
    }
    while (data == NULL);

    return data;
}

// ------------------------------------------------------------------------------------------------
// move from used to unused list

void rtsafe_memory_pool_deallocate(RtMemPool_Handle handle, void* memoryPtr)
{
    assert(handle);

    k_list_head* nodePtr;
    RtMemPool* poolPtr = (RtMemPool*)handle;

    list_del((k_list_head*)memoryPtr - 1);
    list_add_tail((k_list_head*)memoryPtr - 1, &poolPtr->unused);
    poolPtr->usedCount--;
    poolPtr->unusedCount++;

    if (poolPtr->enforceThreadSafety && pthread_mutex_trylock(&poolPtr->mutex) == 0)
    {
        while (poolPtr->unusedCount > poolPtr->maxPreallocated)
        {
            assert(! list_empty(&poolPtr->unused));

            nodePtr = poolPtr->unused.next;

            list_del(nodePtr);
            list_add_tail(nodePtr, &poolPtr->pending);
            poolPtr->unusedCount--;
        }

        poolPtr->unusedCount2 = poolPtr->unusedCount;

        pthread_mutex_unlock(&poolPtr->mutex);
    }
}

#ifdef WANT_LV2
#include "lv2/lv2_rtmempool.h"

void lv2_rtmempool_init(LV2_RtMemPool_Pool* poolPtr)
{
    poolPtr->create  = rtsafe_memory_pool_create;
    poolPtr->destroy = rtsafe_memory_pool_destroy;
    poolPtr->allocate_atomic = rtsafe_memory_pool_allocate_atomic;
    poolPtr->allocate_sleepy = rtsafe_memory_pool_allocate_sleepy;
    poolPtr->deallocate = rtsafe_memory_pool_deallocate;
}
#endif
