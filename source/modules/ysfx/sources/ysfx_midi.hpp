// Copyright 2021 Jean Pierre Cimalando
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once
#include "ysfx.h"
#include <vector>
#include <memory>

struct ysfx_midi_header_t {
    uint32_t bus;
    uint32_t offset;
    uint32_t size;
};

struct ysfx_midi_buffer_t {
    std::vector<uint8_t> data;
    size_t read_pos = 0;
    size_t read_pos_for_bus[ysfx_max_midi_buses] = {};
    bool extensible = false;
};
using ysfx_midi_buffer_u = std::unique_ptr<ysfx_midi_buffer_t>;

enum {
    ysfx_midi_message_max_size = 1 << 24,
};

// NOTE: regarding buses,
//    The buffer keeps 2 kinds of read positions: global, and per-bus.
//
//    These are tracked separately, so use either global/per-bus reading API,
//    but not both mixed in the same piece of code.
//
//    The JSFX API `midi*` implementations should always use per-bus access:
//    if `ext_midi_bus` is true, use the bus defined by `midi_bus`, otherwise 0.

void ysfx_midi_reserve(ysfx_midi_buffer_t *midi, uint32_t capacity, bool extensible);
void ysfx_midi_clear(ysfx_midi_buffer_t *midi);
bool ysfx_midi_push(ysfx_midi_buffer_t *midi, const ysfx_midi_event_t *event);
void ysfx_midi_rewind(ysfx_midi_buffer_t *midi);
bool ysfx_midi_get_next(ysfx_midi_buffer_t *midi, ysfx_midi_event_t *event);
bool ysfx_midi_get_next_from_bus(ysfx_midi_buffer_t *midi, uint32_t bus, ysfx_midi_event_t *event);

// incremental writer into a midi buffer
struct ysfx_midi_push_t {
    ysfx_midi_buffer_t *midi = nullptr;
    size_t start = 0;
    uint32_t count = 0;
    bool eob = false;
};
bool ysfx_midi_push_begin(ysfx_midi_buffer_t *midi, uint32_t bus, uint32_t offset, ysfx_midi_push_t *mp);
bool ysfx_midi_push_data(ysfx_midi_push_t *mp, const uint8_t *data, uint32_t size);
bool ysfx_midi_push_end(ysfx_midi_push_t *mp);

//------------------------------------------------------------------------------

// determine the length of a midi message according to its status byte
// if length is dynamic, returns 0
uint32_t ysfx_midi_sizeof(uint8_t id);
