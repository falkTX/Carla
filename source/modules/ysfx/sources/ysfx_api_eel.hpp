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
#include <string>

typedef void *NSEEL_VMCTX;
class WDL_FastString;

//------------------------------------------------------------------------------
void ysfx_api_init_eel();

//------------------------------------------------------------------------------
void ysfx_eel_string_initvm(NSEEL_VMCTX vm);

//------------------------------------------------------------------------------
class eel_string_context_state;
eel_string_context_state *ysfx_eel_string_context_new();
void ysfx_eel_string_context_free(eel_string_context_state *state);
void ysfx_eel_string_context_update_named_vars(eel_string_context_state *state, NSEEL_VMCTX vm);
YSFX_DEFINE_AUTO_PTR(eel_string_context_state_u, eel_string_context_state, ysfx_eel_string_context_free);

//------------------------------------------------------------------------------
enum { ysfx_string_max_length = 1 << 16 };
bool ysfx_string_access(ysfx_t *fx, ysfx_real id, bool for_write, void (*access)(void *, WDL_FastString &), void *userdata);
bool ysfx_string_get(ysfx_t *fx, ysfx_real id, std::string &txt);
bool ysfx_string_set(ysfx_t *fx, ysfx_real id, const std::string &txt);
void ysfx_string_lock(ysfx_t *fx);
void ysfx_string_unlock(ysfx_t *fx);
const char *ysfx_string_access_unlocked(ysfx_t *fx, ysfx_real id, WDL_FastString **fs, bool for_write);

struct ysfx_string_scoped_lock {
    ysfx_string_scoped_lock(ysfx_t *fx) : m_fx(fx) { ysfx_string_lock(fx); }
    ~ysfx_string_scoped_lock() { ysfx_string_unlock(m_fx); }
private:
    ysfx_t *m_fx = nullptr;
};

#define EEL_STRING_GET_CONTEXT_POINTER(opaque) (((ysfx_t *)(opaque))->string_ctx.get())
#define EEL_STRING_GET_FOR_INDEX(x, wr) (ysfx_string_access_unlocked((ysfx_t *)(opaque), x, wr, false))
#define EEL_STRING_GET_FOR_WRITE(x, wr) (ysfx_string_access_unlocked((ysfx_t *)(opaque), x, wr, true))
#define EEL_STRING_MUTEXLOCK_SCOPE ysfx_string_scoped_lock lock{(ysfx_t *)(opaque)};
