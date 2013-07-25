/*
 * RealTime Memory Pool, heavily based on work by Nedko Arnaudov
 * Copyright (C) 2006-2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#ifndef __RTMEMPOOL_H__
#define __RTMEMPOOL_H__

#ifdef __cplusplus
# include <cstddef>
#else
# include <stdbool.h>
# include <stddef.h>
#endif

/** max size of memory pool name, in chars, including terminating zero char */
#define RTSAFE_MEMORY_POOL_NAME_MAX 128

/**
 * Opaque data for RtMemPool_Pool.
 */
typedef void* RtMemPool_Handle;

/**
 * Create new memory pool
 *
 * <b>may/will sleep</b>
 *
 * @param poolName pool name, for debug purposes, max RTSAFE_MEMORY_POOL_NAME_MAX chars, including terminating zero char. May be NULL.
 * @param dataSize memory chunk size
 * @param minPreallocated min chunks preallocated
 * @param maxPreallocated max chunks preallocated
 *
 * @return Success status, true if successful
 */
bool rtsafe_memory_pool_create(RtMemPool_Handle* handlePtr,
                               const char* poolName,
                               size_t dataSize,
                               size_t minPreallocated,
                               size_t maxPreallocated);

/**
 * Create new memory pool, thread-safe version
 *
 * <b>may/will sleep</b>
 *
 * @param poolName pool name, for debug purposes, max RTSAFE_MEMORY_POOL_NAME_MAX chars, including terminating zero char. May be NULL.
 * @param dataSize memory chunk size
 * @param minPreallocated min chunks preallocated
 * @param maxPreallocated max chunks preallocated
 *
 * @return Success status, true if successful
 */
bool rtsafe_memory_pool_create_safe(RtMemPool_Handle* handlePtr,
                                    const char* poolName,
                                    size_t dataSize,
                                    size_t minPreallocated,
                                    size_t maxPreallocated);

/**
 * Destroy previously created memory pool
 *
 * <b>may/will sleep</b>
 */
void rtsafe_memory_pool_destroy(RtMemPool_Handle handle);

/**
 * Allocate memory in context where sleeping is not allowed
 *
 * <b>will not sleep</b>
 *
 * @return Pointer to allocated memory or NULL if memory no memory is available
 */
void* rtsafe_memory_pool_allocate_atomic(RtMemPool_Handle handle);

/**
 * Allocate memory in context where sleeping is allowed
 *
 * <b>may/will sleep</b>
 *
 * @return Pointer to allocated memory or NULL if memory no memory is available (should not happen under normal conditions)
 */
void* rtsafe_memory_pool_allocate_sleepy(RtMemPool_Handle handle);

/**
 * Deallocate previously allocated memory
 *
 * <b>will not sleep</b>
 *
 * @param memoryPtr pointer to previously allocated memory chunk
 */
void rtsafe_memory_pool_deallocate(RtMemPool_Handle handle,
                                   void* memoryPtr);

#endif // __RTMEMPOOL_H__
