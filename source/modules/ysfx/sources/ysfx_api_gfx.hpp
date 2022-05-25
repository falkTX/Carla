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

#if !defined(YSFX_NO_GFX)
struct ysfx_gfx_state_t;
ysfx_gfx_state_t *ysfx_gfx_state_new(ysfx_t *fx);
void ysfx_gfx_state_free(ysfx_gfx_state_t *state);
YSFX_DEFINE_AUTO_PTR(ysfx_gfx_state_u, ysfx_gfx_state_t, ysfx_gfx_state_free);
void ysfx_gfx_state_set_bitmap(ysfx_gfx_state_t *state, uint8_t *data, uint32_t w, uint32_t h, uint32_t stride);
void ysfx_gfx_state_set_scale_factor(ysfx_gfx_state_t *state, ysfx_real scale);
void ysfx_gfx_state_set_callback_data(ysfx_gfx_state_t *state, void *callback_data);
void ysfx_gfx_state_set_show_menu_callback(ysfx_gfx_state_t *state, int (*callback)(void *, const char *, int32_t, int32_t));
void ysfx_gfx_state_set_set_cursor_callback(ysfx_gfx_state_t *state, void (*callback)(void *, int32_t));
void ysfx_gfx_state_set_get_drop_file_callback(ysfx_gfx_state_t *state, const char *(*callback)(void *, int32_t));
bool ysfx_gfx_state_is_dirty(ysfx_gfx_state_t *state);
void ysfx_gfx_state_add_key(ysfx_gfx_state_t *state, uint32_t mods, uint32_t key, bool press);
void ysfx_gfx_state_update_mouse(ysfx_gfx_state_t *state, uint32_t mods, int xpos, int ypos, uint32_t buttons, int wheel, int hwheel);

//------------------------------------------------------------------------------
void ysfx_gfx_enter(ysfx_t *fx, bool doinit);
void ysfx_gfx_leave(ysfx_t *fx);
ysfx_gfx_state_t *ysfx_gfx_get_context(ysfx_t *fx);
void ysfx_gfx_prepare(ysfx_t *fx);

struct ysfx_scoped_gfx_t {
    ysfx_scoped_gfx_t(ysfx_t *fx, bool doinit) : m_fx(fx) { ysfx_gfx_enter(fx, doinit); }
    ~ysfx_scoped_gfx_t() { ysfx_gfx_leave(m_fx); }
    ysfx_t *m_fx = nullptr;
};
#endif

//------------------------------------------------------------------------------
void ysfx_api_init_gfx();
