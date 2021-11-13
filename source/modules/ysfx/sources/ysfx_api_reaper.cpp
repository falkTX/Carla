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

#include "ysfx.hpp"
#include "ysfx_api_reaper.hpp"
#include "ysfx_api_eel.hpp"
#include "ysfx_eel_utils.hpp"
#include "ysfx_utils.hpp"
#include <cmath>
#include <cstring>
#include <cassert>

#include "WDL/wdlstring.h"

#define REAPER_GET_INTERFACE(opaque) ((opaque) ? (ysfx_t *)(opaque) : nullptr)

static EEL_F *NSEEL_CGEN_CALL ysfx_api_spl(void *opaque, EEL_F *n_)
{
    //NOTE: callable from @gfx thread

    ysfx_t *fx = REAPER_GET_INTERFACE(opaque);
    int32_t n = ysfx_eel_round<int32_t>(*n_);

    if (n < 0 || n >= ysfx_max_channels) {
        fx->var.ret_temp = 0;
        return &fx->var.ret_temp;
    }

    return fx->var.spl[(uint32_t)n];
}

static EEL_F *NSEEL_CGEN_CALL ysfx_api_slider(void *opaque, EEL_F *n_)
{
    //NOTE: callable from @gfx thread

    ysfx_t *fx = REAPER_GET_INTERFACE(opaque);
    int32_t n = ysfx_eel_round<int32_t>(*n_);

    if (n < 1 || n > ysfx_max_sliders) {
        fx->var.ret_temp = 0;
        return &fx->var.ret_temp;
    }

    n -= 1;
    return fx->var.slider[(uint32_t)n];
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_slider_next_chg(void *opaque, EEL_F *index_, EEL_F *val_)
{
    //TODO frame-accurate slider changes
    (void)opaque;
    (void)index_;
    (void)val_;
    return -1;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_slider_automate(void *opaque, EEL_F *mask_or_slider_)
{
    //NOTE: callable from @gfx thread

    ysfx_t *fx = REAPER_GET_INTERFACE(opaque);
    uint32_t slider = ysfx_get_slider_of_var(fx, mask_or_slider_);

    uint64_t mask;
    if (slider < ysfx_max_sliders)
        mask = (uint64_t)1 << slider;
    else
        mask = ysfx_eel_round<uint64_t>(std::fabs(*mask_or_slider_));

    fx->slider.automate_mask |= mask;
    fx->slider.change_mask |= mask;
    return 0;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_sliderchange(void *opaque, EEL_F *mask_or_slider_)
{
    //NOTE: callable from @gfx thread

    ysfx_t *fx = REAPER_GET_INTERFACE(opaque);
    uint32_t slider = ysfx_get_slider_of_var(fx, mask_or_slider_);

    uint64_t mask;
    if (slider < ysfx_max_sliders)
        mask = (uint64_t)1 << slider;
    else
        mask = ysfx_eel_round<uint64_t>(std::fabs(*mask_or_slider_));

    fx->slider.change_mask |= mask;
    return 0;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_slider_show(void *opaque, EEL_F *mask_or_slider_, EEL_F *value_)
{
    //NOTE: callable from @gfx thread

    ysfx_t *fx = REAPER_GET_INTERFACE(opaque);
    uint32_t slider = ysfx_get_slider_of_var(fx, mask_or_slider_);

    uint64_t mask;
    if (slider < ysfx_max_sliders)
        mask = (uint64_t)1 << slider;
    else
        mask = ysfx_eel_round<uint64_t>(std::fabs(*mask_or_slider_));

    if (*value_ >= (EEL_F)+0.5) {
        // show
        fx->slider.visible_mask |= mask;
    }
    else if (*value_ >= (EEL_F)-0.5) {
        // hide
        mask = ~mask;
        fx->slider.visible_mask &= mask;
    }
    else {
        // toggle
        mask = fx->slider.visible_mask.fetch_xor(mask) ^ mask;
    }

    return (EEL_F)mask;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_midisend(void *opaque, INT_PTR np, EEL_F **parms)
{
    if (ysfx_get_thread_id() != ysfx_thread_id_dsp)
        return 0;

    int32_t offset;
    uint8_t msg1;
    uint8_t msg2;
    uint8_t msg3;

    switch (np) {
    case 3:
    {
        offset = ysfx_eel_round<int32_t>(*parms[0]);
        msg1 = (uint8_t)ysfx_eel_round<int32_t>(*parms[1]);
        const uint32_t msg23 = ysfx_eel_round<int32_t>(*parms[2]);
        msg2 = (uint8_t)(msg23 & 0xff);
        msg3 = (uint8_t)(msg23 >> 8);
        break;
    }
    case 4:
        offset = ysfx_eel_round<int32_t>(*parms[0]);
        msg1 = (uint8_t)ysfx_eel_round<int32_t>(*parms[1]);
        msg2 = (uint8_t)ysfx_eel_round<int32_t>(*parms[2]);
        msg3 = (uint8_t)ysfx_eel_round<int32_t>(*parms[3]);
        break;
    default:
        assert(false);
        return 0;
    }

    if (offset < 0)
        offset = 0;

    // NOTE(jpc) correct the length of the message
    // in case it should be less than 3 bytes
    uint32_t length = ysfx_midi_sizeof(msg1);
    if (length == 0) // don't know what message this is
        length = 3;

    ysfx_t *fx = REAPER_GET_INTERFACE(opaque);
    ysfx_midi_event_t event;
    const uint8_t data[] = {msg1, msg2, msg3};
    event.bus = ysfx_current_midi_bus(fx);
    event.offset = (uint32_t)offset;
    event.size = length;
    event.data = data;
    if (!ysfx_midi_push(fx->midi.out.get(), &event))
        return 0;

    return msg1;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_midisend_buf(void *opaque, EEL_F *offset_, EEL_F *buf_, EEL_F *len_)
{
    if (ysfx_get_thread_id() != ysfx_thread_id_dsp)
        return 0;

    int32_t offset = ysfx_eel_round<int32_t>(*offset_);
    int32_t buf = ysfx_eel_round<int32_t>(*buf_);
    int32_t len = ysfx_eel_round<int32_t>(*len_);

    if (len <= 0)
        return 0;
    if (offset < 0)
        offset = 0;

    ysfx_t *fx = REAPER_GET_INTERFACE(opaque);

    ysfx_midi_push_t mp;
    if (!ysfx_midi_push_begin(fx->midi.out.get(), ysfx_current_midi_bus(fx), (uint32_t)offset, &mp))
        return 0;

    ysfx_eel_ram_reader reader{fx->vm.get(), buf};
    for (uint32_t i = 0; i < (uint32_t)len; ++i) {
        uint8_t byte = (uint8_t)ysfx_eel_round<int32_t>(reader.read_next());
        if (!ysfx_midi_push_data(&mp, &byte, 1))
            break;
    }

    if (!ysfx_midi_push_end(&mp))
        return 0;

    return len;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_midisend_str(void *opaque, EEL_F *offset_, EEL_F *str_)
{
    if (ysfx_get_thread_id() != ysfx_thread_id_dsp)
        return 0;

    int32_t offset = ysfx_eel_round<int32_t>(*offset_);

    if (offset < 0)
        offset = 0;

    ysfx_t *fx = REAPER_GET_INTERFACE(opaque);

    ///
    struct process_data {
        ysfx_t *fx = nullptr;
        uint32_t offset = 0;
        uint32_t result = 0;
    };

    process_data pdata;
    pdata.offset = (uint32_t)offset;
    pdata.fx = fx;

    auto process_str = [](void *userdata, WDL_FastString &str) {
        process_data *pdata = (process_data *)userdata;
        ysfx_t *fx = pdata->fx;
        ysfx_midi_event_t event;
        event.bus = ysfx_current_midi_bus(fx);
        event.offset = pdata->offset;
        event.size = (uint32_t)str.GetLength();
        event.data = (const uint8_t *)str.Get();
        pdata->result = ysfx_midi_push(fx->midi.out.get(), &event) ? event.size : 0;
    };

    ///
    if (!ysfx_string_access(fx, *str_, false, +process_str, &pdata))
        return 0;

    return pdata.result;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_midisyx(void *opaque, EEL_F *offset_, EEL_F *buf_, EEL_F *len_)
{
    if (ysfx_get_thread_id() != ysfx_thread_id_dsp)
        return 0;

    int32_t offset = ysfx_eel_round<int32_t>(*offset_);
    int32_t buf = ysfx_eel_round<int32_t>(*buf_);
    int32_t len = ysfx_eel_round<int32_t>(*len_);

    if (len <= 0)
        return 0;
    if (offset < 0)
        offset = 0;

    ysfx_t *fx = REAPER_GET_INTERFACE(opaque);

    ysfx_midi_push_t mp;
    if (!ysfx_midi_push_begin(fx->midi.out.get(), ysfx_current_midi_bus(fx), (uint32_t)offset, &mp))
        return 0;

    ysfx_eel_ram_reader reader{fx->vm.get(), buf};
    for (uint32_t i = 0; i < (uint32_t)len; ++i) {
        uint8_t byte = (uint8_t)ysfx_eel_round<int32_t>(reader.read_next());
        const uint8_t head = 0xf0;
        const uint8_t tail = 0xf7;
        if (i == 0 && byte != head) {
            if (!ysfx_midi_push_data(&mp, &head, 1))
                break;
        }
        if (!ysfx_midi_push_data(&mp, &byte, 1))
            break;
        if (i + 1 == (uint32_t)len && byte != tail) {
            if (!ysfx_midi_push_data(&mp, &tail, 1))
                break;
        }
    }

    if (!ysfx_midi_push_end(&mp))
        return 0;

    return len;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_midirecv(void *opaque, INT_PTR np, EEL_F **parms)
{
    if (ysfx_get_thread_id() != ysfx_thread_id_dsp)
        return 0;

    ysfx_t *fx = REAPER_GET_INTERFACE(opaque);
    uint32_t bus = ysfx_current_midi_bus(fx);

    ysfx_midi_event_t event;
    bool have_event = ysfx_midi_get_next_from_bus(fx->midi.in.get(), bus, &event);
    // pass through the sysex events
    while (have_event && event.size > 3) {
        ysfx_midi_push(fx->midi.out.get(), &event);
        have_event = ysfx_midi_get_next_from_bus(fx->midi.in.get(), bus, &event);
    }
    if (!have_event)
        return 0;

    uint8_t msg1 = 0;
    uint8_t msg2 = 0;
    uint8_t msg3 = 0;

    switch (event.size) {
        case 3: msg3 = event.data[2]; // fall through
        case 2: msg2 = event.data[1]; // fall through
        case 1: msg1 = event.data[0]; break;
    }

    *parms[0] = (EEL_F)event.offset;
    *parms[1] = (EEL_F)msg1;
    switch (np) {
    case 4:
        *parms[2] = (EEL_F)msg2;
        *parms[3] = (EEL_F)msg3;
        break;
    case 3:
        *parms[2] = (EEL_F)(msg2 + (msg3 << 8));
        break;
    default:
        assert(false);
        return 0;
    }

    return 1;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_midirecv_buf(void *opaque, EEL_F *offset_, EEL_F *buf_, EEL_F *recvlen_)
{
    if (ysfx_get_thread_id() != ysfx_thread_id_dsp)
        return 0;

    ysfx_t *fx = REAPER_GET_INTERFACE(opaque);
    NSEEL_VMCTX vm = fx->vm.get();

    int32_t buf = ysfx_eel_round<int32_t>(*buf_);
    int32_t recvlen = ysfx_eel_round<int32_t>(*recvlen_);

    if (recvlen < 0)
        recvlen = 0;

    uint32_t bus = ysfx_current_midi_bus(fx);

    ysfx_midi_event_t event;
    bool have_event = ysfx_midi_get_next_from_bus(fx->midi.in.get(), bus, &event);
    // pass through the events larger than the buffer
    while (have_event && event.size > (uint32_t)recvlen) {
        ysfx_midi_push(fx->midi.out.get(), &event);
        have_event = ysfx_midi_get_next_from_bus(fx->midi.in.get(), bus, &event);
    }
    if (!have_event)
        return 0;

    *offset_ = (EEL_F)event.offset;

    ysfx_eel_ram_writer writer{vm, buf};
    for (uint32_t i = 0; i < event.size; ++i)
        writer.write_next(event.data[i]);

    return event.size;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_midirecv_str(void *opaque, EEL_F *offset_, EEL_F *str_)
{
    if (ysfx_get_thread_id() != ysfx_thread_id_dsp)
        return 0;

    ysfx_t *fx = REAPER_GET_INTERFACE(opaque);

    uint32_t bus = ysfx_current_midi_bus(fx);

    ysfx_midi_event_t event;
    bool have_event = ysfx_midi_get_next_from_bus(fx->midi.in.get(), bus, &event);
    // pass through the events larger than the maximum string
    while (have_event && event.size > ysfx_string_max_length) {
        ysfx_midi_push(fx->midi.out.get(), &event);
        have_event = ysfx_midi_get_next_from_bus(fx->midi.in.get(), bus, &event);
    }
    if (!have_event)
        return 0;

    auto process_str = [](void *userdata, WDL_FastString &str) {
        ysfx_midi_event_t *event = (ysfx_midi_event_t *)userdata;
        str.SetRaw((const char *)event->data, (int32_t)event->size);
    };

    ///
    if (!ysfx_string_access(fx, *str_, true, +process_str, &event))
        return 0;

    *offset_ = (EEL_F)event.offset;
    return event.size;
}

//------------------------------------------------------------------------------
void ysfx_api_init_reaper()
{
    NSEEL_addfunc_retptr("spl", 1, NSEEL_PProc_THIS, &ysfx_api_spl);
    NSEEL_addfunc_retptr("slider", 1, NSEEL_PProc_THIS, &ysfx_api_slider);

    NSEEL_addfunc_retval("slider_next_chg", 2, NSEEL_PProc_THIS, &ysfx_api_slider_next_chg);
    NSEEL_addfunc_retval("slider_automate", 1, NSEEL_PProc_THIS, &ysfx_api_slider_automate);
    NSEEL_addfunc_retval("sliderchange", 1, NSEEL_PProc_THIS, &ysfx_api_sliderchange);
    NSEEL_addfunc_retval("slider_show", 2, NSEEL_PProc_THIS, &ysfx_api_slider_show);

    NSEEL_addfunc_exparms("midisend", 3, NSEEL_PProc_THIS, &ysfx_api_midisend);
    NSEEL_addfunc_exparms("midisend", 4, NSEEL_PProc_THIS, &ysfx_api_midisend);
    NSEEL_addfunc_retval("midisend_buf", 3, NSEEL_PProc_THIS, &ysfx_api_midisend_buf);
    NSEEL_addfunc_retval("midisend_str", 2, NSEEL_PProc_THIS, &ysfx_api_midisend_str);
    NSEEL_addfunc_exparms("midirecv", 3, NSEEL_PProc_THIS, &ysfx_api_midirecv);
    NSEEL_addfunc_exparms("midirecv", 4, NSEEL_PProc_THIS, &ysfx_api_midirecv);
    NSEEL_addfunc_retval("midirecv_buf", 3, NSEEL_PProc_THIS, &ysfx_api_midirecv_buf);
    NSEEL_addfunc_retval("midirecv_str", 2, NSEEL_PProc_THIS, &ysfx_api_midirecv_str);
    NSEEL_addfunc_retval("midisyx", 3, NSEEL_PProc_THIS, &ysfx_api_midisyx);
}
