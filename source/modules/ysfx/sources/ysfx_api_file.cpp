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
#include "ysfx_config.hpp"
#include "ysfx_api_file.hpp"
#include "ysfx_eel_utils.hpp"
#include <cstring>
#include <cstdio>
#include <cassert>

ysfx_raw_file_t::ysfx_raw_file_t(NSEEL_VMCTX vm, const char *filename)
    : m_vm(vm),
      m_stream(ysfx::fopen_utf8(filename, "rb"))
{
}

int32_t ysfx_raw_file_t::avail()
{
    if (!m_stream)
        return 0;

    int64_t cur_off = ysfx::ftell_lfs(m_stream.get());
    if (cur_off == -1)
        return 0;

    if (ysfx::fseek_lfs(m_stream.get(), 0, SEEK_END) == -1)
        return 0;

    int64_t end_off = ysfx::ftell_lfs(m_stream.get());
    if (end_off == -1)
        return 0;

    if (ysfx::fseek_lfs(m_stream.get(), cur_off, SEEK_SET) == -1)
        return 0;

    if ((uint64_t)end_off < (uint64_t)cur_off)
        return 0;

    uint64_t byte_count = (uint64_t)end_off - (uint64_t)cur_off;
    uint64_t f32_count = byte_count / 4;
    return (f32_count > 0x7fffffff) ? 0x7fffffff : (uint32_t)f32_count;
}

void ysfx_raw_file_t::rewind()
{
    if (!m_stream)
        return;

    ::rewind(m_stream.get());
}

bool ysfx_raw_file_t::var(ysfx_real *var)
{
    if (!m_stream)
        return false;

    uint8_t data[4];
    if (fread(data, 1, 4, m_stream.get()) != 4)
        return false;

    *var = (EEL_F)ysfx::unpack_f32le(data);
    return true;
}

uint32_t ysfx_raw_file_t::mem(uint32_t offset, uint32_t length)
{
    if (!m_stream)
        return 0;

    ysfx_eel_ram_writer writer{m_vm, offset};

    uint32_t read;
    for (read = 0; read < length; ++read) {
        ysfx_real value;
        if (!var(&value))
            break;
        writer.write_next(value);
    }

    return read;
}

uint32_t ysfx_raw_file_t::string(std::string &str)
{
    if (!m_stream)
        return 0;

    uint8_t data[4];
    if (fread(data, 1, 4, m_stream.get()) != 4)
        return 0;

    str.clear();

    uint32_t srclen = ysfx::unpack_u32le(data);
    str.reserve((srclen < ysfx_string_max_length) ? srclen : ysfx_string_max_length);

    uint32_t count = 0;
    for (int byte; count < srclen && (byte = fgetc(m_stream.get())) != EOF; ) {
        if (str.size() < ysfx_string_max_length)
            str.push_back((unsigned char)byte);
        ++count;
    }
    return count;
}

//------------------------------------------------------------------------------
ysfx_text_file_t::ysfx_text_file_t(NSEEL_VMCTX vm, const char *filename)
    : m_vm(vm),
      m_stream(ysfx::fopen_utf8(filename, "rb"))
{
    m_buf.reserve(256);
}

int32_t ysfx_text_file_t::avail()
{
    if (!m_stream || ferror(m_stream.get()))
        return -1;

    return (feof(m_stream.get()) == 0) ? 0 : 1;
}

void ysfx_text_file_t::rewind()
{
    if (!m_stream)
        return;

    ::rewind(m_stream.get());
}

bool ysfx_text_file_t::var(ysfx_real *var)
{
    if (!m_stream)
        return false;

    //TODO support the expression language for arithmetic

    int ch;
    do {
        // get the next number separated by newline or comma
        // but skip invalid lines
        m_buf.clear();
        while ((ch = fgetc(m_stream.get())) != EOF && ch != '\n' && ch != ',')
            m_buf.push_back((unsigned char)ch);
        const char *startp = m_buf.c_str();
        const char *endp = (char *)startp;
        double value = ysfx::dot_strtod(startp, (char **)&endp);
        if (endp != startp) {
            *var = (EEL_F)value;
            return true;
        }
    } while (ch != EOF);

    return false;
}

uint32_t ysfx_text_file_t::mem(uint32_t offset, uint32_t length)
{
    if (!m_stream)
        return 0;

    ysfx_eel_ram_writer writer{m_vm, offset};

    uint32_t read;
    for (read = 0; read < length; ++read) {
        ysfx_real value;
        if (!var(&value))
            break;
        writer.write_next(value);
    }

    return read;
}

uint32_t ysfx_text_file_t::string(std::string &str)
{
    if (!m_stream)
        return 0;

    str.clear();
    str.reserve(256);

    int ch;
    do {
        ch = fgetc(m_stream.get());
        if (ch != EOF && str.size() < ysfx_string_max_length)
            str.push_back((unsigned char)ch);
    } while (ch != EOF && ch != '\n');

    return (uint32_t)str.size();
}

//------------------------------------------------------------------------------
ysfx_audio_file_t::ysfx_audio_file_t(NSEEL_VMCTX vm, const ysfx_audio_format_t &fmt, const char *filename)
    : m_vm(vm),
      m_fmt(fmt),
      m_reader(fmt.open(filename), fmt.close)
{
}

int32_t ysfx_audio_file_t::avail()
{
    if (!m_reader)
        return -1;

    uint64_t avail = m_fmt.avail(m_reader.get());
    return (avail > 0x7fffffff) ? 0x7fffffff : (int32_t)avail;
}

void ysfx_audio_file_t::rewind()
{
    if (!m_reader)
        return;

    m_fmt.rewind(m_reader.get());
}

bool ysfx_audio_file_t::var(ysfx_real *var)
{
    if (!m_reader)
        return false;

    return m_fmt.read(m_reader.get(), var, 1) == 1;
}

uint32_t ysfx_audio_file_t::mem(uint32_t offset, uint32_t length)
{
    if (!m_reader)
        return 0;

    uint32_t numread = 0;
    ysfx_real *buf = m_buf.get();
    ysfx_eel_ram_writer writer(m_vm, offset);

    while (numread < length) {
        uint32_t n = length - numread;
        if (n > buffer_size)
            n = buffer_size;

        uint32_t m = (uint32_t)m_fmt.read(m_reader.get(), buf, n);
        for (uint32_t i = 0; i < m; ++i)
            writer.write_next(buf[i]);

        numread += m;
        if (m < n)
            break;
    }

    return numread;
}

uint32_t ysfx_audio_file_t::string(std::string &str)
{
    (void)str;
    return 0;
}

bool ysfx_audio_file_t::riff(uint32_t &nch, ysfx_real &samplerate)
{
    if (!m_reader)
        return false;

    ysfx_audio_file_info_t info = m_fmt.info(m_reader.get());
    nch = info.channels;
    samplerate = info.sample_rate;
    return true;
}

//------------------------------------------------------------------------------
ysfx_serializer_t::ysfx_serializer_t(NSEEL_VMCTX vm)
    : m_vm(vm)
{
}

void ysfx_serializer_t::begin(bool write, std::string &buffer)
{
    m_write = (int)write;
    m_buffer = &buffer;
    m_pos = 0;
}

void ysfx_serializer_t::end()
{
    m_write = -1;
    m_buffer = nullptr;
}

int32_t ysfx_serializer_t::avail()
{
    if (m_write)
        return -1;
    else
        return 0;
}

void ysfx_serializer_t::rewind()
{
}

bool ysfx_serializer_t::var(ysfx_real *var)
{
    if (m_write == 1) {
        uint8_t buf[4];
        ysfx::pack_f32le((float)*var, buf);
        m_buffer->append((char *)buf, 4);
        return true;
    }
    else if (m_write == 0) {
        if (m_pos + 4 > m_buffer->size()) {
            m_pos = m_buffer->size();
            *var = 0;
            return false;
        }
        *var = (EEL_F)ysfx::unpack_f32le((uint8_t *)&(*m_buffer)[m_pos]);
        m_pos += 4;
        return true;
    }
    return false;
}

uint32_t ysfx_serializer_t::mem(uint32_t offset, uint32_t length)
{
    if (m_write == 1) {
        ysfx_eel_ram_reader reader{m_vm, offset};
        for (uint32_t i = 0; i < length; ++i) {
            ysfx_real value = reader.read_next();
            if (!var(&value))
                return i;
        }
        return length;
    }
    else if (m_write == 0) {
        ysfx_eel_ram_writer writer{m_vm, offset};
        for (uint32_t i = 0; i < length; ++i) {
            ysfx_real value{};
            if (!var(&value))
                return i;
            writer.write_next(value);
        }
        return length;
    }
    return 0;
}

uint32_t ysfx_serializer_t::string(std::string &str)
{
    // TODO implement me; docs claim support in Reaper 4.59+ but it seems
    //   non-working (as of Reaper 6.40)
    return 0;
}

//------------------------------------------------------------------------------
static EEL_F NSEEL_CGEN_CALL ysfx_api_file_open(void *opaque, EEL_F *file_)
{
    ysfx_t *fx = (ysfx_t *)opaque;

    std::string filepath;
    if (!ysfx_find_data_file(fx, file_, filepath))
        return -1;

    void *fmtobj = nullptr;
    ysfx_file_type_t ftype = ysfx_detect_file_type(fx, filepath.c_str(), &fmtobj);

    ysfx_file_u file;
    switch (ftype) {
    case ysfx_file_type_txt:
        file.reset(new ysfx_text_file_t(fx->vm.get(), filepath.c_str()));
        break;
    case ysfx_file_type_raw:
        file.reset(new ysfx_raw_file_t(fx->vm.get(), filepath.c_str()));
        break;
    case ysfx_file_type_audio:
        file.reset(new ysfx_audio_file_t(fx->vm.get(), *(ysfx_audio_format_t *)fmtobj, filepath.c_str()));
        break;
    case ysfx_file_type_none:
        break;
    default:
        assert(false);
    }

    if (file) {
        int32_t handle = ysfx_insert_file(fx, file.get());
        if (handle == -1)
            return -1;
        (void)file.release();
        return (EEL_F)(uint32_t)handle;
    }

    return -1;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_file_close(void *opaque, EEL_F *handle_)
{
    int32_t handle = ysfx_eel_round<int32_t>(*handle_);
    if (handle <= 0) //NOTE: cannot close the serializer handle (0)
        return -1;

    ysfx_t *fx = (ysfx_t *)opaque;
    std::unique_ptr<ysfx::mutex> file_mutex;
    std::unique_lock<ysfx::mutex> lock;
    std::unique_lock<ysfx::mutex> list_lock;

    // hold both locks to protect file and list access during removal
    if (!ysfx_get_file(fx, (uint32_t)handle, lock, &list_lock))
        return -1;

    // preserve the locked mutex of the object being removed
    file_mutex = std::move(fx->file.list[(uint32_t)handle]->m_mutex);

    fx->file.list[(uint32_t)handle].reset();
    return 0;
}

static EEL_F *NSEEL_CGEN_CALL ysfx_api_file_rewind(void *opaque, EEL_F *handle_)
{
    int32_t handle = ysfx_eel_round<int32_t>(*handle_);
    if (handle < 0)
        return handle_;

    ysfx_t *fx = (ysfx_t *)opaque;
    std::unique_lock<ysfx::mutex> lock;
    ysfx_file_t *file = ysfx_get_file(fx, (uint32_t)handle, lock);
    if (!file)
        return 0;

    file->rewind();
    return handle_;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_file_var(void *opaque, EEL_F *handle_, EEL_F *var)
{
    int32_t handle = ysfx_eel_round<int32_t>(*handle_);
    if (handle < 0)
        return 0;

    ysfx_t *fx = (ysfx_t *)opaque;
    std::unique_lock<ysfx::mutex> lock;
    ysfx_file_t *file = ysfx_get_file(fx, (uint32_t)handle, lock);
    if (!file)
        return 0;

    if (!file->var(var))
        return 0;

    return 1;
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_file_mem(void *opaque, EEL_F *handle_, EEL_F *offset_, EEL_F *length_)
{
    int32_t handle = ysfx_eel_round<int32_t>(*handle_);
    int32_t offset = ysfx_eel_round<int32_t>(*offset_);
    int32_t length = ysfx_eel_round<int32_t>(*length_);
    if (handle < 0 || offset < 0 || length <= 0)
        return 0;

    ysfx_t *fx = (ysfx_t *)opaque;
    std::unique_lock<ysfx::mutex> lock;
    ysfx_file_t *file = ysfx_get_file(fx, (uint32_t)handle, lock);
    if (!file)
        return 0;

    return (EEL_F)file->mem((uint32_t)offset, (uint32_t)length);
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_file_avail(void *opaque, EEL_F *handle_)
{
    int32_t handle = ysfx_eel_round<int32_t>(*handle_);
    if (handle < 0)
        return 0;

    ysfx_t *fx = (ysfx_t *)opaque;
    std::unique_lock<ysfx::mutex> lock;
    ysfx_file_t *file = ysfx_get_file(fx, (uint32_t)handle, lock);
    if (!file)
        return 0;

    return file->avail();
}

static EEL_F *NSEEL_CGEN_CALL ysfx_api_file_riff(void *opaque, EEL_F *handle_, EEL_F *nch_, EEL_F *samplerate_)
{
    int32_t handle = ysfx_eel_round<int32_t>(*handle_);
    if (handle < 0)
        return 0;

    ysfx_t *fx = (ysfx_t *)opaque;
    std::unique_lock<ysfx::mutex> lock;
    ysfx_file_t *file = ysfx_get_file(fx, (uint32_t)handle, lock);
    if (!file) {
        *nch_ = 0;
        *samplerate_ = 0;
        return nch_;
    }

    uint32_t nch = 0;
    ysfx_real samplerate = 0;
    if (!file->riff(nch, samplerate)) {
        *nch_ = 0;
        *samplerate_ = 0;
        return nch_;
    }

    *nch_ = (EEL_F)nch;
    *samplerate_ = samplerate;
    return nch_;
}


static EEL_F NSEEL_CGEN_CALL ysfx_api_file_text(void *opaque, EEL_F *handle_)
{
    int32_t handle = ysfx_eel_round<int32_t>(*handle_);
    if (handle < 0)
        return 0;

    ysfx_t *fx = (ysfx_t *)opaque;
    std::unique_lock<ysfx::mutex> lock;
    ysfx_file_t *file = ysfx_get_file(fx, (uint32_t)handle, lock);
    if (!file)
        return 0;

    return (EEL_F)file->is_text();
}

static EEL_F NSEEL_CGEN_CALL ysfx_api_file_string(void *opaque, EEL_F *handle_, EEL_F *string_)
{
    int32_t handle = ysfx_eel_round<int32_t>(*handle_);
    if (handle < 0)
        return 0;

    ysfx_t *fx = (ysfx_t *)opaque;
    std::unique_lock<ysfx::mutex> lock;
    ysfx_file_t *file = ysfx_get_file(fx, (uint32_t)handle, lock);
    if (!file)
        return 0;

    std::string txt;
    uint32_t count;
    if (!file->is_in_write_mode()) {
        count = file->string(txt);
        ysfx_string_set(fx, *string_, txt);
    }
    else {
        ysfx_string_get(fx, *string_, txt);
        count = file->string(txt);
    }
    return (EEL_F)count;
}

void ysfx_api_init_file()
{
    NSEEL_addfunc_retval("file_open", 1, NSEEL_PProc_THIS, &ysfx_api_file_open);
    NSEEL_addfunc_retval("file_close", 1, NSEEL_PProc_THIS, &ysfx_api_file_close);
    NSEEL_addfunc_retptr("file_rewind", 1, NSEEL_PProc_THIS, &ysfx_api_file_rewind);
    NSEEL_addfunc_retval("file_var", 2, NSEEL_PProc_THIS, &ysfx_api_file_var);
    NSEEL_addfunc_retval("file_mem", 3, NSEEL_PProc_THIS, &ysfx_api_file_mem);
    NSEEL_addfunc_retval("file_avail", 1, NSEEL_PProc_THIS, &ysfx_api_file_avail);
    NSEEL_addfunc_retptr("file_riff", 3, NSEEL_PProc_THIS, &ysfx_api_file_riff);
    NSEEL_addfunc_retval("file_text", 1, NSEEL_PProc_THIS, &ysfx_api_file_text);
    NSEEL_addfunc_retval("file_string", 2, NSEEL_PProc_THIS, &ysfx_api_file_string);
}
