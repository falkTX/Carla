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
#include <type_traits>
#include <atomic>
#include <cstdint>

namespace ysfx {

//------------------------------------------------------------------------------
// sync_bitset64: A lock-free synchronized bitset of size 64
//
// This is implemented by a single qword on 64-bit machine, otherwise a pair of
// dwords on 32-bit machine, which is to ensure lock-freedom.
//
// This bitset is synchronized but not atomic; a thread might see a
// partially updated set after a modification. Conceptually, it can be seen as
// atomic on individual bit flips (that is, masking operations do not race).

class sync_bitset64_single;
class sync_bitset64_dual;

// could use std::atomic::is_always_lock_free on C++17 and up
using sync_bitset64 = std::conditional<
    sizeof(intptr_t) <= 4, sync_bitset64_dual, sync_bitset64_single>::type;

//------------------------------------------------------------------------------
class sync_bitset64_single {
public:
    uint64_t load() const
    {
        return bits_.load(std::memory_order_relaxed);
    }

    void store(uint64_t value)
    {
        bits_.store(value, std::memory_order_relaxed);
    }

    uint64_t exchange(uint64_t value)
    {
        return bits_.exchange(value, std::memory_order_relaxed);
    }

    uint64_t fetch_or(uint64_t value)
    {
        return bits_.fetch_or(value, std::memory_order_relaxed);
    }

    uint64_t fetch_and(uint64_t value)
    {
        return bits_.fetch_and(value, std::memory_order_relaxed);
    }

    uint64_t fetch_xor(uint64_t value)
    {
        return bits_.fetch_xor(value, std::memory_order_relaxed);
    }

    void operator|=(uint64_t value)
    {
        fetch_or(value);
    }

    void operator&=(uint64_t value)
    {
        fetch_and(value);
    }

    void operator^=(uint64_t value)
    {
        fetch_xor(value);
    }

private:
    std::atomic<uint64_t> bits_{0};
};

//------------------------------------------------------------------------------
class sync_bitset64_dual {
public:
    uint64_t load() const
    {
        return join(lobits_.load(std::memory_order_relaxed),
                    hibits_.load(std::memory_order_relaxed));
    }

    void store(uint64_t value)
    {
        lobits_.store(lo(value), std::memory_order_relaxed);
        hibits_.store(hi(value), std::memory_order_relaxed);
    }

    uint64_t exchange(uint64_t value)
    {
        return join(lobits_.exchange(lo(value), std::memory_order_relaxed),
                    hibits_.exchange(hi(value), std::memory_order_relaxed));
    }

    uint64_t fetch_or(uint64_t value)
    {
        return join(lobits_.fetch_or(lo(value), std::memory_order_relaxed),
                    hibits_.fetch_or(hi(value), std::memory_order_relaxed));
    }

    uint64_t fetch_and(uint64_t value)
    {
        return join(lobits_.fetch_and(lo(value), std::memory_order_relaxed),
                    hibits_.fetch_and(hi(value), std::memory_order_relaxed));
    }

    uint64_t fetch_xor(uint64_t value)
    {
        return join(lobits_.fetch_xor(lo(value), std::memory_order_relaxed),
                    hibits_.fetch_xor(hi(value), std::memory_order_relaxed));
    }

    void operator|=(uint64_t value)
    {
        fetch_or(value);
    }

    void operator&=(uint64_t value)
    {
        fetch_and(value);
    }

    void operator^=(uint64_t value)
    {
        fetch_xor(value);
    }

private:
    static constexpr uint64_t join(uint32_t lo, uint32_t hi)
    {
        return (uint64_t)lo | ((uint64_t)hi << 32);
    }

    static constexpr uint32_t lo(uint64_t value)
    {
        return (uint32_t)(value & 0xFFFFFFFFu);
    }

    static constexpr uint32_t hi(uint64_t value)
    {
        return (uint32_t)(value >> 32);
    }

private:
    std::atomic<uint32_t> lobits_{0};
    std::atomic<uint32_t> hibits_{0};
};

} // namespace ysfx
