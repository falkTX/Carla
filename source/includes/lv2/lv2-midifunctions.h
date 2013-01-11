/****************************************************************************
    
    lv2-midifunctions.h - support file for using MIDI in LV2 plugins
    
    Copyright (C) 2006  Lars Luthman <lars.luthman@gmail.com>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307  USA

****************************************************************************/

#ifndef LV2_MIDIFUNCTIONS
#define LV2_MIDIFUNCTIONS

#include "lv2-miditype.h"


typedef struct {
  LV2_MIDI* midi;
  uint32_t frame_count;
  uint32_t position;
} LV2_MIDIState;


inline double lv2midi_get_event(LV2_MIDIState* state,
                                double* timestamp, 
                                uint32_t* size, 
                                unsigned char** data) {
  
  if (state->position >= state->midi->size) {
    state->position = state->midi->size;
    *timestamp = state->frame_count;
    *size = 0;
    *data = NULL;
    return *timestamp;
  }
  
  *timestamp = *(double*)(state->midi->data + state->position);
  *size = *(size_t*)(state->midi->data + state->position + sizeof(double));
  *data = state->midi->data + state->position + 
    sizeof(double) + sizeof(size_t);
  return *timestamp;
}


inline double lv2midi_step(LV2_MIDIState* state) {

  if (state->position >= state->midi->size) {
    state->position = state->midi->size;
    return state->frame_count;
  }
  
  state->position += sizeof(double);
  size_t size = *(size_t*)(state->midi->data + state->position);
  state->position += sizeof(size_t);
  state->position += size;
  return *(double*)(state->midi->data + state->position);
}


inline void lv2midi_put_event(LV2_MIDIState* state,
                             double timestamp,
                             uint32_t size,
                             const unsigned char* data) {
  
  if (state->midi->size + sizeof(double) + sizeof(size_t) + size < state->midi->capacity)
  {
    *((double*)(state->midi->data + state->midi->size)) = timestamp;
    state->midi->size += sizeof(double);
    *((size_t*)(state->midi->data + state->midi->size)) = size;
    state->midi->size += sizeof(size_t);
    memcpy(state->midi->data + state->midi->size, data, size);

    state->midi->size += size;
    state->midi->event_count++;
  }
}

#endif

