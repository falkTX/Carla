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

#include "ysfx_audio_flac.hpp"
#include "ysfx_utils.hpp"
#include <memory>
#include <cstring>

#if defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wunused-function"
#endif

#define DR_FLAC_IMPLEMENTATION
#define DRFLAC_API static
#define DRFLAC_PRIVATE static
#include "dr_flac.h"

#if defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif

struct drflac_u_deleter {
    void operator()(drflac *x) const noexcept { drflac_close(x); }
};
using drflac_u = std::unique_ptr<drflac, drflac_u_deleter>;

///
struct ysfx_flac_reader_t {
    drflac_u flac;
    uint32_t nbuff = 0;
    std::unique_ptr<float[]> buff;
};

static bool ysfx_flac_can_handle(const char *path)
{
    return ysfx::path_has_suffix(path, "flac");
}

static ysfx_audio_reader_t *ysfx_flac_open(const char *path)
{
#if !defined(_WIN32)
    drflac_u flac{drflac_open_file(path, NULL)};
#else
    drflac_u flac{drflac_open_file_w(ysfx::widen(path).c_str(), NULL)};
#endif
    if (!flac)
        return nullptr;
    std::unique_ptr<ysfx_flac_reader_t> reader{new ysfx_flac_reader_t};
    reader->flac = std::move(flac);
    reader->buff.reset(new float[reader->flac->channels]);
    return (ysfx_audio_reader_t *)reader.release();
}

static void ysfx_flac_close(ysfx_audio_reader_t *reader_)
{
    ysfx_flac_reader_t *reader = (ysfx_flac_reader_t *)reader_;
    delete reader;
}

static ysfx_audio_file_info_t ysfx_flac_info(ysfx_audio_reader_t *reader_)
{
    ysfx_flac_reader_t *reader = (ysfx_flac_reader_t *)reader_;
    ysfx_audio_file_info_t info;
    info.channels = reader->flac->channels;
    info.sample_rate = (ysfx_real)reader->flac->sampleRate;
    return info;
}

static uint64_t ysfx_flac_avail(ysfx_audio_reader_t *reader_)
{
    ysfx_flac_reader_t *reader = (ysfx_flac_reader_t *)reader_;
    return reader->nbuff + reader->flac->channels * (reader->flac->totalPCMFrameCount - reader->flac->currentPCMFrame);
}

static void ysfx_flac_rewind(ysfx_audio_reader_t *reader_)
{
    ysfx_flac_reader_t *reader = (ysfx_flac_reader_t *)reader_;
    drflac_seek_to_pcm_frame(reader->flac.get(), 0);
    reader->nbuff = 0;
}

static uint64_t ysfx_flac_unload_buffer(ysfx_audio_reader_t *reader_, ysfx_real *samples, uint64_t count)
{
    ysfx_flac_reader_t *reader = (ysfx_flac_reader_t *)reader_;

    uint32_t nbuff = reader->nbuff;
    if (nbuff > count)
        nbuff = (uint32_t)count;

    if (nbuff == 0)
        return 0;

    const float *src = &reader->buff[reader->flac->channels - reader->nbuff];
    for (uint32_t i = 0; i < nbuff; ++i)
        samples[i] = src[i];

    reader->nbuff -= nbuff;
    return nbuff;
}

static uint64_t ysfx_flac_read(ysfx_audio_reader_t *reader_, ysfx_real *samples, uint64_t count)
{
    ysfx_flac_reader_t *reader = (ysfx_flac_reader_t *)reader_;
    uint32_t channels = reader->flac->channels;
    uint64_t readtotal = 0;

    if (count == 0)
        return readtotal;
    else {
        uint64_t copied = ysfx_flac_unload_buffer(reader_, samples, count);
        samples += copied;
        count -= copied;
        readtotal += copied;
    }

    if (count == 0)
        return readtotal;
    else {
        float *f32buf = (float *)samples;
        uint64_t readframes = drflac_read_pcm_frames_f32(reader->flac.get(), count / channels, f32buf);
        uint64_t readsamples = channels * readframes;
        // f32->f64
        for (uint64_t i = readsamples; i-- > 0; )
            samples[i] = f32buf[i];
        samples += readsamples;
        count -= readsamples;
        readtotal += readsamples;
    }

    if (count == 0)
        return readtotal;
    else if (drflac_read_pcm_frames_f32(reader->flac.get(), 1, reader->buff.get()) == 1) {
        reader->nbuff = channels;
        uint64_t copied = ysfx_flac_unload_buffer(reader_, samples, count);
        samples += copied;
        count -= copied;
        readtotal += copied;
    }

    return readtotal;
}

const ysfx_audio_format_t ysfx_audio_format_flac = {
    &ysfx_flac_can_handle,
    &ysfx_flac_open,
    &ysfx_flac_close,
    &ysfx_flac_info,
    &ysfx_flac_avail,
    &ysfx_flac_rewind,
    &ysfx_flac_read,
};
