/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 *   Copyright (C) 2006,2007,2008,2009 Nedko Arnaudov <nedko@arnaudov.name>
 *   Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include "CarlaNative.h"

//#include "nekofilter.h"
#include "filter.h"
#define LOG_LEVEL LOG_LEVEL_ERROR
#include "log.h"

#include "ui.c"

#define BANDS_COUNT 4

struct nekofilter
{
  filter_handle filter;
  float params_global[GLOBAL_PARAMETERS_COUNT];
  float params_bands[BAND_PARAMETERS_COUNT*BANDS_COUNT];
  HostDescriptor* host;
  struct control* ui;
};

PluginHandle
nekofilter_instantiate(
  const struct _PluginDescriptor* _this_,
  HostDescriptor* host)
{
  struct nekofilter * nekofilter_ptr;
  unsigned int i;

  LOG_DEBUG("nekofilter_create_plugin_instance() called.");

  nekofilter_ptr = malloc(sizeof(struct nekofilter));
  if (nekofilter_ptr == NULL)
  {
    goto fail;
  }

  nekofilter_ptr->host = host;
  nekofilter_ptr->ui   = NULL;

  if (!filter_create(host->get_sample_rate(host->handle), BANDS_COUNT, &nekofilter_ptr->filter))
  {
    goto fail_destroy_filter;
  }

  nekofilter_ptr->params_global[GLOBAL_PARAMETER_ACTIVE] = 0.0f;
  nekofilter_ptr->params_global[GLOBAL_PARAMETER_GAIN]   = 0.0f;

  filter_connect_global_parameter(nekofilter_ptr->filter,
                                  GLOBAL_PARAMETER_ACTIVE,
                                  &nekofilter_ptr->params_global[GLOBAL_PARAMETER_ACTIVE]);

  filter_connect_global_parameter(nekofilter_ptr->filter,
                                  GLOBAL_PARAMETER_GAIN,
                                  &nekofilter_ptr->params_global[GLOBAL_PARAMETER_GAIN]);

  for (i=0; i < BANDS_COUNT; i++)
  {
    nekofilter_ptr->params_bands[i*BAND_PARAMETERS_COUNT + BAND_PARAMETER_ACTIVE]    = 0.0f;
    nekofilter_ptr->params_bands[i*BAND_PARAMETERS_COUNT + BAND_PARAMETER_FREQUENCY] = 0.0f;
    nekofilter_ptr->params_bands[i*BAND_PARAMETERS_COUNT + BAND_PARAMETER_BANDWIDTH] = 1.0f;
    nekofilter_ptr->params_bands[i*BAND_PARAMETERS_COUNT + BAND_PARAMETER_GAIN]      = 0.0f;

    switch (i)
    {
    case 0:
      nekofilter_ptr->params_bands[i*BAND_PARAMETERS_COUNT + BAND_PARAMETER_FREQUENCY] = 200.0f;
      break;
    case 1:
      nekofilter_ptr->params_bands[i*BAND_PARAMETERS_COUNT + BAND_PARAMETER_FREQUENCY] = 400.0f;
      break;
    case 2:
      nekofilter_ptr->params_bands[i*BAND_PARAMETERS_COUNT + BAND_PARAMETER_FREQUENCY] = 1000.0f;
      break;
    case 3:
      nekofilter_ptr->params_bands[i*BAND_PARAMETERS_COUNT + BAND_PARAMETER_FREQUENCY] = 2000.0f;
      break;
    }

    filter_connect_band_parameter(nekofilter_ptr->filter,
                                  i,
                                  BAND_PARAMETER_ACTIVE,
                                  &nekofilter_ptr->params_bands[i*BAND_PARAMETERS_COUNT + BAND_PARAMETER_ACTIVE]);

    filter_connect_band_parameter(nekofilter_ptr->filter,
                                  i,
                                  BAND_PARAMETER_FREQUENCY,
                                  &nekofilter_ptr->params_bands[i*BAND_PARAMETERS_COUNT + BAND_PARAMETER_FREQUENCY]);

    filter_connect_band_parameter(nekofilter_ptr->filter,
                                  i,
                                  BAND_PARAMETER_BANDWIDTH,
                                  &nekofilter_ptr->params_bands[i*BAND_PARAMETERS_COUNT + BAND_PARAMETER_BANDWIDTH]);

    filter_connect_band_parameter(nekofilter_ptr->filter,
                                  i,
                                  BAND_PARAMETER_GAIN,
                                  &nekofilter_ptr->params_bands[i*BAND_PARAMETERS_COUNT + BAND_PARAMETER_GAIN]);
  }

  return (PluginHandle)nekofilter_ptr;

fail_destroy_filter:
  filter_destroy(nekofilter_ptr->filter);
  free(nekofilter_ptr);

fail:
  return NULL;

  // unused
  (void)_this_;
}

#define nekofilter_ptr ((struct nekofilter *)handle)

uint32_t
nekofilter_get_parameter_count(
  PluginHandle handle)
{
  return GLOBAL_PARAMETERS_COUNT + BAND_PARAMETERS_COUNT*BANDS_COUNT;

  // unused
  (void)handle;
}

const Parameter*
nekofilter_get_parameter_info(
  PluginHandle handle,
  uint32_t index)
{
  static Parameter param;
  static bool first_init = true;
  uint32_t band;
  char strBuf[32];

  if (first_init)
  {
    first_init = false;
    param.name = NULL;
    param.unit = NULL;
  }
  else
  {
    if (param.name != NULL)
    {
      free((void*)param.name);
      param.name = NULL;
    }
    if (param.unit != NULL)
    {
      free((void*)param.unit);
      param.unit = NULL;
    }
  }

  if (handle == NULL && index == 0xf00baa)
    // internal cleanup call
    return NULL;

  param.hints = PARAMETER_IS_ENABLED|PARAMETER_IS_AUTOMABLE;
  param.ranges.def = 0.0f;
  param.ranges.min = 0.0f;
  param.ranges.max = 0.0f;
  param.scalePointCount = 0;
  param.scalePoints     = NULL;

  switch (index)
  {
  case GLOBAL_PARAMETER_ACTIVE:
    param.hints |= PARAMETER_IS_BOOLEAN;
    param.name = strdup("Active");
    param.ranges.max = 1.0f;
    goto ready;
    break;

  case GLOBAL_PARAMETER_GAIN:
    param.name = strdup("Gain");
    param.unit = strdup("dB");
    param.ranges.min = -20.0f;
    param.ranges.max = 20.0f;
    goto ready;
    break;
  }

  index -= GLOBAL_PARAMETERS_COUNT;

  band   = index / BANDS_COUNT;
  index %= BANDS_COUNT;

  sprintf(strBuf, "%i:", band);

  switch (index)
  {
  case BAND_PARAMETER_ACTIVE:
    strcat(strBuf, "Active");
    param.hints |= PARAMETER_IS_BOOLEAN;
    param.name = strdup(strBuf);
    param.ranges.max = 1.0f;
    break;

  case BAND_PARAMETER_FREQUENCY:
    strcat(strBuf, "Frequency");
    param.hints |= PARAMETER_IS_LOGARITHMIC;
    param.name = strdup(strBuf);
    param.unit = strdup("Hz");

    switch (band)
    {
    case 0:
      param.ranges.min = 20.0f;
      param.ranges.max = 2000.0f;
      break;
    case 1:
      param.ranges.min = 40.0f;
      param.ranges.max = 4000.0f;
      break;
    case 2:
      param.ranges.min = 100.0f;
      param.ranges.max = 10000.0f;
      break;
    case 3:
      param.ranges.min = 200.0f;
      param.ranges.max = 20000.0f;
      break;
    }
    break;

  case BAND_PARAMETER_BANDWIDTH:
    strcat(strBuf, "Bandwidth");
    param.hints |= PARAMETER_IS_LOGARITHMIC;
    param.name = strdup(strBuf);
    param.ranges.min = 0.125f;
    param.ranges.max = 8.0f;
    break;

  case BAND_PARAMETER_GAIN:
    strcat(strBuf, "Gain");
    param.name = strdup(strBuf);
    param.unit = strdup("dB");
    param.ranges.min = -20.0f;
    param.ranges.max = 20.0f;
    break;
  }

ready:
  if (param.hints & PARAMETER_IS_BOOLEAN)
  {
    param.ranges.step = 1.0f;
    param.ranges.stepSmall = 1.0f;
    param.ranges.stepLarge = 1.0f;
  }
  else
  {
    float range = param.ranges.max - param.ranges.min;
    param.ranges.step = range/100.0f;
    param.ranges.stepSmall = range/1000.0f;
    param.ranges.stepLarge = range/10.0f;
  }

  return &param;
}

float
nekofilter_get_parameter_value(
  PluginHandle handle,
  uint32_t index)
{
  if (index < GLOBAL_PARAMETERS_COUNT)
  {
    return nekofilter_ptr->params_global[index];
  }
  else
  {
    assert(index >= GLOBAL_PARAMETERS_COUNT);
    index -= GLOBAL_PARAMETERS_COUNT;

    return nekofilter_ptr->params_bands[index];
  }
}

void
nekofilter_set_parameter_value(
  PluginHandle handle,
  uint32_t index,
  float value)
{
  if (index < GLOBAL_PARAMETERS_COUNT)
  {
    nekofilter_ptr->params_global[index] = value;
  }
  else
  {
    assert(index >= GLOBAL_PARAMETERS_COUNT);
    index -= GLOBAL_PARAMETERS_COUNT;

    nekofilter_ptr->params_bands[index] = value;
  }
}

void
nekofilter_process(
  PluginHandle handle,
  float** inBuffer,
  float** outBuffer,
  uint32_t frames,
  uint32_t midiEventCount,
  const MidiEvent* midiEvents)
{
  LOG_DEBUG("nekofilter_run");
  filter_run(
    nekofilter_ptr->filter,
    inBuffer[0],
    outBuffer[0],
    frames);

  return;

  // unused
  (void)midiEventCount;
  (void)midiEvents;
}

void nekofilter_ui_show(
  PluginHandle handle,
  bool show)
{
  if (show)
  {
    if (nekofilter_ptr->ui == NULL)
      nekofilter_ptr->ui = nekoui_instantiate(nekofilter_ptr->host);
    nekoui_show(nekofilter_ptr->ui);
  }
  else if (nekofilter_ptr->ui != NULL)
    nekoui_hide(nekofilter_ptr->ui);
}

void nekofilter_ui_idle(
  PluginHandle handle)
{
  if (nekofilter_ptr->ui != NULL)
    nekoui_run(nekofilter_ptr->ui);
}

void nekofilter_ui_set_parameter_value(
  PluginHandle handle,
  uint32_t index,
  float value)
{
  if (nekofilter_ptr->ui != NULL)
    nekoui_set_parameter_value(nekofilter_ptr->ui, index, value);
}

void
nekofilter_cleanup(
  PluginHandle handle)
{
  if (nekofilter_ptr->ui != NULL)
  {
    nekoui_quit(nekofilter_ptr->ui);
    nekoui_cleanup(nekofilter_ptr->ui);
  }

  filter_destroy(nekofilter_ptr->filter);
  free(nekofilter_ptr);

  // cleanup static data
  nekofilter_get_parameter_info(NULL, 0xf00baa);
}

#undef nekofilter_ptr
