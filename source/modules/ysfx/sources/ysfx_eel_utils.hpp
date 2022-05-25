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
#include "WDL/eel2/ns-eel.h"
#include "WDL/eel2/ns-eel-int.h"
#include <type_traits>
#include <cstdint>

template <class T>
inline typename std::enable_if<std::is_integral<T>::value, T>::type
ysfx_eel_round(EEL_F value)
{
    return (T)(value + (EEL_F)0.0001); // same one as used in eel2 everywhere
}

//------------------------------------------------------------------------------
class ysfx_eel_ram_reader {
public:
    ysfx_eel_ram_reader() = default;
    ysfx_eel_ram_reader(NSEEL_VMCTX vm, int64_t addr);
    EEL_F read_next();

private:
    NSEEL_VMCTX m_vm{};
    int64_t m_addr = 0;
    const EEL_F *m_block = nullptr;
    uint32_t m_block_avail = 0;
};

//------------------------------------------------------------------------------
class ysfx_eel_ram_writer {
public:
    ysfx_eel_ram_writer() = default;
    ysfx_eel_ram_writer(NSEEL_VMCTX vm, int64_t addr);
    bool write_next(EEL_F value);

private:
    NSEEL_VMCTX m_vm{};
    int64_t m_addr = 0;
    EEL_F *m_block = nullptr;
    uint32_t m_block_avail = 0;
};
