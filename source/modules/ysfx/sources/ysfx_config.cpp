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

#include "ysfx_config.hpp"
#include "ysfx_utils.hpp"
#include "ysfx_audio_wav.hpp"
#include "ysfx_audio_flac.hpp"
#include <cassert>

ysfx_config_t *ysfx_config_new()
{
    return new ysfx_config_t;
}

void ysfx_config_free(ysfx_config_t *config)
{
    if (!config)
        return;

    if (config->ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
        delete config;
}

void ysfx_config_add_ref(ysfx_config_t *config)
{
    config->ref_count.fetch_add(1, std::memory_order_relaxed);
}

void ysfx_set_import_root(ysfx_config_t *config, const char *root)
{
    config->import_root = ysfx::path_ensure_final_separator(root ? root : "");
}

void ysfx_set_data_root(ysfx_config_t *config, const char *root)
{
    config->data_root = ysfx::path_ensure_final_separator(root ? root : "");
}

const char *ysfx_get_import_root(ysfx_config_t *config)
{
    return config->import_root.c_str();
}

const char *ysfx_get_data_root(ysfx_config_t *config)
{
    return config->data_root.c_str();
}

void ysfx_guess_file_roots(ysfx_config_t *config, const char *sourcepath)
{
    if (config->import_root.empty()) {
        bool stop = false;
        const std::string sourcedir = ysfx::path_directory(sourcepath);

        std::string cur_dir = sourcedir + "../";
        ysfx::file_uid cur_uid;
        if (!ysfx::get_file_uid(cur_dir.c_str(), cur_uid))
            stop = true;

        while (!stop) {
            bool match =
                ysfx::exists((cur_dir + "Effects/").c_str()) &&
                ysfx::exists((cur_dir + "Data/").c_str());
            if (match) {
                stop = true;
                config->import_root = cur_dir + "Effects/";
            }
            else {
                cur_dir += "../";
                ysfx::file_uid old_uid = cur_uid;
                if (!ysfx::get_file_uid(cur_dir.c_str(), cur_uid) || old_uid == cur_uid)
                    stop = true;
            }
        }
    }

    if (config->data_root.empty() && !config->import_root.empty()) {
        const std::string datadir = config->import_root + "../Data/";

        bool match = ysfx::exists(datadir.c_str());
        if (match)
            config->data_root = datadir;
    }
}

void ysfx_register_audio_format(ysfx_config_t *config, ysfx_audio_format_t *afmt)
{
    config->audio_formats.push_back(*afmt);
}

void ysfx_register_builtin_audio_formats(ysfx_config_t *config)
{
    config->audio_formats.push_back(ysfx_audio_format_wav);
    config->audio_formats.push_back(ysfx_audio_format_flac);
}

void ysfx_set_log_reporter(ysfx_config_t *config, ysfx_log_reporter_t *reporter)
{
    config->log_reporter = reporter;
}

void ysfx_set_user_data(ysfx_config_t *config, intptr_t userdata)
{
    config->userdata = userdata;
}

//------------------------------------------------------------------------------
const char *ysfx_log_level_string(ysfx_log_level level)
{
    switch (level) {
    case ysfx_log_info:
        return "info";
    case ysfx_log_warning:
        return "warning";
    case ysfx_log_error:
        return "error";
    default:
        assert(false);
        return "?";
    }
}

void ysfx_log(ysfx_config_t &conf, ysfx_log_level level, const char *message)
{
    if (conf.log_reporter)
        conf.log_reporter(conf.userdata, level, message);
    else
        fprintf(stderr, "[ysfx] %s: %s\n", ysfx_log_level_string(level), message);
}

void ysfx_logfv(ysfx_config_t &conf, ysfx_log_level level, const char *format, va_list ap)
{
    char buf[256];
    vsnprintf(buf, sizeof(buf), format, ap);
    buf[sizeof(buf)-1] = '\0';
    ysfx_log(conf, level, buf);
}

void ysfx_logf(ysfx_config_t &conf, ysfx_log_level level, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    ysfx_logfv(conf, level, format, ap);
    va_end(ap);
}
