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
#include "ysfx_reader.hpp"
#include "ysfx_utils.hpp"
#include <string>
#include <vector>
#include <cstdint>

struct ysfx_section_t;
struct ysfx_toplevel_t;
struct ysfx_slider_t;
struct ysfx_header_t;
using ysfx_section_u = std::unique_ptr<ysfx_section_t>;
using ysfx_toplevel_u = std::unique_ptr<ysfx_toplevel_t>;
using ysfx_slider_u = std::unique_ptr<ysfx_slider_t>;
using ysfx_header_u = std::unique_ptr<ysfx_header_t>;

struct ysfx_section_t {
    uint32_t line_offset = 0;
    std::string text;
};

struct ysfx_toplevel_t {
    ysfx_section_u header;
    ysfx_section_u init;
    ysfx_section_u slider;
    ysfx_section_u block;
    ysfx_section_u sample;
    ysfx_section_u serialize;
    ysfx_section_u gfx;
    uint32_t gfx_w = 0;
    uint32_t gfx_h = 0;
};

struct ysfx_parse_error {
    uint32_t line = 0;
    std::string message;
    explicit operator bool() { return !message.empty(); }
};

struct ysfx_slider_t {
    uint32_t id = 0;
    bool exists = false;
    ysfx_real def = 0;
    ysfx_real min = 0;
    ysfx_real max = 0;
    ysfx_real inc = 0;
    std::string var;
    std::string path;
    bool is_enum = false;
    ysfx::string_list enum_names;
    std::string desc;
    bool initially_visible = false;
};

struct ysfx_options_t {
    std::string gmem;
    uint32_t maxmem = 0;
    bool want_all_kb = false;
    bool no_meter = false;
};

struct ysfx_header_t {
    std::string desc;
    std::string author;
    ysfx::string_list tags;
    ysfx::string_list imports;
    ysfx::string_list in_pins;
    ysfx::string_list out_pins;
    bool explicit_pins = false;
    ysfx::string_list filenames;
    ysfx_options_t options;
    ysfx_slider_t sliders[ysfx_max_sliders];
};

struct ysfx_parsed_filename_t {
    uint32_t index;
    std::string filename;
};

bool ysfx_parse_toplevel(ysfx::text_reader &reader, ysfx_toplevel_t &toplevel, ysfx_parse_error *error);
bool ysfx_parse_slider(const char *line, ysfx_slider_t &slider);
bool ysfx_parse_filename(const char *line, ysfx_parsed_filename_t &filename);
void ysfx_parse_header(ysfx_section_t *section, ysfx_header_t &header);
