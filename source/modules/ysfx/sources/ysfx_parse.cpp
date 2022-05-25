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

#include "ysfx_parse.hpp"
#include "ysfx_utils.hpp"
#include <cstdlib>
#include <cstring>

bool ysfx_parse_toplevel(ysfx::text_reader &reader, ysfx_toplevel_t &toplevel, ysfx_parse_error *error)
{
    toplevel = ysfx_toplevel_t{};

    ysfx_section_t *current = new ysfx_section_t;
    toplevel.header.reset(current);

    std::string line;
    uint32_t lineno = 0;

    line.reserve(256);

    while (reader.read_next_line(line)) {
        const char *linep = line.c_str();

        if (linep[0] == '@') {
            // a new section starts
            ysfx::string_list tokens = ysfx::split_strings_noempty(linep, &ysfx::ascii_isspace);

            current = new ysfx_section_t;

            if (tokens[0] == "@init")
                toplevel.init.reset(current);
            else if (tokens[0] == "@slider")
                toplevel.slider.reset(current);
            else if (tokens[0] == "@block")
                toplevel.block.reset(current);
            else if (tokens[0] == "@sample")
                toplevel.sample.reset(current);
            else if (tokens[0] == "@serialize")
                toplevel.serialize.reset(current);
            else if (tokens[0] == "@gfx") {
                toplevel.gfx.reset(current);
                long gfx_w = 0;
                long gfx_h = 0;
                if (tokens.size() > 1)
                    gfx_w = (long)ysfx::dot_atof(tokens[1].c_str());
                if (tokens.size() > 2)
                    gfx_h = (long)ysfx::dot_atof(tokens[2].c_str());
                toplevel.gfx_w = (gfx_w > 0) ? (uint32_t)gfx_w : 0;
                toplevel.gfx_h = (gfx_h > 0) ? (uint32_t)gfx_h : 0;
            }
            else {
                delete current;
                if (error) {
                    error->line = lineno;
                    error->message = std::string("Invalid section: ") + line;
                }
                return false;
            }
            current->line_offset = lineno + 1;
        }
        else {
            current->text.append(line);
            current->text.push_back('\n');
        }

        ++lineno;
    }

    return true;
}

void ysfx_parse_header(ysfx_section_t *section, ysfx_header_t &header)
{
    header = ysfx_header_t{};

    ysfx::string_text_reader reader(section->text.c_str());

    std::string line;
    //uint32_t lineno = section->line_offset;

    line.reserve(256);

    ///
    auto unprefix = [](const char *text, const char **restp, const char *prefix) -> bool {
        size_t len = strlen(prefix);
        if (strncmp(text, prefix, len))
            return false;
        if (restp)
            *restp = text + len;
        return true;
    };

    //--------------------------------------------------------------------------
    // pass 1: regular metadata

    while (reader.read_next_line(line)) {
        const char *linep = line.c_str();
        const char *rest = nullptr;

        ysfx_slider_t slider;
        ysfx_parsed_filename_t filename;

        ///
        if (unprefix(linep, &rest, "desc:")) {
            if (header.desc.empty())
                header.desc = ysfx::trim(rest, &ysfx::ascii_isspace);
        }
        else if (unprefix(linep, &rest, "author:")) {
            if (header.author.empty())
                header.author = ysfx::trim(rest, &ysfx::ascii_isspace);
        }
        else if (unprefix(linep, &rest, "tags:")) {
            if (header.tags.empty()) {
                for (const std::string &tag : ysfx::split_strings_noempty(rest, &ysfx::ascii_isspace))
                    header.tags.push_back(tag);
            }
        }
        else if (unprefix(linep, &rest, "in_pin:")) {
            header.explicit_pins = true;
            header.in_pins.push_back(ysfx::trim(rest, &ysfx::ascii_isspace));
        }
        else if (unprefix(linep, &rest, "out_pin:")) {
            header.explicit_pins = true;
            header.out_pins.push_back(ysfx::trim(rest, &ysfx::ascii_isspace));
        }
        else if (unprefix(linep, &rest, "options:")) {
            for (const std::string &opt : ysfx::split_strings_noempty(rest, &ysfx::ascii_isspace)) {
                size_t pos = opt.find('=');
                std::string name = (pos == opt.npos) ? opt : opt.substr(0, pos);
                std::string value = (pos == opt.npos) ? std::string{} : opt.substr(pos + 1);
                if (name == "gmem")
                    header.options.gmem = value;
                else if (name == "maxmem") {
                    int32_t maxmem = (int32_t)ysfx::dot_atof(value.c_str());
                    header.options.maxmem = (maxmem < 0) ? 0 : (uint32_t)maxmem;
                }
                else if (name == "want_all_kb")
                    header.options.want_all_kb = true;
                else if (name == "no_meter")
                    header.options.no_meter = true;
            }
        }
        else if (unprefix(linep, &rest, "import") && ysfx::ascii_isspace(rest[0]))
            header.imports.push_back(ysfx::trim(rest + 1, &ysfx::ascii_isspace));
        else if (ysfx_parse_slider(linep, slider)) {
            if (slider.id >= ysfx_max_sliders)
                continue;
            slider.exists = true;
            header.sliders[slider.id] = slider;
        }
        else if (ysfx_parse_filename(linep, filename)) {
            if (filename.index != header.filenames.size())
                continue;
            header.filenames.push_back(std::move(filename.filename));
        }

        //++lineno;
    }

    //--------------------------------------------------------------------------
    // pass 2: comments

    reader = ysfx::string_text_reader{section->text.c_str()};

    while (reader.read_next_line(line)) {
        const char *linep = line.c_str();
        const char *rest = nullptr;

        // some files contain metadata in the form of comments
        // this is not part of spec, but we'll take this info regardless

        if (unprefix(linep, &rest, "//author:")) {
            if (header.author.empty())
                header.author = ysfx::trim(rest, &ysfx::ascii_isspace);
        }
        else if (unprefix(linep, &rest, "//tags:")) {
            if (header.tags.empty()) {
                for (const std::string &tag : ysfx::split_strings_noempty(rest, &ysfx::ascii_isspace))
                    header.tags.push_back(tag);
            }
        }
    }

    //--------------------------------------------------------------------------
    if (header.in_pins.size() == 1 && !ysfx::ascii_casecmp(header.in_pins.front().c_str(), "none"))
        header.in_pins.clear();
    if (header.out_pins.size() == 1 && !ysfx::ascii_casecmp(header.out_pins.front().c_str(), "none"))
        header.out_pins.clear();

    if (header.in_pins.size() > ysfx_max_channels)
        header.in_pins.resize(ysfx_max_channels);
    if (header.out_pins.size() > ysfx_max_channels)
        header.out_pins.resize(ysfx_max_channels);
}

bool ysfx_parse_slider(const char *line, ysfx_slider_t &slider)
{
    // NOTE this parser is intentionally very permissive,
    //   in order to match the reference behavior

    slider = ysfx_slider_t{};

    #define PARSE_FAIL do {                                                   \
        /*fprintf(stderr, "parse error (line %d): `%s`\n", __LINE__, line);*/ \
        return false;                                                         \
    } while (0)

    const char *cur = line;

    for (const char *p = "slider"; *p; ++p) {
        if (*cur++ != *p)
            PARSE_FAIL;
    }

    // parse ID (1-index)
    unsigned long id = strtoul(cur, (char **)&cur, 10);
    if (id < 1 || id > ysfx_max_sliders)
        PARSE_FAIL;
    slider.id = (uint32_t)--id;

    // semicolon
    if (*cur++ != ':')
        PARSE_FAIL;

    // search if there is an '=' sign prior to any '<' or ','
    // if there is, it's a custom variable
    {
        const char *pos = cur;
        for (char c; pos && (c = *pos) && c != '='; )
            pos = (c == '<' || c == ',') ? nullptr : (pos + 1);
        if (pos && *pos) {
            slider.var.assign(cur, pos);
            cur = pos + 1;
        }
        else
            slider.var = "slider" + std::to_string(id + 1);
    }

    // a regular slider
    if (*cur != '/') {
        slider.def = (ysfx_real)ysfx::dot_strtod(cur, (char **)&cur);

        while (*cur && *cur != ',' && *cur != '<') ++cur;
        if (!*cur) PARSE_FAIL;

        // no range specification
        if (*cur == ',')
            ++cur;
        // range specification
        else if (*cur == '<') {
            ++cur;

            // min
            slider.min = (ysfx_real)ysfx::dot_strtod(cur, (char **)&cur);

            while (*cur && *cur != ',' && *cur != '>') ++cur;
            if (!*cur) PARSE_FAIL;

            // max
            if (*cur == ',') {
                ++cur;
                slider.max = (ysfx_real)ysfx::dot_strtod(cur, (char **)&cur);

                while (*cur && *cur != ',' && *cur != '>') ++cur;
                if (!*cur) PARSE_FAIL;
            }

            // inc
            if (*cur == ',') {
                ++cur;
                slider.inc = (ysfx_real)ysfx::dot_strtod(cur, (char **)&cur);

                while (*cur && *cur != '{' && *cur != ',' && *cur != '>') ++cur;
                if (!*cur) PARSE_FAIL;

                // enumeration values
                if (*cur == '{') {
                    const char *pos = ++cur;

                    while (*cur && *cur != '}' && *cur != '>') ++cur;
                    if (!*cur) PARSE_FAIL;

                    slider.is_enum = true;
                    slider.enum_names = ysfx::split_strings_noempty(
                        std::string(pos, cur).c_str(),
                        [](char c) -> bool { return c == ','; });
                    for (std::string &name : slider.enum_names)
                        name = ysfx::trim(name.c_str(), &ysfx::ascii_isspace);
                }
            }

            while (*cur && *cur != '>') ++cur;
            if (!*cur) PARSE_FAIL;

            ++cur;
        }
        else
            PARSE_FAIL;

        // NOTE: skip ',' and whitespace. not sure why, it's how it is
        while (*cur && (*cur == ',' || ysfx::ascii_isspace(*cur))) ++cur;
        if (!*cur) PARSE_FAIL;
    }
    // a path slider
    else
    {
        const char *pos = cur;
        while (*cur && *cur != ':') ++cur;
        if (!*cur) PARSE_FAIL;

        slider.path.assign(pos, cur);
        ++cur;
        slider.def = (ysfx_real)ysfx::dot_strtod(cur, (char **)&cur);
        slider.inc = 1;
        slider.is_enum = true;

        while (*cur && *cur != ':') ++cur;
        if (!*cur) PARSE_FAIL;

        ++cur;
    }

    // description and visibility
    while (ysfx::ascii_isspace(*cur))
        ++cur;

    slider.initially_visible = true;
    if (*cur == '-') {
        ++cur;
        slider.initially_visible = false;
    }

    slider.desc = ysfx::trim(cur, &ysfx::ascii_isspace);
    if (slider.desc.empty())
        PARSE_FAIL;

    //
    #undef PARSE_FAIL

    return true;
}

bool ysfx_parse_filename(const char *line, ysfx_parsed_filename_t &filename)
{
    filename = ysfx_parsed_filename_t{};

    const char *cur = line;

    for (const char *p = "filename:"; *p; ++p) {
        if (*cur++ != *p)
            return false;
    }

    int64_t index = (int64_t)ysfx::dot_strtod(cur, (char **)&cur);
    if (index < 0 || index > ~(uint32_t)0)
        return false;

    while (*cur && *cur != ',') ++cur;
    if (!*cur) return false;;

    ++cur;

    filename.index = (uint32_t)index;
    filename.filename.assign(cur);
    return true;
}
