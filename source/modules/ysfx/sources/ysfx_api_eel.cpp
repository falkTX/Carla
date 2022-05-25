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
#include "ysfx_api_eel.hpp"
#include "ysfx_utils.hpp"
#include <cstring>
#include <cstdlib>
#include <cstddef>

#include "WDL/ptrlist.h"
#include "WDL/assocarray.h"
#include "WDL/mutex.h"

#ifndef EELSCRIPT_NO_STDIO
#   define EEL_STRING_STDOUT_WRITE(x,len) { fwrite(x,len,1,stdout); fflush(stdout); }
#endif

//TODO: thread-safety considerations with strings

#define EEL_STRING_MAXUSERSTRING_LENGTH_HINT ysfx_string_max_length

static ysfx::mutex atomic_mutex;
#define EEL_ATOMIC_SET_SCOPE(opaque) ysfx::mutex *mutex = ((opaque) ? &((ysfx_t *)(opaque))->atomic_mutex : &atomic_mutex);
#define EEL_ATOMIC_ENTER mutex->lock()
#define EEL_ATOMIC_LEAVE mutex->unlock()

#include "WDL/eel2/eel_strings.h"
#include "WDL/eel2/eel_misc.h"
#include "WDL/eel2/eel_fft.h"
#include "WDL/eel2/eel_mdct.h"
#include "WDL/eel2/eel_atomic.h"

//------------------------------------------------------------------------------
void ysfx_api_init_eel()
{
    EEL_string_register();
    EEL_fft_register();
    EEL_mdct_register();
    EEL_string_register();
    EEL_misc_register();
    EEL_atomic_register();
}

//------------------------------------------------------------------------------
void ysfx_eel_string_initvm(NSEEL_VMCTX vm)
{
    eel_string_initvm(vm);
}

//------------------------------------------------------------------------------
eel_string_context_state *ysfx_eel_string_context_new()
{
    return new eel_string_context_state;
}

void ysfx_eel_string_context_free(eel_string_context_state *state)
{
    delete state;
}

void ysfx_eel_string_context_update_named_vars(eel_string_context_state *state, NSEEL_VMCTX vm)
{
    state->update_named_vars(vm);
}

//------------------------------------------------------------------------------
static_assert(
    ysfx_string_max_length == EEL_STRING_MAXUSERSTRING_LENGTH_HINT,
    "string max lengths do not match");

bool ysfx_string_access(ysfx_t *fx, ysfx_real id, bool for_write, void (*access)(void *, WDL_FastString &), void *userdata)
{
    void *opaque = fx;
    eel_string_context_state *ctx = EEL_STRING_GET_CONTEXT_POINTER(opaque);
    EEL_STRING_MUTEXLOCK_SCOPE

    EEL_STRING_STORAGECLASS *wr = nullptr;
    ctx->GetStringForIndex(id, &wr, for_write);
    if (!wr)
        return false;

    access(userdata, *wr);
    return true;
}

bool ysfx_string_get(ysfx_t *fx, ysfx_real id, std::string &txt)
{
    return ysfx_string_access(fx, id, false, [](void *ud, WDL_FastString &str) {
        ((std::string *)ud)->assign(str.Get(), (uint32_t)str.GetLength());
    }, &txt);
}

bool ysfx_string_set(ysfx_t *fx, ysfx_real id, const std::string &txt)
{
    return ysfx_string_access(fx, id, true, [](void *ud, WDL_FastString &str) {
        const std::string *txt = (const std::string *)ud;
        size_t size = txt->size();
        if (size > ysfx_string_max_length)
            size = ysfx_string_max_length;
        str.SetRaw(txt->data(), (int)size);
    }, (void *)&txt);
}

void ysfx_string_lock(ysfx_t *fx)
{
    fx->string_mutex.lock();
}

void ysfx_string_unlock(ysfx_t *fx)
{
    fx->string_mutex.unlock();
}

const char *ysfx_string_access_unlocked(ysfx_t *fx, ysfx_real id, WDL_FastString **fs, bool for_write)
{
    return fx->string_ctx->GetStringForIndex(id, fs, for_write);
}

//------------------------------------------------------------------------------
// NOTE(jpc) implement this? I guess probably not.
//     DSP and UI should not mutex each other.

void NSEEL_HOSTSTUB_EnterMutex()
{
}

void NSEEL_HOSTSTUB_LeaveMutex()
{
}
