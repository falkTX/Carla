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
#include "ysfx_utils.hpp"
#include "WDL/eel2/ns-eel.h"
#include "WDL/eel2/ns-eel-int.h"
#include <vector>
#include <memory>

struct ysfx_file_t {
    virtual ~ysfx_file_t() {}

    virtual int32_t avail() = 0;
    virtual void rewind() = 0;
    virtual bool var(ysfx_real *var) = 0;
    virtual uint32_t mem(uint32_t offset, uint32_t length) = 0;
    virtual uint32_t string(std::string &str) = 0;
    virtual bool riff(uint32_t &nch, ysfx_real &samplerate) = 0;
    virtual bool is_text() = 0;
    virtual bool is_in_write_mode() = 0;

    std::unique_ptr<ysfx::mutex> m_mutex{new ysfx::mutex};
};

using ysfx_file_u = std::unique_ptr<ysfx_file_t>;

//------------------------------------------------------------------------------

struct ysfx_raw_file_t final : ysfx_file_t {
    ysfx_raw_file_t(NSEEL_VMCTX vm, const char *filename);

    int32_t avail() override;
    void rewind() override;
    bool var(ysfx_real *var) override;
    uint32_t mem(uint32_t offset, uint32_t length) override;
    uint32_t string(std::string &str) override;
    bool riff(uint32_t &, ysfx_real &) override { return false; }
    bool is_text() override { return false; }
    bool is_in_write_mode() override { return false; }

    NSEEL_VMCTX m_vm = nullptr;
    ysfx::FILE_u m_stream;
};

//------------------------------------------------------------------------------

struct ysfx_text_file_t final : ysfx_file_t {
    ysfx_text_file_t(NSEEL_VMCTX vm, const char *filename);

    int32_t avail() override;
    void rewind() override;
    bool var(ysfx_real *var) override;
    uint32_t mem(uint32_t offset, uint32_t length) override;
    uint32_t string(std::string &str) override;
    bool riff(uint32_t &, ysfx_real &) override { return false; }
    bool is_text() override { return true; }
    bool is_in_write_mode() override { return false; }

    NSEEL_VMCTX m_vm = nullptr;
    ysfx::FILE_u m_stream;
    std::string m_buf;
};

//------------------------------------------------------------------------------

struct ysfx_audio_file_t final : ysfx_file_t {
    ysfx_audio_file_t(NSEEL_VMCTX vm, const ysfx_audio_format_t &fmt, const char *filename);

    int32_t avail() override;
    void rewind() override;
    bool var(ysfx_real *var) override;
    uint32_t mem(uint32_t offset, uint32_t length) override;
    uint32_t string(std::string &str) override;
    bool riff(uint32_t &nch, ysfx_real &samplerate) override;
    bool is_text() override { return false; }
    bool is_in_write_mode() override { return false; }

    NSEEL_VMCTX m_vm = nullptr;
    ysfx_audio_format_t m_fmt{};
    std::unique_ptr<ysfx_audio_reader_t, void (*)(ysfx_audio_reader_t *)> m_reader;
    enum { buffer_size = 256 };
    std::unique_ptr<ysfx_real[]> m_buf{new ysfx_real[buffer_size]};
};

//------------------------------------------------------------------------------

struct ysfx_serializer_t final : ysfx_file_t {
    explicit ysfx_serializer_t(NSEEL_VMCTX vm);

    void begin(bool write, std::string &buffer);
    void end();

    int32_t avail() override;
    void rewind() override;
    bool var(ysfx_real *var) override;
    uint32_t mem(uint32_t offset, uint32_t length) override;
    uint32_t string(std::string &str) override;
    bool riff(uint32_t &, ysfx_real &) override { return false; }
    bool is_text() override { return false; }
    bool is_in_write_mode() override { return m_write == 1; }

    NSEEL_VMCTX m_vm{};
    int m_write = -1;
    std::string *m_buffer = nullptr;
    size_t m_pos = 0;
};

using ysfx_serializer_u = std::unique_ptr<ysfx_serializer_t>;

//------------------------------------------------------------------------------
void ysfx_api_init_file();
