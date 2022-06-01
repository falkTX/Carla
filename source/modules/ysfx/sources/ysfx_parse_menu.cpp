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

#include "ysfx_parse_menu.hpp"
#include "ysfx_utils.hpp"
#include <vector>
#include <cstring>
#include <cassert>

static void ysfx_menu_insn_clear(ysfx_menu_insn_t *insn)
{
        delete[] insn->name;
}

static bool ysfx_do_create_menu(std::vector<ysfx_menu_insn_t> &insns, const char **str, uint32_t *startid, uint32_t menudepth)
{
    if (menudepth >= 8)
        return false;

    ///
    auto shrink_insns_to = [&insns](size_t count) {
        assert(insns.size() >= count);
        while (insns.size() > count) {
            ysfx_menu_insn_clear(&insns.back());
            insns.pop_back();
        }
    };

    ///
    size_t insn_count_at_start = insns.size();

    ///
    size_t pos = 0;
    uint32_t id = *startid;

    const char *p = *str;
    const char *sep = strchr(p, '|');
    while (sep || *p) {
        size_t len = sep ? (size_t)(sep - p) : strlen(p);
        std::string buf(p, len);
        p += len;
        if (sep)
            sep = strchr(++p, '|');

        const char *q = buf.c_str();
        bool subm = false;
        size_t insn_count_at_subm = 0;
        bool done = false;
        uint32_t item_flags = 0;
        while (*q && strchr(">#!<", *q)) {
            if (*q == '>' && !subm) {
                insn_count_at_subm = insns.size();
                insns.emplace_back();
                insns.back().opcode = ysfx_menu_sub;
                subm = ysfx_do_create_menu(insns, &p, &id, menudepth + 1);
                insns.emplace_back();
                insns.back().opcode = ysfx_menu_endsub;
                sep = strchr(p, '|');
            }
            if (*q == '#')
                item_flags |= ysfx_menu_item_disabled;
            if (*q == '!')
                item_flags |= ysfx_menu_item_checked;
            if (*q == '<')
                done = true;
            ++q;
        }
        if (*q) {
            if (subm) {
                for (ysfx_menu_insn_t *insn : {&insns[insn_count_at_subm], &insns.back()}) {
                    insn->name = ysfx::strdup_using_new(q);
                    insn->item_flags = item_flags;
                }
            }
            else {
                ysfx_menu_insn_t &insn = (insns.emplace_back(), insns.back());
                insn.opcode = ysfx_menu_item;
                insn.id = id++;
                insn.name = ysfx::strdup_using_new(q);
                insn.item_flags = item_flags;
            }
        }
        else {
            if (subm)
                shrink_insns_to(insn_count_at_subm);
            if (!done) {
                ysfx_menu_insn_t &insn = (insns.emplace_back(), insns.back());
                insn.opcode = ysfx_menu_separator;
            }
        }
        ++pos;
        if (done)
            break;
    }

    *str = p;
    *startid = id;

    ///
    if (!pos) {
        shrink_insns_to(insn_count_at_start);
        return false;
    }

    return true;
}

ysfx_menu_t *ysfx_parse_menu(const char *text)
{
    std::vector<ysfx_menu_insn_t> insns;
    insns.reserve(256);

    ///
    auto cleanup = ysfx::defer([&insns]() {
        for (ysfx_menu_insn_t &insn : insns)
            ysfx_menu_insn_clear(&insn);
    });

    ///
    const char *textpos = text;
    uint32_t id = 1;
    ysfx_do_create_menu(insns, &textpos, &id, 0);

    ///
    ysfx_menu_u menu{new ysfx_menu_t};
    menu->insn_count = (uint32_t)insns.size();
    menu->insns = new ysfx_menu_insn_t[menu->insn_count];
    memcpy(menu->insns, insns.data(), menu->insn_count * sizeof(ysfx_menu_insn_t));
    insns.clear();
    return menu.release();
}

void ysfx_menu_free(ysfx_menu_t *menu)
{
    if (!menu)
        return;

    for (uint32_t i = 0; i < menu->insn_count; ++i)
        ysfx_menu_insn_clear(&menu->insns[i]);

    delete[] menu->insns;
    delete menu;
}
