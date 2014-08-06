/*
 * RealTime Memory Pool, heavily based on work by Nedko Arnaudov
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef __RTMEMPOOL_LV2_H__
#define __RTMEMPOOL_LV2_H__

#include "rtmempool.h"
#include "lv2/lv2_rtmempool.h"

/**
 * Initialize LV2_RTSAFE_MEMORY_POOL__Pool feature
 *
 * @param poolPtr host allocated pointer to LV2_RtMemPool_Pool
 */
void lv2_rtmempool_init(LV2_RtMemPool_Pool* poolPtr);

void lv2_rtmempool_init_deprecated(LV2_RtMemPool_Pool_Deprecated* poolPtr);

#endif // __RTMEMPOOL_LV2_H__
