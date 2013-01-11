/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 *   This file is part of zynjacku
 *
 *   Copyright (C) 2006,2007,2008,2009 Nedko Arnaudov <nedko@arnaudov.name>
 *   Copyright (C) 2012 Filipe Coelho <falktx@falktx.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *****************************************************************************/

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>              /* sprintf */
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

#include "rtmempool.h"

#include "list.h"
//#define LOG_LEVEL LOG_LEVEL_DEBUG
#include "log.h"

struct rtsafe_memory_pool
{
  char name[LV2_RTSAFE_MEMORY_POOL_NAME_MAX];
  size_t data_size;
  size_t min_preallocated;
  size_t max_preallocated;

  unsigned int used_count;
  struct list_head unused;
  struct list_head used;
  unsigned int unused_count;

  bool enforce_thread_safety;
  /* next members are initialized/used only if enforce_thread_safety is true */
  pthread_mutex_t mutex;
  unsigned int unused_count2;
  struct list_head pending;

  size_t used_size;
};

static
void
rtsafe_memory_pool_sleepy(
  LV2_RtMemPool_Handle pool_handle);

static
bool
rtsafe_memory_pool_create(
  LV2_RtMemPool_Handle * pool_handle_ptr,
  const char * pool_name,
  size_t data_size,
  size_t min_preallocated,
  size_t max_preallocated,
  bool enforce_thread_safety)
{
  int ret;
  struct rtsafe_memory_pool * pool_ptr;

  assert(min_preallocated <= max_preallocated);

  assert(pool_name == NULL || strlen(pool_name) < LV2_RTSAFE_MEMORY_POOL_NAME_MAX);

  LOG_DEBUG(
    "creating pool \"%s\" (size %u, min = %u, max = %u, enforce = %s)",
    pool_name,
    (unsigned int)data_size,
    (unsigned int)min_preallocated,
    (unsigned int)max_preallocated,
    enforce_thread_safety ? "true" : "false");

  pool_ptr = malloc(sizeof(struct rtsafe_memory_pool));
  if (pool_ptr == NULL)
  {
    return false;
  }

  if (pool_name != NULL)
  {
    strcpy(pool_ptr->name, pool_name);
  }
  else
  {
    sprintf(pool_ptr->name, "%p", pool_ptr);
  }

  pool_ptr->data_size = data_size;
  pool_ptr->min_preallocated = min_preallocated;
  pool_ptr->max_preallocated = max_preallocated;

  INIT_LIST_HEAD(&pool_ptr->used);
  pool_ptr->used_count = 0;

  INIT_LIST_HEAD(&pool_ptr->unused);
  pool_ptr->unused_count = 0;

  pool_ptr->enforce_thread_safety = enforce_thread_safety;
  if (enforce_thread_safety)
  {
    ret = pthread_mutex_init(&pool_ptr->mutex, NULL);
    if (ret != 0)
    {
      free(pool_ptr);
      return false;
    }

    INIT_LIST_HEAD(&pool_ptr->pending);
    pool_ptr->unused_count2 = 0;
  }

  pool_ptr->used_size = 0;

  rtsafe_memory_pool_sleepy((LV2_RtMemPool_Handle)pool_ptr);
  *pool_handle_ptr = (LV2_RtMemPool_Handle)pool_ptr;

  return true;
}

#define pool_ptr ((struct rtsafe_memory_pool *)pool_handle)

static
void
rtsafe_memory_pool_destroy(
  LV2_RtMemPool_Handle pool_handle)
{
  int ret;
  struct list_head * node_ptr;

  LOG_DEBUG("destroying pool \"%s\"", pool_ptr->name);

  /* caller should deallocate all chunks prior releasing pool itself */
  if (pool_ptr->used_count != 0)
  {
    LOG_WARNING("Deallocating non-empty pool \"%s\", leaking %u entries:", pool_ptr->name, pool_ptr->used_count);

    list_for_each(node_ptr, &pool_ptr->used)
    {
      LOG_WARNING("    %p", node_ptr + 1);
    }

    assert(0);
  }

  while (pool_ptr->unused_count != 0)
  {
    assert(!list_empty(&pool_ptr->unused));

    node_ptr = pool_ptr->unused.next;

    list_del(node_ptr);
    pool_ptr->unused_count--;

    free(node_ptr);
  }

  assert(list_empty(&pool_ptr->unused));

  if (pool_ptr->enforce_thread_safety)
  {
    while (!list_empty(&pool_ptr->pending))
    {
      node_ptr = pool_ptr->pending.next;

      list_del(node_ptr);

      free(node_ptr);
    }

    ret = pthread_mutex_destroy(&pool_ptr->mutex);
    assert(ret == 0);
  }

  free(pool_ptr);

  // unused variable
  (void)ret;
}

/* adjust unused list size */
static
void
rtsafe_memory_pool_sleepy(
  LV2_RtMemPool_Handle pool_handle)
{
  struct list_head * node_ptr;
  unsigned int count;

  LOG_DEBUG("pool \"%s\", sleepy", pool_ptr->name);

  if (pool_ptr->enforce_thread_safety)
  {
    pthread_mutex_lock(&pool_ptr->mutex);

    count = pool_ptr->unused_count2;

    assert(pool_ptr->min_preallocated < pool_ptr->max_preallocated);

    while (count < pool_ptr->min_preallocated)
    {
      node_ptr = malloc(sizeof(struct list_head) + pool_ptr->data_size);
      if (node_ptr == NULL)
      {
        LOG_DEBUG("malloc() failed (%u)", (unsigned int)pool_ptr->used_size);
        break;
      }

      list_add_tail(node_ptr, &pool_ptr->pending);

      count++;

      pool_ptr->used_size += pool_ptr->data_size;
    }

    while (count > pool_ptr->max_preallocated && !list_empty(&pool_ptr->pending))
    {
      node_ptr = pool_ptr->pending.next;

      list_del(node_ptr);

      free(node_ptr);

      count--;

      pool_ptr->used_size -= pool_ptr->data_size;
    }

    pthread_mutex_unlock(&pool_ptr->mutex);
  }
  else
  {
    while (pool_ptr->unused_count < pool_ptr->min_preallocated)
    {
      node_ptr = malloc(sizeof(struct list_head) + pool_ptr->data_size);
      if (node_ptr == NULL)
      {
        LOG_DEBUG("malloc() failed (%u)", (unsigned int)pool_ptr->used_size);
        return;
      }

      list_add_tail(node_ptr, &pool_ptr->unused);
      pool_ptr->unused_count++;
      pool_ptr->used_size += pool_ptr->data_size;
    }

    while (pool_ptr->unused_count > pool_ptr->max_preallocated)
    {
      assert(!list_empty(&pool_ptr->unused));

      node_ptr = pool_ptr->unused.next;

      list_del(node_ptr);
      pool_ptr->unused_count--;

      free(node_ptr);
      pool_ptr->used_size -= pool_ptr->data_size;
    }
  }
}

/* find entry in unused list, fail if it is empty */
static
void *
rtsafe_memory_pool_allocate_atomic(
  LV2_RtMemPool_Handle pool_handle)
{
  struct list_head * node_ptr;

  LOG_DEBUG("pool \"%s\", allocate (%u, %u)", pool_ptr->name, pool_ptr->used_count, pool_ptr->unused_count);

  if (list_empty(&pool_ptr->unused))
  {
    return NULL;
  }

  node_ptr = pool_ptr->unused.next;
  list_del(node_ptr);
  pool_ptr->unused_count--;
  pool_ptr->used_count++;
  list_add_tail(node_ptr, &pool_ptr->used);

  if (pool_ptr->enforce_thread_safety &&
      pthread_mutex_trylock(&pool_ptr->mutex) == 0)
  {
    while (pool_ptr->unused_count < pool_ptr->min_preallocated && !list_empty(&pool_ptr->pending))
    {
      node_ptr = pool_ptr->pending.next;

      list_del(node_ptr);
      list_add_tail(node_ptr, &pool_ptr->unused);
      pool_ptr->unused_count++;
    }

    pool_ptr->unused_count2 = pool_ptr->unused_count;

    pthread_mutex_unlock(&pool_ptr->mutex);
  }

  LOG_DEBUG("pool \"%s\", allocated %p (%u)", pool_ptr->name, node_ptr + 1, pool_ptr->used_count);
  return (node_ptr + 1);
}

/* move from used to unused list */
static
void
rtsafe_memory_pool_deallocate(
  LV2_RtMemPool_Handle pool_handle,
  void * data)
{
  struct list_head * node_ptr;

  LOG_DEBUG("pool \"%s\", deallocate %p (%u)", pool_ptr->name, (struct list_head *)data - 1, pool_ptr->used_count);

  list_del((struct list_head *)data - 1);
  list_add_tail((struct list_head *)data - 1, &pool_ptr->unused);
  pool_ptr->used_count--;
  pool_ptr->unused_count++;

  if (pool_ptr->enforce_thread_safety &&
      pthread_mutex_trylock(&pool_ptr->mutex) == 0)
  {
    while (pool_ptr->unused_count > pool_ptr->max_preallocated)
    {
      assert(!list_empty(&pool_ptr->unused));

      node_ptr = pool_ptr->unused.next;

      list_del(node_ptr);
      list_add_tail(node_ptr, &pool_ptr->pending);
      pool_ptr->unused_count--;
    }

    pool_ptr->unused_count2 = pool_ptr->unused_count;

    pthread_mutex_unlock(&pool_ptr->mutex);
  }
}

static
void *
rtsafe_memory_pool_allocate_sleepy(
  LV2_RtMemPool_Handle pool_handle)
{
  void * data;

  LOG_DEBUG("pool \"%s\", allocate sleepy", pool_ptr->name);

  do
  {
    rtsafe_memory_pool_sleepy(pool_handle);
    data = rtsafe_memory_pool_allocate_atomic(pool_handle);
  }
  while (data == NULL);

  return data;
}

#undef pool_ptr

static
bool
rtsafe_memory_pool_create2(
    LV2_RtMemPool_Handle * pool_handle_ptr,
    const char * pool_name,
    size_t data_size,
    size_t min_preallocated,
    size_t max_preallocated)
{
  return rtsafe_memory_pool_create(pool_handle_ptr, pool_name, data_size, min_preallocated, max_preallocated, false);
}

void
rtmempool_allocator_init(
  struct _LV2_RtMemPool_Pool * allocator_ptr)
{
  allocator_ptr->create  = rtsafe_memory_pool_create2;
  allocator_ptr->destroy = rtsafe_memory_pool_destroy;
  allocator_ptr->allocate_atomic = rtsafe_memory_pool_allocate_atomic;
  allocator_ptr->allocate_sleepy = rtsafe_memory_pool_allocate_sleepy;
  allocator_ptr->deallocate = rtsafe_memory_pool_deallocate;
}
