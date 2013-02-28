/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
  Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
  The DSP code is based on ladspa:1970 by Fons Adriaensen

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

/* if NDEBUG is defined, assert checks are disabled */
//#define NDEBUG

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "filter.h"

static float exp2ap(float x)
{
    int i;

    i = (int)(floor(x));
    x -= i;
//    return ldexp(1 + x * (0.66 + 0.34 * x), i);
    return ldexp(1 + x * (0.6930 + x * (0.2416 + x * (0.0517 + x * 0.0137))), i);
}

struct param_sect
{
  float f, b, g;
  float s1, s2, a;
  float z1, z2;
};

void
param_sect_init(
  struct param_sect * sect_ptr)
{
  sect_ptr->f = 0.25f;
  sect_ptr->b = sect_ptr->g = 1.0f;
  sect_ptr->a = sect_ptr->s1 = sect_ptr->s2 = sect_ptr->z1 = sect_ptr->z2 = 0.0f;
}

void
param_sect_proc(
  struct param_sect * sect_ptr,
  int k,
  float * sig,
  float f,
  float b,
  float g)
{
  float s1, s2, d1, d2, a, da, x, y;
  bool  u2 = false;

  s1 = sect_ptr->s1;
  s2 = sect_ptr->s2;
  a = sect_ptr->a;
  d1 = 0;
  d2 = 0;
  da = 0;

  if (f != sect_ptr->f)
  {
    if      (f < 0.5f * sect_ptr->f) f = 0.5f * sect_ptr->f;
    else if (f > 2.0f * sect_ptr->f) f = 2.0f * sect_ptr->f;
    sect_ptr->f = f;
    sect_ptr->s1 = -cosf(6.283185f * f);
    d1 = (sect_ptr->s1 - s1) / k;
    u2 = true;
  }

  if (g != sect_ptr->g)
  {
    if      (g < 0.5f * sect_ptr->g) g = 0.5f * sect_ptr->g;
    else if (g > 2.0f * sect_ptr->g) g = 2.0f * sect_ptr->g;
    sect_ptr->g = g;
    sect_ptr->a = 0.5f * (g - 1.0f);
    da = (sect_ptr->a - a) / k;
    u2 = true;
  }

  if (b != sect_ptr->b)
  {
    if      (b < 0.5f * sect_ptr->b) b = 0.5f * sect_ptr->b;
    else if (b > 2.0f * sect_ptr->b) b = 2.0f * sect_ptr->b;
    sect_ptr->b = b;
    u2 = true;
  }

  if (u2)
  {
    b *= 7 * f / sqrtf(g);
    sect_ptr->s2 = (1 - b) / (1 + b);
    d2 = (sect_ptr->s2 - s2) / k;
  }

  while (k--)
  {
    s1 += d1;
    s2 += d2;
    a += da;
    x = *sig;
    y = x - s2 * sect_ptr->z2;
    *sig++ -= a * (sect_ptr->z2 + s2 * y - x);
    y -= s1 * sect_ptr->z1;
    sect_ptr->z2 = sect_ptr->z1 + s1 * y;
    sect_ptr->z1 = y + 1e-10f;
  }
}

struct filter
{
  float sample_rate;

  const float * global_parameters[GLOBAL_PARAMETERS_COUNT];

  unsigned int bands_count;
  const float ** band_parameters; /* [band_index * BAND_PARAMETERS_COUNT + parameter_index] */

  float gain;
  int fade;
  struct param_sect * sect;     /* [band_index] */
};

bool
filter_create(
  float sample_rate,
  unsigned int bands_count,
  filter_handle * handle_ptr)
{
  struct filter * filter_ptr;
  unsigned int j;

  assert(bands_count > 0);

  filter_ptr = calloc(1, sizeof(struct filter));
  if (filter_ptr == NULL)
  {
    goto fail;
  }

  filter_ptr->band_parameters = calloc(bands_count, sizeof(float *) * BAND_PARAMETERS_COUNT);
  if (filter_ptr->band_parameters == NULL)
  {
    goto free_filter;
  }

  filter_ptr->sect = malloc(sizeof(struct param_sect) * bands_count);
  if (filter_ptr->sect == NULL)
  {
    goto free_band_params;
  }

  filter_ptr->sample_rate = sample_rate;
  filter_ptr->bands_count = bands_count;
  filter_ptr->fade = 0;
  filter_ptr->gain = 1.0;

  for (j = 0; j < bands_count; j++)
  {
    param_sect_init(filter_ptr->sect + j);
  }

  *handle_ptr = (filter_handle)filter_ptr;

  return true;

free_band_params:
  free(filter_ptr->band_parameters);

free_filter:
  free(filter_ptr);

fail:
  return false;
}

#define filter_ptr ((struct filter *)handle)

void
filter_destroy(
  filter_handle handle)
{
  free(filter_ptr->sect);
  free(filter_ptr->band_parameters);
  free(filter_ptr);
}

void
filter_connect_global_parameter(
  filter_handle handle,
  unsigned int global_parameter,
  const float * value_ptr)
{
  assert(global_parameter < GLOBAL_PARAMETERS_COUNT);

  filter_ptr->global_parameters[global_parameter] = value_ptr;
}

void
filter_connect_band_parameter(
  filter_handle handle,
  unsigned int band_index,
  unsigned int band_parameter,
  const float * value_ptr)
{
  assert(band_index < filter_ptr->bands_count);
  assert(band_parameter < BAND_PARAMETERS_COUNT);

  filter_ptr->band_parameters[band_index * BAND_PARAMETERS_COUNT + band_parameter] = value_ptr;
}

void
filter_run(
  filter_handle handle,
  const float * input_buffer,
  float * output_buffer,
  unsigned long samples_count)
{
  int i, j, k;
  const float * p;
  float sig[48];
  float t, g, d;
  float fgain;
  float sfreq[filter_ptr->bands_count];
  float sband[filter_ptr->bands_count];
  float sgain[filter_ptr->bands_count];
  float bands_count;

  bands_count = filter_ptr->bands_count;

  fgain = exp2ap(0.1661 * *filter_ptr->global_parameters[GLOBAL_PARAMETER_GAIN]);

  for (j = 0; j < bands_count; j++)
  {
    t = *filter_ptr->band_parameters[BAND_PARAMETERS_COUNT * j + BAND_PARAMETER_FREQUENCY] / filter_ptr->sample_rate;
    if (t < 0.0002)
    {
      t = 0.0002;
    }
    else if (t > 0.4998)
    {
      t = 0.4998;
    }
    sfreq[j] = t;

    sband[j] = *filter_ptr->band_parameters[BAND_PARAMETERS_COUNT * j + BAND_PARAMETER_BANDWIDTH];

    if (*filter_ptr->band_parameters[BAND_PARAMETERS_COUNT * j + BAND_PARAMETER_ACTIVE] > 0.0)
    {
      sgain[j] = exp2ap(0.1661 * *filter_ptr->band_parameters[BAND_PARAMETERS_COUNT * j + BAND_PARAMETER_GAIN]);
    }
    else
    {
      sgain[j] = 1.0;
    }
  }

  while (samples_count)
  {
    k = (samples_count > 48) ? 32 : samples_count;

    t = fgain;
    g = filter_ptr->gain;

    if (t > 1.25 * g)
    {
      t = 1.25 * g;
    }
    else if (t < 0.80 * g)
    {
      t = 0.80 * g;
    }

    filter_ptr->gain = t;
    d = (t - g) / k;
    for (i = 0; i < k; i++)
    {
      g += d;
      sig[i] = g * input_buffer[i];
    }

    for (j = 0; j < bands_count; j++)
    {
      param_sect_proc(filter_ptr->sect + j, k, sig, sfreq[j], sband[j], sgain[j]);
    }

    j = filter_ptr->fade;
    g = j / 16.0;
    p = 0;

    if (*filter_ptr->global_parameters[GLOBAL_PARAMETER_ACTIVE] > 0.0)
    {
      if (j == 16)
      {
        p = sig;
      }
      else
      {
        ++j;
      }
    }
    else
    {
      if (j == 0)
      {
        p = input_buffer;
      }
      else
      {
        --j;
      }
    }

    filter_ptr->fade = j;

    if (p)
    {
      memcpy(output_buffer, p, k * sizeof(float));
    }
    else
    {
      d = (j / 16.0 - g) / k;
      for (i = 0; i < k; i++)
      {
        g += d;
        output_buffer[i] = g * sig[i] + (1 - g) * input_buffer[i];
      }
    }

    input_buffer += k;
    output_buffer += k;
    samples_count -= k;
  }
}
