/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
  Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef FILTER_H__D5DC5ADF_211A_48F6_93A5_68CD3B73D6C5__INCLUDED
#define FILTER_H__D5DC5ADF_211A_48F6_93A5_68CD3B73D6C5__INCLUDED

typedef struct {int unused; } * filter_handle;

#define GLOBAL_PARAMETER_ACTIVE    0
#define GLOBAL_PARAMETER_GAIN      1
#define GLOBAL_PARAMETERS_COUNT    2

#define BAND_PARAMETER_ACTIVE      0
#define BAND_PARAMETER_FREQUENCY   1
#define BAND_PARAMETER_BANDWIDTH   2
#define BAND_PARAMETER_GAIN        3
#define BAND_PARAMETERS_COUNT      4

bool
filter_create(
  float sample_rate,
  unsigned int bands_count,
  filter_handle * handle_ptr);

void
filter_connect_global_parameter(
  filter_handle handle,
  unsigned int global_parameter,
  const float * value_ptr);

void
filter_connect_band_parameter(
  filter_handle handle,
  unsigned int band_index,
  unsigned int band_parameter,
  const float * value_ptr);

void
filter_run(
  filter_handle handle,
  const float * input_buffer,
  float * output_buffer,
  unsigned long samples_count);

void
filter_destroy(
  filter_handle handle);

#endif /* #ifndef FILTER_H__D5DC5ADF_211A_48F6_93A5_68CD3B73D6C5__INCLUDED */
