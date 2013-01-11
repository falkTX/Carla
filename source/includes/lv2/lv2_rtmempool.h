/*
  LV2 realtime safe memory pool extension definition
  This work is in public domain.

  This file is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  If you have questions, contact Filipe Coelho (aka falkTX) <falktx@falktx.com>
  or ask in #lad channel, FreeNode IRC network.
*/

/**
 * @file lv2_rtmempool.h
 * C header for the LV2 rtmempool extension <http://kxstudio.sf.net/ns/lv2ext/rtmempool>
 *
 */

#ifndef LV2_RTMEMPOOL_H
#define LV2_RTMEMPOOL_H

#define LV2_RTSAFE_MEMORY_POOL_URI    "http://kxstudio.sf.net/ns/lv2ext/rtmempool"
#define LV2_RTSAFE_MEMORY_POOL_PREFIX LV2_RTSAFE_MEMORY_POOL_URI "#"

#define LV2_RTSAFE_MEMORY_POOL__Pool  LV2_RTSAFE_MEMORY_POOL_URI "Pool"

/** max size of memory pool name, in chars, including terminating zero char */
#define LV2_RTSAFE_MEMORY_POOL_NAME_MAX 128

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

/**
 * Opaque data to host data for LV2_RtMemPool_Pool.
 */
typedef void* LV2_RtMemPool_Handle;

/**
 * On instantiation, host must supply LV2_RTSAFE_MEMORY_POOL__Pool feature.
 * LV2_Feature::data must be pointer to LV2_RtMemPool_Pool.
 */
typedef struct _LV2_RtMemPool_Pool {
  /**
   * This function is called when plugin wants to create memory pool
   *
   * <b>may/will sleep</b>
   *
   * @param pool_name pool name, for debug purposes, max RTSAFE_MEMORY_POOL_NAME_MAX chars, including terminating zero char. May be NULL.
   * @param data_size memory chunk size
   * @param min_preallocated min chunks preallocated
   * @param max_preallocated max chunks preallocated
   *
   * @return Success status, true if successful
   */
  bool (*create)(LV2_RtMemPool_Handle * handle_ptr,
                 const char * pool_name,
                 size_t data_size,
                 size_t min_preallocated,
                 size_t max_preallocated);

  /**
   * This function is called when plugin wants to destroy previously created memory pool
   *
   * <b>may/will sleep</b>
   */
  void (*destroy)(LV2_RtMemPool_Handle handle);

  /**
   * This function is called when plugin wants to allocate memory in context where sleeping is not allowed
   *
   * <b>will not sleep</b>
   *
   * @return Pointer to allocated memory or NULL if memory no memory is available
   */
  void * (*allocate_atomic)(LV2_RtMemPool_Handle handle);

  /**
   * This function is called when plugin wants to allocate memory in context where sleeping is allowed
   *
   * <b>may/will sleep</b>
   *
   * @return Pointer to allocated memory or NULL if memory no memory is available (should not happen under normal conditions)
   */
  void * (*allocate_sleepy)(LV2_RtMemPool_Handle handle);

  /**
   * This function is called when plugin wants to deallocate previously allocated memory
   *
   * <b>will not sleep</b>
   *
   * @param memory_ptr pointer to previously allocated memory chunk
   */
  void (*deallocate)(LV2_RtMemPool_Handle handle,
                     void * memory_ptr);

} LV2_RtMemPool_Pool;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV2_RTMEMPOOL_H */
