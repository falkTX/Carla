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

#ifndef RTMEMPOOL_H__1FA54215_11CF_4659_9CF3_C17A10A67A1F__INCLUDED
#define RTMEMPOOL_H__1FA54215_11CF_4659_9CF3_C17A10A67A1F__INCLUDED

#include "lv2/lv2_rtmempool.h"

#ifdef __cplusplus
extern "C" {
#endif

void
rtmempool_allocator_init(
  struct _LV2_RtMemPool_Pool * allocator_ptr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* #ifndef RTMEMPOOL_H__1FA54215_11CF_4659_9CF3_C17A10A67A1F__INCLUDED */
