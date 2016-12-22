/*
 * RealTime Memory Pool, heavily based on work by Nedko Arnaudov
 * Copyright (C) 2006-2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2013-2016 Filipe Coelho <falktx@falktx.com>
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

#include "list.h"
#include "rtmempool.h"
#include "rtmempool-lv2.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RTMEMPOOL_THREAD_SAFETY 0

#if RTMEMPOOL_THREAD_SAFETY
#define rtmempool_mutex_lock(m) pthread_mutex_lock(m)
#define rtmempool_mutex_unlock(m) pthread_mutex_unlock(m)
#else
#define rtmempool_mutex_lock(...)
#define rtmempool_mutex_unlock(...)
#endif

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

    pthread_mutex_t mutex;

} RtMemPool;

// ------------------------------------------------------------------------------------------------
// adjust unused list size

static void rtsafe_memory_pool_sleepy(RtMemPool* poolPtr, bool* overMaxOrMallocFailed)
{
    k_list_head* nodePtr;
    k_list_head unused;
    unsigned int unusedCount;

    INIT_LIST_HEAD(&unused);
    unusedCount = 0;

    while (poolPtr->unusedCount + unusedCount < poolPtr->minPreallocated)
    {
        if (poolPtr->usedCount + poolPtr->unusedCount + unusedCount >= poolPtr->maxPreallocated)
        {
            *overMaxOrMallocFailed = true;
            break;
        }

        nodePtr = malloc(sizeof(k_list_head) + poolPtr->dataSize);

        if (nodePtr == NULL)
        {
            *overMaxOrMallocFailed = true;
            break;
        }

        list_add_tail(nodePtr, &unused);
        ++unusedCount;
    }

    rtmempool_mutex_lock(&poolPtr->mutex);

    poolPtr->unusedCount += unusedCount;

    while (unusedCount != 0)
    {
        assert(! list_empty(&unused));

        nodePtr = unused.next;
        list_del(nodePtr);
        list_add_tail(nodePtr, &poolPtr->unused);
        --unusedCount;
    }

    rtmempool_mutex_unlock(&poolPtr->mutex);
}

// ------------------------------------------------------------------------------------------------

static bool rtsafe_memory_pool_create2(RtMemPool_Handle* handlePtr,
                                       const char* poolName,
                                       size_t dataSize,
                                       size_t minPreallocated,
                                       size_t maxPreallocated)
{
    assert(minPreallocated <= maxPreallocated);
    assert(poolName == NULL || strlen(poolName) < RTSAFE_MEMORY_POOL_NAME_MAX);

    k_list_head* nodePtr;
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

    pthread_mutexattr_t atts;
    pthread_mutexattr_init(&atts);
#ifdef __ARM_ARCH_7A__
    pthread_mutexattr_setprotocol(&atts, PTHREAD_PRIO_INHERIT);
#endif
    pthread_mutex_init(&poolPtr->mutex, &atts);
    pthread_mutexattr_destroy(&atts);

    while (poolPtr->unusedCount < poolPtr->minPreallocated)
    {
        nodePtr = malloc(sizeof(k_list_head) + poolPtr->dataSize);

        if (nodePtr == NULL)
        {
            break;
        }

        list_add_tail(nodePtr, &poolPtr->unused);
        poolPtr->unusedCount++;
    }

    *handlePtr = (RtMemPool_Handle)poolPtr;

    return true;
}

// ------------------------------------------------------------------------------------------------

static unsigned char rtsafe_memory_pool_create_old(const char* poolName,
                                                   size_t dataSize,
                                                   size_t minPreallocated,
                                                   size_t maxPreallocated,
                                                   RtMemPool_Handle* handlePtr)
{
    return rtsafe_memory_pool_create2(handlePtr, poolName, dataSize, minPreallocated, maxPreallocated);
}

// ------------------------------------------------------------------------------------------------

bool rtsafe_memory_pool_create(RtMemPool_Handle* handlePtr,
                               const char* poolName,
                               size_t dataSize,
                               size_t minPreallocated,
                               size_t maxPreallocated)
{
    return rtsafe_memory_pool_create2(handlePtr, poolName, dataSize, minPreallocated, maxPreallocated);
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

    pthread_mutex_destroy(&poolPtr->mutex);

    free(poolPtr);
}

// ------------------------------------------------------------------------------------------------
// find entry in unused list, fail if it is empty

void* rtsafe_memory_pool_allocate_atomic(RtMemPool_Handle handle)
{
    assert(handle);

    k_list_head* nodePtr;
    RtMemPool* poolPtr = (RtMemPool*)handle;

    rtmempool_mutex_lock(&poolPtr->mutex);

    if (list_empty(&poolPtr->unused))
    {
        rtmempool_mutex_unlock(&poolPtr->mutex);
        return NULL;
    }

    nodePtr = poolPtr->unused.next;
    list_del(nodePtr);

    poolPtr->unusedCount--;
    poolPtr->usedCount++;

    list_add_tail(nodePtr, &poolPtr->used);

    rtmempool_mutex_unlock(&poolPtr->mutex);

    return (nodePtr + 1);
}

// ------------------------------------------------------------------------------------------------

void* rtsafe_memory_pool_allocate_sleepy(RtMemPool_Handle handle)
{
    assert(handle);

    void* data;
    bool overMaxOrMallocFailed = false;
    RtMemPool* poolPtr = (RtMemPool*)handle;

    do {
        rtsafe_memory_pool_sleepy(poolPtr, &overMaxOrMallocFailed);
        data = rtsafe_memory_pool_allocate_atomic((RtMemPool_Handle)poolPtr);
    }
    while (data == NULL && ! overMaxOrMallocFailed);

    return data;
}

// ------------------------------------------------------------------------------------------------
// move from used to unused list

void rtsafe_memory_pool_deallocate(RtMemPool_Handle handle, void* memoryPtr)
{
    assert(handle);

    RtMemPool* poolPtr = (RtMemPool*)handle;

    rtmempool_mutex_lock(&poolPtr->mutex);

    list_del((k_list_head*)memoryPtr - 1);
    list_add_tail((k_list_head*)memoryPtr - 1, &poolPtr->unused);
    poolPtr->usedCount--;
    poolPtr->unusedCount++;

    rtmempool_mutex_unlock(&poolPtr->mutex);
}

// ------------------------------------------------------------------------------------------------
// LV2 stuff

void lv2_rtmempool_init(LV2_RtMemPool_Pool* poolPtr)
{
    poolPtr->create  = rtsafe_memory_pool_create;
    poolPtr->destroy = rtsafe_memory_pool_destroy;
    poolPtr->allocate_atomic = rtsafe_memory_pool_allocate_atomic;
    poolPtr->allocate_sleepy = rtsafe_memory_pool_allocate_sleepy;
    poolPtr->deallocate = rtsafe_memory_pool_deallocate;
}

void lv2_rtmempool_init_deprecated(LV2_RtMemPool_Pool_Deprecated* poolPtr)
{
    poolPtr->create  = rtsafe_memory_pool_create_old;
    poolPtr->destroy = rtsafe_memory_pool_destroy;
    poolPtr->allocate_atomic = rtsafe_memory_pool_allocate_atomic;
    poolPtr->allocate_sleepy = rtsafe_memory_pool_allocate_sleepy;
    poolPtr->deallocate = rtsafe_memory_pool_deallocate;
}
