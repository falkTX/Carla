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

#include "ysfx_eel_utils.hpp"

ysfx_eel_ram_reader::ysfx_eel_ram_reader(NSEEL_VMCTX vm, int64_t addr)
    : m_vm(vm),
      m_addr(addr)
{
}

EEL_F ysfx_eel_ram_reader::read_next()
{
    if (m_block_avail == 0) {
        m_block = (m_addr < 0 || m_addr > 0xFFFFFFFFu) ? nullptr :
            NSEEL_VM_getramptr_noalloc(m_vm, (uint32_t)m_addr, (int32_t *)&m_block_avail);
        if (m_block)
            m_addr += m_block_avail;
        else {
            m_addr += 1;
            m_block_avail = 1;
        }
    }
    EEL_F value = m_block ? *m_block++ : 0;
    m_block_avail -= 1;
    return value;
}

//------------------------------------------------------------------------------
ysfx_eel_ram_writer::ysfx_eel_ram_writer(NSEEL_VMCTX vm, int64_t addr)
    : m_vm(vm),
      m_addr(addr)
{
}

bool ysfx_eel_ram_writer::write_next(EEL_F value)
{
    if (m_block_avail == 0) {
        m_block = (m_addr < 0 || m_addr > 0xFFFFFFFFu) ? nullptr :
            NSEEL_VM_getramptr(m_vm, (uint32_t)m_addr, (int32_t *)&m_block_avail);
        if (m_block)
            m_addr += m_block_avail;
        else {
            m_addr += 1;
            m_block_avail = 1;
        }
    }
    if (m_block)
        *m_block++ = value;
    m_block_avail -= 1;
    return true;
}
