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

#include "ysfx_midi.hpp"
#include <cstring>
#include <cassert>

void ysfx_midi_reserve(ysfx_midi_buffer_t *midi, uint32_t capacity, bool extensible)
{
    std::vector<uint8_t> data;
    data.reserve(capacity);
    std::swap(data, midi->data);
    midi->extensible = extensible;
    ysfx_midi_rewind(midi);
}

void ysfx_midi_clear(ysfx_midi_buffer_t *midi)
{
    midi->data.clear();
    ysfx_midi_rewind(midi);
}

bool ysfx_midi_push(ysfx_midi_buffer_t *midi, const ysfx_midi_event_t *event)
{
    if (event->size > ysfx_midi_message_max_size)
        return false;
    if (event->bus >= ysfx_max_midi_buses)
        return false;

    ysfx_midi_header_t header;

    if (!midi->extensible) {
        size_t writable = midi->data.capacity() - midi->data.size();
        if (writable < sizeof(header) + event->size)
            return false;
    }

    const uint8_t *data = event->data;
    const uint8_t *headp = (const uint8_t *)&header;
    header.bus = event->bus;
    header.offset = event->offset;
    header.size = event->size;

    midi->data.insert(midi->data.end(), headp, headp + sizeof(header));
    midi->data.insert(midi->data.end(), data, data + header.size);
    return true;
}

void ysfx_midi_rewind(ysfx_midi_buffer_t *midi)
{
    midi->read_pos = 0;
    for (uint32_t i = 0; i < ysfx_max_midi_buses; ++i)
        midi->read_pos_for_bus[i] = 0;
}

bool ysfx_midi_get_next(ysfx_midi_buffer_t *midi, ysfx_midi_event_t *event)
{
    size_t *pos_ptr = &midi->read_pos;
    size_t pos = *pos_ptr;

    size_t avail = midi->data.size() - pos;
    ysfx_midi_header_t header;
    if (avail == 0)
        return false;

    assert(avail >= sizeof(header));
    memcpy(&header, &midi->data[pos], sizeof(header));
    assert(avail >= sizeof(header) + header.size);

    event->bus = header.bus;
    event->offset = header.offset;
    event->size = header.size;
    event->data = &midi->data[pos + sizeof(header)];
    *pos_ptr = pos + (sizeof(header) + header.size);
    return true;
}

bool ysfx_midi_get_next_from_bus(ysfx_midi_buffer_t *midi, uint32_t bus, ysfx_midi_event_t *event)
{
    if (bus >= ysfx_max_midi_buses)
        return false;

    size_t *pos_ptr = &midi->read_pos_for_bus[bus];
    size_t pos = *pos_ptr;
    size_t avail = midi->data.size() - pos;
    ysfx_midi_header_t header;

    bool found = false;
    while (!found && avail > 0) {
        assert(avail >= sizeof(header));
        memcpy(&header, &midi->data[pos], sizeof(header));
        assert(avail >= sizeof(header) + header.size);

        found = header.bus == bus;
        if (!found) {
            pos += sizeof(header) + header.size;
            avail -= sizeof(header) + header.size;
        }
    }

    if (!found) {
        *pos_ptr = pos;
        return false;
    }

    event->bus = header.bus;
    event->offset = header.offset;
    event->size = header.size;
    event->data = &midi->data[pos + sizeof(header)];
    *pos_ptr = pos + (sizeof(header) + header.size);
    return true;
}

bool ysfx_midi_push_begin(ysfx_midi_buffer_t *midi, uint32_t bus, uint32_t offset, ysfx_midi_push_t *mp)
{
    ysfx_midi_header_t header;

    mp->midi = midi;
    mp->start = midi->data.size();
    mp->count = 0;
    mp->eob = false;

    if (!midi->extensible) {
        size_t writable = midi->data.capacity() - midi->data.size();
        if (writable < sizeof(header)) {
            mp->eob = true;
            return false;
        }
    }

    header.bus = bus;
    header.offset = offset;
    header.size = 0;

    const uint8_t *headp = (const uint8_t *)&header;
    midi->data.insert(midi->data.end(), headp, headp + sizeof(header));

    return true;
}

bool ysfx_midi_push_data(ysfx_midi_push_t *mp, const uint8_t *data, uint32_t size)
{
    if (mp->eob)
        return false;

    if (size > ysfx_midi_message_max_size || mp->count + size > ysfx_midi_message_max_size) {
        mp->eob = true;
        return false;
    }

    ysfx_midi_buffer_t *midi = mp->midi;

    if (!midi->extensible) {
        size_t writable = midi->data.capacity() - midi->data.size();
        if (writable < size) {
            mp->eob = true;
            return false;
        }
    }

    midi->data.insert(midi->data.end(), data, data + size);
    mp->count += size;
    return true;
}

bool ysfx_midi_push_end(ysfx_midi_push_t *mp)
{
    if (mp->eob) {
        mp->midi->data.resize(mp->start);
        return false;
    }

    ysfx_midi_header_t header;
    uint8_t *headp = &mp->midi->data[mp->start];
    memcpy(&header, headp, sizeof(header));
    header.size = mp->count;
    memcpy(headp, &header, sizeof(header));
    return true;
}

//------------------------------------------------------------------------------
uint32_t ysfx_midi_sizeof(uint8_t id)
{
    if ((id >> 7) == 0) {
        return 0;
    }
    else if ((id >> 4) != 0b1111) {
        static const uint8_t sizetable[8] = {
            3, 3, 3, 3, 2, 2, 3,
        };
        return sizetable[(id >> 4) & 0b111];
    }
    else {
        static const uint8_t sizetable[16] = {
            0, 2, 3, 2, 1, 1, 1, 0,
            1, 1, 1, 1, 1, 1, 1, 1,
        };
        return sizetable[id & 0b1111];
    }
}
