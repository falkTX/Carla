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
#include "ysfx_eel_utils.hpp"
#include <type_traits>
#include <algorithm>
#include <functional>
#include <deque>
#include <set>
#include <new>
#include <stdexcept>
#include <cstring>
#include <cassert>

static_assert(std::is_same<EEL_F, ysfx_real>::value,
              "ysfx_real is incorrectly defined");

enum {
    ysfx_max_file_handles = 64, // change if it needs more
};

//------------------------------------------------------------------------------
static thread_local ysfx_thread_id_t ysfx_thread_id;

ysfx_thread_id_t ysfx_get_thread_id()
{
    return ysfx_thread_id;
}

void ysfx_set_thread_id(ysfx_thread_id_t id)
{
    ysfx_thread_id = id;
}

//------------------------------------------------------------------------------
struct ysfx_api_initializer {
private:
    ysfx_api_initializer();
    ~ysfx_api_initializer();
public:
    static void init_once();
};

ysfx_api_initializer::ysfx_api_initializer()
{
    if (NSEEL_init() != 0)
        throw std::runtime_error("NSEEL_init");

    ysfx_api_init_eel();
    ysfx_api_init_reaper();
    ysfx_api_init_file();
    ysfx_api_init_gfx();
}

ysfx_api_initializer::~ysfx_api_initializer()
{
    NSEEL_quit();
}

void ysfx_api_initializer::init_once()
{
    static ysfx_api_initializer init;
}

//------------------------------------------------------------------------------
ysfx_t *ysfx_new(ysfx_config_t *config)
{
    ysfx_u fx{new ysfx_t};

    ysfx_config_add_ref(config);
    fx->config.reset(config);

    fx->string_ctx.reset(ysfx_eel_string_context_new());

    ysfx_api_initializer::init_once();

    NSEEL_VMCTX vm = NSEEL_VM_alloc();
    if (!vm)
        throw std::bad_alloc();
    fx->vm.reset(vm);

    NSEEL_VM_SetCustomFuncThis(vm, fx.get());

    ysfx_eel_string_initvm(vm);

#if !defined(YSFX_NO_GFX)
    fx->gfx.state.reset(ysfx_gfx_state_new(fx.get()));
#endif

    auto var_resolver = [](void *userdata, const char *name) -> EEL_F * {
        ysfx_t *fx = (ysfx_t *)userdata;
        auto it = fx->source.slider_alias.find(name);
        if (it != fx->source.slider_alias.end())
            return fx->var.slider[it->second];
        return nullptr;
    };
    NSEEL_VM_set_var_resolver(vm, var_resolver, fx.get());

    for (uint32_t i = 0; i < ysfx_max_channels; ++i) {
        std::string name = "spl" + std::to_string(i);
        EEL_F *var = NSEEL_VM_regvar(vm, name.c_str());
        *(fx->var.spl[i] = var) = 0;
    }
    for (uint32_t i = 0; i < ysfx_max_sliders; ++i) {
        std::string name = "slider" + std::to_string(i + 1);
        EEL_F *var = NSEEL_VM_regvar(vm, name.c_str());
        *(fx->var.slider[i] = var) = 0;
        fx->slider_of_var[var] = i;
    }

    #define AUTOVAR(name, value) *(fx->var.name = NSEEL_VM_regvar(vm, #name)) = (value)
    AUTOVAR(srate, fx->sample_rate);
    AUTOVAR(num_ch, fx->valid_input_channels);
    AUTOVAR(samplesblock, fx->block_size);
    AUTOVAR(trigger, 0);
    AUTOVAR(tempo, 120);
    AUTOVAR(play_state, 1);
    AUTOVAR(play_position, 0);
    AUTOVAR(beat_position, 0);
    AUTOVAR(ts_num, 0);
    AUTOVAR(ts_denom, 4);
    AUTOVAR(ext_noinit, 0);
    AUTOVAR(ext_nodenorm, 0);
    AUTOVAR(ext_midi_bus, 0);
    AUTOVAR(midi_bus, 0);
    AUTOVAR(pdc_delay, 0);
    AUTOVAR(pdc_bot_ch, 0);
    AUTOVAR(pdc_top_ch, 0);
    AUTOVAR(pdc_midi, 0);
    // gfx variables
    AUTOVAR(gfx_r, 0);
    AUTOVAR(gfx_g, 0);
    AUTOVAR(gfx_b, 0);
    AUTOVAR(gfx_a, 0);
    AUTOVAR(gfx_a2, 0);
    AUTOVAR(gfx_w, 0);
    AUTOVAR(gfx_h, 0);
    AUTOVAR(gfx_x, 0);
    AUTOVAR(gfx_y, 0);
    AUTOVAR(gfx_mode, 0);
    AUTOVAR(gfx_clear, 0);
    AUTOVAR(gfx_texth, 0);
    AUTOVAR(gfx_dest, 0);
    AUTOVAR(gfx_ext_retina, 0);
    AUTOVAR(mouse_x, 0);
    AUTOVAR(mouse_y, 0);
    AUTOVAR(mouse_cap, 0);
    AUTOVAR(mouse_wheel, 0);
    AUTOVAR(mouse_hwheel, 0);
    #undef AUTOVAR

    fx->midi.in.reset(new ysfx_midi_buffer_t);
    fx->midi.out.reset(new ysfx_midi_buffer_t);
    ysfx_set_midi_capacity(fx.get(), 1024, true);

    fx->file.list.reserve(16);
    fx->file.list.emplace_back(new ysfx_serializer_t(fx->vm.get()));

    return fx.release();
}

void ysfx_free(ysfx_t *fx)
{
    if (!fx)
        return;

    if (fx->ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
        delete fx;
}

void ysfx_add_ref(ysfx_t *fx)
{
    fx->ref_count.fetch_add(1, std::memory_order_relaxed);
}

ysfx_config_t *ysfx_get_config(ysfx_t *fx)
{
    return fx->config.get();
}

bool ysfx_load_file(ysfx_t *fx, const char *filepath, uint32_t loadopts)
{
    ysfx_unload(fx);

    //--------------------------------------------------------------------------
    // failure guard

    auto fail_guard = ysfx::defer([fx]() { ysfx_unload_source(fx); });

    //--------------------------------------------------------------------------
    // load the main file

    ysfx::file_uid main_uid;

    {
        ysfx_source_unit_u main{new ysfx_source_unit_t};

        ysfx::FILE_u stream{ysfx::fopen_utf8(filepath, "rb")};
        if (!stream || !ysfx::get_stream_file_uid(stream.get(), main_uid)) {
            ysfx_logf(*fx->config, ysfx_log_error, "%s: cannot open file for reading", ysfx::path_file_name(filepath).c_str());
            return false;
        }

        ysfx::stdio_text_reader reader(stream.get());

        ysfx_parse_error error;
        if (!ysfx_parse_toplevel(reader, main->toplevel, &error)) {
            ysfx_logf(*fx->config, ysfx_log_error, "%s:%u: %s", ysfx::path_file_name(filepath).c_str(), error.line + 1, error.message.c_str());
            return false;
        }
        ysfx_parse_header(main->toplevel.header.get(), main->header);

        // validity check
        if (main->header.desc.empty()) {
            ysfx_logf(*fx->config, ysfx_log_warning, "%s: the required `desc` field is missing", ysfx::path_file_name(filepath).c_str());
            main->header.desc = ysfx::path_file_name(filepath);
        }

        if (loadopts & ysfx_load_ignoring_imports)
            main->header.imports.clear();

        // if no pins are specified and we have @sample, the default is stereo
        if (main->toplevel.sample && !main->header.explicit_pins &&
            main->header.in_pins.empty() && main->header.out_pins.empty())
        {
            main->header.in_pins = {"JS input 1", "JS input 2"};
            main->header.out_pins = {"JS output 1", "JS output 2"};
        }

        // register variables aliased to sliders
        for (uint32_t i = 0; i < ysfx_max_sliders; ++i) {
            if (main->header.sliders[i].exists) {
                if (!main->header.sliders[i].var.empty())
                    fx->source.slider_alias.insert({main->header.sliders[i].var, i});
            }
        }

        fx->source.main = std::move(main);
        fx->source.main_file_path.assign(filepath);

        // find the bank file, if present
        ysfx::case_resolve(
            ysfx::path_directory(filepath).c_str(),
            (ysfx::path_file_name(filepath) + ".rpl").c_str(),
            fx->source.bank_path);

        // fill the file enums with the contents of directories
        ysfx_fill_file_enums(fx);

        // find incorrect enums and fix them
        ysfx_fix_invalid_enums(fx);

        // set the initial mask of visible sliders
        ysfx_update_slider_visibility_mask(fx);
    }

    //--------------------------------------------------------------------------
    // load the imports

    // we load the imports recursively using post-order

    static constexpr uint32_t max_import_level = 32;
    std::set<ysfx::file_uid> seen;

    std::function<bool(const std::string &, const std::string &, uint32_t)> do_next_import =
        [fx, &seen, &do_next_import]
        (const std::string &name, const std::string &origin, uint32_t level) -> bool
        {
            if (level >= max_import_level) {
                ysfx_logf(*fx->config, ysfx_log_error, "%s: %s", ysfx::path_file_name(origin.c_str()).c_str(), "too many import levels");
                return false;
            }

            std::string imported_path = ysfx_resolve_import_path(fx, name, origin);
            if (imported_path.empty()) {
                ysfx_logf(*fx->config, ysfx_log_error, "%s: cannot find import: %s", ysfx::path_file_name(origin.c_str()).c_str(), name.c_str());
                return false;
            }

            ysfx::file_uid imported_uid;
            ysfx::FILE_u stream{ysfx::fopen_utf8(imported_path.c_str(), "rb")};
            if (!stream || !ysfx::get_stream_file_uid(stream.get(), imported_uid)) {
                ysfx_logf(*fx->config, ysfx_log_error, "%s: cannot open file for reading", ysfx::path_file_name(imported_path.c_str()).c_str());
                return false;
            }

            // this file was already visited, skip
            if (!seen.insert(imported_uid).second)
                return true;

            // parse it
            ysfx_source_unit_u unit{new ysfx_source_unit_t};
            ysfx::stdio_text_reader reader(stream.get());

            ysfx_parse_error error;
            if (!ysfx_parse_toplevel(reader, unit->toplevel, &error)) {
                ysfx_logf(*fx->config, ysfx_log_error, "%s:%u: %s", ysfx::path_file_name(imported_path.c_str()).c_str(), error.line + 1, error.message.c_str());
                return false;
            }
            ysfx_parse_header(unit->toplevel.header.get(), unit->header);

            // process the imported dependencies, *first*
            for (const std::string &name : unit->header.imports) {
                if (!do_next_import(name, imported_path.c_str(), level + 1))
                    return false;
            }

            // add it to the import sources, *second*
            fx->source.imports.push_back(std::move(unit));

            return true;
        };

    for (const std::string &name : fx->source.main->header.imports) {
        if (!do_next_import(name, filepath, 0))
            return false;
    }

    //--------------------------------------------------------------------------
    // initialize the sliders to defaults

    for (uint32_t i = 0; i < ysfx_max_sliders; ++i)
        *fx->var.slider[i] = fx->source.main->header.sliders[i].def;

    //--------------------------------------------------------------------------

    fail_guard.disarm();
    return true;
}

bool ysfx_compile(ysfx_t *fx, uint32_t compileopts)
{
    ysfx_unload_code(fx);

    if (!fx->source.main) {
        ysfx_logf(*fx->config, ysfx_log_error, "???: no source is loaded, cannot compile");
        return false;
    }

    //--------------------------------------------------------------------------
    // failure guard

    auto fail_guard = ysfx::defer([fx]() { ysfx_unload_code(fx); });

    //--------------------------------------------------------------------------
    // configure VM

    NSEEL_VMCTX vm = fx->vm.get();

    {
        uint32_t maxmem = fx->source.main->header.options.maxmem;
        if (maxmem == 0)
            maxmem = 8 * 1024 * 1024;
        if (maxmem > 32 * 1024 * 1024)
            maxmem = 32 * 1024 * 1024;

        NSEEL_VM_setramsize(vm, (int)maxmem);
    }

    //--------------------------------------------------------------------------
    // compile

    auto compile_section =
        [fx](ysfx_section_t *section, const char *name, NSEEL_CODEHANDLE_u &dest) -> bool
        {
            NSEEL_VMCTX vm = fx->vm.get();
            if (section->text.empty()) {
                // NOTE: check for empty source, which would return null code
                dest.reset();
                return true;
            }
            NSEEL_CODEHANDLE_u code{NSEEL_code_compile_ex(vm, section->text.c_str(), section->line_offset, NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS)};
            if (!code) {
                ysfx_logf(*fx->config, ysfx_log_error, "%s: %s", name, NSEEL_code_getcodeerror(vm));
                return false;
            }
            dest = std::move(code);
            return true;
        };

    // compile the multiple @init sections, imports first
    {
        std::vector<ysfx_section_t *> secs;
        secs.reserve(fx->source.imports.size() + 1);

        // collect init sections: imports first, main second
        for (size_t i = 0; i < fx->source.imports.size(); ++i)
            secs.push_back(fx->source.imports[i]->toplevel.init.get());
        secs.push_back(fx->source.main->toplevel.init.get());

        for (ysfx_section_t *sec : secs) {
            NSEEL_CODEHANDLE_u code;
            if (sec && !compile_section(sec, "@init", code))
                return false;
            fx->code.init.push_back(std::move(code));
        }
    }

    // compile the other sections, single
    // a non-@init section is searched in the main file first;
    // if not found, it's inherited from the first import which has it.
    ysfx_section_t *slider = ysfx_search_section(fx, ysfx_section_slider);
    ysfx_section_t *block = ysfx_search_section(fx, ysfx_section_block);
    ysfx_section_t *sample = ysfx_search_section(fx, ysfx_section_sample);
    ysfx_section_t *gfx = nullptr;
    ysfx_section_t *serialize = nullptr;
    if ((compileopts & ysfx_compile_no_gfx) == 0)
        gfx = ysfx_search_section(fx, ysfx_section_gfx);
    if ((compileopts & ysfx_compile_no_serialize) == 0)
        serialize = ysfx_search_section(fx, ysfx_section_serialize);

    if (slider && !compile_section(slider, "@slider", fx->code.slider))
        return false;
    if (block && !compile_section(block, "@block", fx->code.block))
        return false;
    if (sample && !compile_section(sample, "@sample", fx->code.sample))
        return false;
    if (gfx && !compile_section(gfx, "@gfx", fx->code.gfx))
        return false;
    if (serialize && !compile_section(serialize, "@serialize", fx->code.serialize))
        return false;

    fx->code.compiled = true;
    fx->is_freshly_compiled = true;
    fx->must_compute_init = true;

    ///
    ysfx_eel_string_context_update_named_vars(fx->string_ctx.get(), vm);

    fail_guard.disarm();
    return true;
}

bool ysfx_is_compiled(ysfx_t *fx)
{
    return fx->code.compiled;
}

void ysfx_unload_source(ysfx_t *fx)
{
    fx->source = {};
}

void ysfx_unload_code(ysfx_t *fx)
{
#if !defined(YSFX_NO_GFX)
    // get rid of gfx first, to prevent a UI thread from trying
    // to access VM and invoke code
    {
        std::lock_guard<ysfx::mutex> lock{fx->gfx.mutex};
        fx->gfx.ready = false;
        fx->gfx.wants_retina = false;
        fx->gfx.must_init.store(false);
    }
#endif

    fx->code = {};

    fx->is_freshly_compiled = false;
    fx->must_compute_init = false;
    fx->must_compute_slider = false;

    NSEEL_VMCTX vm = fx->vm.get();
    NSEEL_code_compile_ex(vm, nullptr, 0, NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS_RESET);
    NSEEL_VM_remove_unused_vars(vm);
    NSEEL_VM_remove_all_nonreg_vars(vm);
    NSEEL_VM_freeRAM(vm);
}

void ysfx_unload(ysfx_t *fx)
{
    ysfx_unload_code(fx);
    ysfx_unload_source(fx);
}

bool ysfx_is_loaded(ysfx_t *fx)
{
    return fx->source.main != nullptr;
}

void ysfx_fill_file_enums(ysfx_t *fx)
{
    if (fx->config->data_root.empty())
        return;

    for (uint32_t i = 0; i < ysfx_max_sliders; ++i) {
        ysfx_slider_t &slider = fx->source.main->header.sliders[i];
        if (slider.path.empty())
            continue;

        std::string dirpath = ysfx::path_ensure_final_separator((fx->config->data_root + slider.path).c_str());
        ysfx::string_list entries = ysfx::list_directory(dirpath.c_str());

        for (const std::string &filename : entries) {
            if (!filename.empty() && ysfx::is_path_separator(filename.back()))
                continue;

            std::string filepath = dirpath + filename;

            ysfx_file_type_t ftype = ysfx_detect_file_type(fx, filepath.c_str(), nullptr);
            if (ftype == ysfx_file_type_none)
                continue;

            slider.enum_names.push_back(std::move(filename));
        }

        if (!slider.enum_names.empty())
            slider.max = (EEL_F)(slider.enum_names.size() - 1);
    }
}

void ysfx_fix_invalid_enums(ysfx_t *fx)
{
    //NOTE: regardless of the range of enum sliders in source, it is <0,N-1,1>
    //  if there is a mismatch, correct and output a warning

    for (uint32_t i = 0; i < ysfx_max_sliders; ++i) {
        ysfx_slider_t &slider = fx->source.main->header.sliders[i];
        if (!slider.is_enum)
            continue;

        uint32_t count = (uint32_t)slider.enum_names.size();
        if (count == 0) {
            bool is_file = !slider.path.empty();
            ysfx_logf(*fx->config, ysfx_log_warning, "slider%u: the enumeration does not contain any %s", i + 1, is_file ? "files" : "items");
            slider.enum_names.emplace_back();
            slider.min = 0;
            slider.max = 0;
            slider.inc = 1;
        }
        else if (slider.min != 0 || slider.inc != 1 || slider.max != (EEL_F)(count - 1)) {
            ysfx_logf(*fx->config, ysfx_log_warning, "slider%u: the enumeration has an invalid range", i + 1);
            slider.min = 0;
            slider.max = (EEL_F)(count - 1);
            slider.inc = 1;
        }
    }
}

const char *ysfx_get_name(ysfx_t *fx)
{
    ysfx_source_unit_t *main = fx->source.main.get();
    if (!main)
        return "";
    return main->header.desc.c_str();
}

const char *ysfx_get_file_path(ysfx_t *fx)
{
    return fx->source.main_file_path.c_str();
}

const char *ysfx_get_author(ysfx_t *fx)
{
    ysfx_source_unit_t *main = fx->source.main.get();
    if (!main)
        return "";
    return main->header.author.c_str();
}

uint32_t ysfx_get_tags(ysfx_t *fx, const char **dest, uint32_t destsize)
{
    ysfx_source_unit_t *main = fx->source.main.get();
    if (!main)
        return 0;

    uint32_t count = (uint32_t)main->header.tags.size();

    uint32_t copysize = (destsize < count) ? destsize : count;
    for (uint32_t i = 0; i < copysize; ++i)
        dest[i] = main->header.tags[i].c_str();

    return count;
}

const char *ysfx_get_tag(ysfx_t *fx, uint32_t index)
{
    ysfx_source_unit_t *main = fx->source.main.get();
    if (!main || index >= main->header.tags.size())
        return "";
    return main->header.tags[index].c_str();
}

uint32_t ysfx_get_num_inputs(ysfx_t *fx)
{
    ysfx_source_unit_t *main = fx->source.main.get();
    if (!main)
        return 0;
    return (uint32_t)main->header.in_pins.size();
}

uint32_t ysfx_get_num_outputs(ysfx_t *fx)
{
    ysfx_source_unit_t *main = fx->source.main.get();
    if (!main)
        return 0;
    return (uint32_t)main->header.out_pins.size();
}

const char *ysfx_get_input_name(ysfx_t *fx, uint32_t index)
{
    ysfx_source_unit_t *main = fx->source.main.get();
    if (!main || index >= main->header.in_pins.size())
        return "";
    return main->header.in_pins[index].c_str();
}

const char *ysfx_get_output_name(ysfx_t *fx, uint32_t index)
{
    ysfx_source_unit_t *main = fx->source.main.get();
    if (!main || index >= main->header.out_pins.size())
        return "";
    return main->header.out_pins[index].c_str();
}

bool ysfx_wants_meters(ysfx_t *fx)
{
    ysfx_source_unit_t *main = fx->source.main.get();
    if (!main)
        return false;

    return !main->header.options.no_meter;
}

bool ysfx_get_gfx_dim(ysfx_t *fx, uint32_t dim[2])
{
    ysfx_toplevel_t *origin = nullptr;
    ysfx_section_t *sec = ysfx_search_section(fx, ysfx_section_gfx, &origin);

    if (!sec) {
        if (dim) {
            dim[0] = 0;
            dim[1] = 0;
        }
        return false;
    }

    if (dim) {
        dim[0] = origin->gfx_w;
        dim[1] = origin->gfx_h;
    }
    return true;
}

ysfx_section_t *ysfx_search_section(ysfx_t *fx, uint32_t type, ysfx_toplevel_t **origin)
{
    if (!fx->source.main)
        return nullptr;

    auto search =
        [fx](ysfx_section_t *(*test)(ysfx_toplevel_t &tl), ysfx_toplevel_t **origin) -> ysfx_section_t *
        {
            ysfx_toplevel_t *tl = &fx->source.main->toplevel;
            ysfx_section_t *sec = test(*tl);
            for (size_t i = 0; !sec && i < fx->source.imports.size(); ++i) {
                tl = &fx->source.imports[i]->toplevel;
                sec = test(*tl);
            }
            if (origin)
                *origin = sec ? tl : nullptr;
            return sec;
        };

    switch (type) {
    case ysfx_section_init:
        return search([](ysfx_toplevel_t &tl) { return tl.init.get(); }, origin);
    case ysfx_section_slider:
        return search([](ysfx_toplevel_t &tl) { return tl.slider.get(); }, origin);
    case ysfx_section_block:
        return search([](ysfx_toplevel_t &tl) { return tl.block.get(); }, origin);
    case ysfx_section_sample:
        return search([](ysfx_toplevel_t &tl) { return tl.sample.get(); }, origin);
    case ysfx_section_gfx:
        return search([](ysfx_toplevel_t &tl) { return tl.gfx.get(); }, origin);
    case ysfx_section_serialize:
        return search([](ysfx_toplevel_t &tl) { return tl.serialize.get(); }, origin);
    default:
        return nullptr;
    }
}

bool ysfx_has_section(ysfx_t *fx, uint32_t type)
{
    return ysfx_search_section(fx, type) != nullptr;
}

bool ysfx_slider_exists(ysfx_t *fx, uint32_t index)
{
    ysfx_source_unit_t *main = fx->source.main.get();
    if (index >= ysfx_max_sliders || !main)
        return false;

    ysfx_slider_t &slider = main->header.sliders[index];
    return slider.exists;
}

const char *ysfx_slider_get_name(ysfx_t *fx, uint32_t index)
{
    ysfx_source_unit_t *main = fx->source.main.get();
    if (index >= ysfx_max_sliders || !main)
        return "";

    ysfx_slider_t &slider = main->header.sliders[index];
    return slider.desc.c_str();
}

bool ysfx_slider_get_range(ysfx_t *fx, uint32_t index, ysfx_slider_range_t *range)
{
    ysfx_source_unit_t *main = fx->source.main.get();
    if (index >= ysfx_max_sliders || !main)
        return false;

    ysfx_slider_t &slider = main->header.sliders[index];
    range->def = slider.def;
    range->min = slider.min;
    range->max = slider.max;
    range->inc = slider.inc;
    return true;
}

bool ysfx_slider_is_enum(ysfx_t *fx, uint32_t index)
{
    ysfx_source_unit_t *main = fx->source.main.get();
    if (index >= ysfx_max_sliders || !main)
        return false;

    ysfx_slider_t &slider = main->header.sliders[index];
    return slider.is_enum;
}

uint32_t ysfx_slider_get_enum_names(ysfx_t *fx, uint32_t index, const char **dest, uint32_t destsize)
{
    ysfx_source_unit_t *main = fx->source.main.get();
    if (index >= ysfx_max_sliders || !main)
        return 0;

    ysfx_slider_t &slider = main->header.sliders[index];
    uint32_t count = (uint32_t)slider.enum_names.size();

    uint32_t copysize = (destsize < count) ? destsize : count;
    for (uint32_t i = 0; i < copysize; ++i)
        dest[i] = slider.enum_names[i].c_str();

    return count;
}

const char *ysfx_slider_get_enum_name(ysfx_t *fx, uint32_t slider_index, uint32_t enum_index)
{
    ysfx_source_unit_t *main = fx->source.main.get();
    if (slider_index >= ysfx_max_sliders || !main)
        return 0;

    ysfx_slider_t &slider = main->header.sliders[slider_index];
    if (enum_index >= slider.enum_names.size())
        return "";

    return slider.enum_names[enum_index].c_str();
}

bool ysfx_slider_is_path(ysfx_t *fx, uint32_t index)
{
    ysfx_source_unit_t *main = fx->source.main.get();
    if (index >= ysfx_max_sliders || !main)
        return false;

    ysfx_slider_t &slider = main->header.sliders[index];
    return !slider.path.empty();
}

bool ysfx_slider_is_initially_visible(ysfx_t *fx, uint32_t index)
{
    ysfx_source_unit_t *main = fx->source.main.get();
    if (index >= ysfx_max_sliders || !main)
        return false;

    ysfx_slider_t &slider = main->header.sliders[index];
    return slider.initially_visible;
}

ysfx_real ysfx_slider_get_value(ysfx_t *fx, uint32_t index)
{
    if (index >= ysfx_max_sliders)
        return 0;
    return *fx->var.slider[index];
}

void ysfx_slider_set_value(ysfx_t *fx, uint32_t index, ysfx_real value)
{
    if (index >= ysfx_max_sliders)
        return;
    if (*fx->var.slider[index] != value) {
        *fx->var.slider[index] = value;
        fx->must_compute_slider = true;
    }
}

std::string ysfx_resolve_import_path(ysfx_t *fx, const std::string &name, const std::string &origin)
{
    std::vector<std::string> dirs;

    // create the list of search directories
    {
        dirs.reserve(2);

        if (!origin.empty())
            dirs.push_back(ysfx::path_directory(origin.c_str()));

        const std::string &import_root = fx->config->import_root;
        if (!import_root.empty() && dirs[0] != import_root)
            dirs.push_back(import_root);
    }

    // the search should be case-insensitive
    static constexpr bool nocase = true;

    static auto *check_existence = +[](const std::string &dir, const std::string &file, std::string &result_path) -> int {
        if (nocase)
            return ysfx::case_resolve(dir.c_str(), file.c_str(), result_path);
        else {
            result_path = dir + file;
            return ysfx::exists(result_path.c_str());
        }
    };

    // search for the file in these directories directly
    for (const std::string &dir : dirs) {
        std::string resolved;
        if (check_existence(dir, name, resolved))
            return resolved;
    }

    // search for the file recursively
    for (const std::string &dir : dirs) {
        struct visit_data {
            const std::string *name = nullptr;
            std::string resolved;
        };
        visit_data vd;
        vd.name = &name;
        auto visit = [](const std::string &dir, void *data) -> bool {
            visit_data &vd = *(visit_data *)data;
            std::string resolved;
            if (check_existence(dir, *vd.name, resolved)) {
                vd.resolved = std::move(resolved);
                return false;
            }
            return true;
        };
        ysfx::visit_directories(dir.c_str(), +visit, &vd);
        if (!vd.resolved.empty())
            return vd.resolved;
    }

    return std::string{};
}


uint32_t ysfx_get_block_size(ysfx_t *fx)
{
    return fx->block_size;
}

ysfx_real ysfx_get_sample_rate(ysfx_t *fx)
{
    return fx->sample_rate;
}

void ysfx_set_block_size(ysfx_t *fx, uint32_t blocksize)
{
    if (fx->block_size != blocksize) {
        fx->block_size = blocksize;
        fx->must_compute_init = true;
    }
}

void ysfx_set_sample_rate(ysfx_t *fx, ysfx_real samplerate)
{
    if (fx->sample_rate != samplerate) {
        fx->sample_rate = samplerate;
        fx->must_compute_init = true;
    }
}

void ysfx_set_midi_capacity(ysfx_t *fx, uint32_t capacity, bool extensible)
{
    ysfx_midi_reserve(fx->midi.in.get(), capacity, extensible);
    ysfx_midi_reserve(fx->midi.out.get(), capacity, extensible);
}

void ysfx_init(ysfx_t *fx)
{
    if (!fx->code.compiled)
        return;

    *fx->var.samplesblock = (EEL_F)fx->block_size;
    *fx->var.srate = fx->sample_rate;

    *fx->var.pdc_delay = 0;
    *fx->var.pdc_bot_ch = 0;
    *fx->var.pdc_top_ch = 0;
    *fx->var.pdc_midi = 0;

    if (fx->is_freshly_compiled) {
        ysfx_first_init(fx);
        fx->is_freshly_compiled = false;
    }

    ysfx_clear_files(fx);

    for (size_t i = 0; i < fx->code.init.size(); ++i)
        NSEEL_code_execute(fx->code.init[i].get());

    fx->must_compute_init = false;
    fx->must_compute_slider = true;

#if !defined(YSFX_NO_GFX)
    // do initializations on next @gfx, on the gfx thread
    // release-acquire order is for VM `gfx_*` variables and `wants_retina`
    fx->gfx.wants_retina = *fx->var.gfx_ext_retina > 0;
    fx->gfx.must_init.store(true, std::memory_order_release);
#endif
}

void ysfx_first_init(ysfx_t *fx)
{
    assert(fx->code.compiled);
    assert(fx->is_freshly_compiled);

    fx->slider.automate_mask.store(0);
    fx->slider.change_mask.store(0);
    ysfx_update_slider_visibility_mask(fx);
}

ysfx_real ysfx_get_pdc_delay(ysfx_t *fx)
{
    ysfx_real value = *fx->var.pdc_delay;
    return (value > 0) ? value : 0;
}

void ysfx_get_pdc_channels(ysfx_t *fx, uint32_t channels[2])
{
    if (!channels)
        return;

    int64_t bot = (int64_t)*fx->var.pdc_bot_ch;
    bot = (bot > 0) ? bot : 0;
    bot = (bot < ysfx_max_channels) ? bot : ysfx_max_channels;
    channels[0] = (uint32_t)bot;

    int64_t top = (int64_t)*fx->var.pdc_top_ch;
    top = (top > bot) ? top : bot;
    top = (top < ysfx_max_channels) ? top : ysfx_max_channels;
    channels[1] = (uint32_t)top;
}

bool ysfx_get_pdc_midi(ysfx_t *fx)
{
    return (bool)*fx->var.pdc_midi;
}

void ysfx_update_slider_visibility_mask(ysfx_t *fx)
{
    uint64_t visible = 0;
    for (uint32_t i = 0; i < ysfx_max_sliders; ++i) {
        ysfx_slider_t &slider = fx->source.main->header.sliders[i];
        visible |= (uint64_t)slider.initially_visible << i;
    }
    fx->slider.visible_mask.store(visible);
}

void ysfx_set_time_info(ysfx_t *fx, const ysfx_time_info_t *info)
{
    uint32_t prev_state = (uint32_t)*fx->var.play_state;
    uint32_t new_state = info->playback_state;

    // unless `ext_noinit`, we should call @init every transport restart
    if (!*fx->var.ext_noinit) {
        auto is_running = [](uint32_t state) {
            return state == ysfx_playback_playing ||
                state == ysfx_playback_recording;
        };
        if (!is_running(prev_state) && is_running(new_state))
            fx->must_compute_init = true;
    }

    *fx->var.tempo = info->tempo;
    *fx->var.play_state = (EEL_F)new_state;
    *fx->var.play_position = info->time_position;
    *fx->var.beat_position = info->beat_position;
    *fx->var.ts_num = (EEL_F)info->time_signature[0];
    *fx->var.ts_denom = (EEL_F)info->time_signature[1];
}

bool ysfx_send_midi(ysfx_t *fx, const ysfx_midi_event_t *event)
{
    return ysfx_midi_push(fx->midi.in.get(), event);
}

bool ysfx_receive_midi(ysfx_t *fx, ysfx_midi_event_t *event)
{
    return ysfx_midi_get_next(fx->midi.out.get(), event);
}

bool ysfx_receive_midi_from_bus(ysfx_t *fx, uint32_t bus, ysfx_midi_event_t *event)
{
    return ysfx_midi_get_next_from_bus(fx->midi.out.get(), 0, event);
}

uint32_t ysfx_current_midi_bus(ysfx_t *fx)
{
    uint32_t bus = 0;
    if (*fx->var.ext_midi_bus)
        bus = (int32_t)*fx->var.midi_bus;
    return bus;
}

bool ysfx_send_trigger(ysfx_t *fx, uint32_t index)
{
    if (index >= ysfx_max_triggers)
        return false;

    fx->triggers |= 1u << index;
    return true;
}

uint64_t ysfx_fetch_slider_changes(ysfx_t *fx)
{
    return fx->slider.change_mask.exchange(0);
}

uint64_t ysfx_fetch_slider_automations(ysfx_t *fx)
{
    return fx->slider.automate_mask.exchange(0);
}

uint64_t ysfx_get_slider_visibility(ysfx_t *fx)
{
    return fx->slider.visible_mask.load();
}

template <class Real>
void ysfx_process_generic(ysfx_t *fx, const Real *const *ins, Real *const *outs, uint32_t num_ins, uint32_t num_outs, uint32_t num_frames)
{
    ysfx_set_thread_id(ysfx_thread_id_dsp);

    // prepare MIDI input for reading, output for writing
    assert(fx->midi.in->read_pos == 0);
    ysfx_midi_clear(fx->midi.out.get());

    // prepare triggers
    *fx->var.trigger = (EEL_F)fx->triggers;
    fx->triggers = 0;

    if (!fx->code.compiled) {
        for (uint32_t ch = 0; ch < num_outs; ++ch)
            memset(outs[ch], 0, num_frames * sizeof(Real));
    }
    else {
        // compute @init if needed
        if (fx->must_compute_init)
            ysfx_init(fx);

        const uint32_t orig_num_outs = num_outs;
        const uint32_t num_code_ins = (uint32_t)fx->source.main->header.in_pins.size();
        const uint32_t num_code_outs = (uint32_t)fx->source.main->header.out_pins.size();
        if (num_ins > num_code_ins)
            num_ins = num_code_ins;
        if (num_outs > num_code_outs)
            num_outs = num_code_outs;

        fx->valid_input_channels = num_ins;

        *fx->var.samplesblock = (EEL_F)num_frames;
        *fx->var.num_ch = (EEL_F)num_ins;

        // compute @slider if needed
        if (fx->must_compute_slider) {
            NSEEL_code_execute(fx->code.slider.get());
            fx->must_compute_slider = false;
        }

        // compute @block
        NSEEL_code_execute(fx->code.block.get());

        // compute @sample, once per frame
        if (fx->code.sample) {
            EEL_F **spl = fx->var.spl;
            for (uint32_t i = 0; i < num_frames; ++i) {
                for (uint32_t ch = 0; ch < num_ins; ++ch)
                    *spl[ch] = (EEL_F)ins[ch][i];
                for (uint32_t ch = num_ins; ch < num_code_ins; ++ch)
                    *spl[ch] = 0;
                NSEEL_code_execute(fx->code.sample.get());
                for (uint32_t ch = 0; ch < num_outs; ++ch)
                    outs[ch][i] = (Real)*spl[ch];
            }
        }

        // clear any output channels above the maximum count
        for (uint32_t ch = num_outs; ch < orig_num_outs; ++ch)
            memset(outs[ch], 0, num_frames * sizeof(Real));
    }

    // prepare MIDI input for writing, output for reading
    assert(fx->midi.out->read_pos == 0);
    ysfx_midi_clear(fx->midi.in.get());

    ysfx_set_thread_id(ysfx_thread_id_none);
}

void ysfx_process_float(ysfx_t *fx, const float *const *ins, float *const *outs, uint32_t num_ins, uint32_t num_outs, uint32_t num_frames)
{
    ysfx_process_generic<float>(fx, ins, outs, num_ins, num_outs, num_frames);
}

void ysfx_process_double(ysfx_t *fx, const double *const *ins, double *const *outs, uint32_t num_ins, uint32_t num_outs, uint32_t num_frames)
{
    ysfx_process_generic<double>(fx, ins, outs, num_ins, num_outs, num_frames);
}

void ysfx_clear_files(ysfx_t *fx)
{
    std::lock_guard<ysfx::mutex> list_lock(fx->file.list_mutex);

    // delete all except the serializer
    while (fx->file.list.size() > 1) {
        ysfx_file_t *file = fx->file.list.back().get();
        std::unique_ptr<ysfx::mutex> file_mutex;
        std::unique_lock<ysfx::mutex> file_lock;
        if (file) {
            file_lock = std::unique_lock<ysfx::mutex>{*fx->file.list.back()->m_mutex};
            file_mutex = std::move(fx->file.list.back()->m_mutex);
        }
        fx->file.list.pop_back();
    }
}

ysfx_file_t *ysfx_get_file(ysfx_t *fx, uint32_t handle, std::unique_lock<ysfx::mutex> &lock, std::unique_lock<ysfx::mutex> *list_lock)
{
    std::unique_lock<ysfx::mutex> local_list_lock;
    if (list_lock)
        *list_lock = std::unique_lock<ysfx::mutex>(fx->file.list_mutex);
    else
        local_list_lock = std::unique_lock<ysfx::mutex>(fx->file.list_mutex);
    if (handle >= fx->file.list.size())
        return nullptr;
    ysfx_file_t *file = fx->file.list[handle].get();
    if (!file)
        return nullptr;
    lock = std::unique_lock<ysfx::mutex>{*file->m_mutex};
    return file;
}

int32_t ysfx_insert_file(ysfx_t *fx, ysfx_file_t *file)
{
    std::lock_guard<ysfx::mutex> lock(fx->file.list_mutex);

    //
    size_t noneidx = ~(size_t)0;
    size_t freeidx = noneidx;

    for (size_t i = 0, n = fx->file.list.size(); i < n && freeidx == noneidx; ++i) {
        if (!fx->file.list[i])
            freeidx = i;
    }
    if (freeidx != noneidx) {
        fx->file.list[freeidx].reset(file);
        return (uint32_t)freeidx;
    }

    enum { max_file_handles = 64 };

    size_t pos = fx->file.list.size();
    if (pos >= max_file_handles)
        return -1;

    fx->file.list.emplace_back(file);
    return (uint32_t)pos;
}

bool ysfx_load_state(ysfx_t *fx, ysfx_state_t *state)
{
    if (!fx->code.compiled)
        return false;

    // restore the serialization
    std::string buffer((char *)state->data, state->data_size);

    // restore the sliders
    for (uint32_t i = 0; i < ysfx_max_sliders; ++i)
        *fx->var.slider[i] = fx->source.main->header.sliders[i].def;

    for (uint32_t i = 0, n = state->slider_count; i < n; ++i) {
        uint32_t j = state->sliders[i].index;
        if (j < ysfx_max_sliders && fx->source.main->header.sliders[j].exists)
            *fx->var.slider[j] = state->sliders[i].value;
    }
    fx->must_compute_slider = true;

    // invoke @serialize
    {
        std::unique_lock<ysfx::mutex> lock;
        ysfx_serializer_t *serializer = static_cast<ysfx_serializer_t *>(ysfx_get_file(fx, 0, lock));
        assert(serializer);
        serializer->begin(false, buffer);
        lock.unlock();
        ysfx_serialize(fx);
        lock.lock();
        serializer->end();
    }

    return true;
}

ysfx_state_t *ysfx_save_state(ysfx_t *fx)
{
    if (!fx->code.compiled)
        return nullptr;

    std::string buffer;

    // invoke @serialize
    {
        std::unique_lock<ysfx::mutex> lock;
        ysfx_serializer_t *serializer = static_cast<ysfx_serializer_t *>(ysfx_get_file(fx, 0, lock));
        assert(serializer);
        serializer->begin(true, buffer);
        lock.unlock();
        ysfx_serialize(fx);
        lock.lock();
        serializer->end();
    }

    // save the sliders
    ysfx_state_u state{new ysfx_state_t};
    uint32_t slider_count = 0;
    for (uint32_t i = 0; i < ysfx_max_sliders; ++i)
        slider_count += fx->source.main->header.sliders[i].exists;

    state->sliders = new ysfx_state_slider_t[slider_count]{};
    state->slider_count = slider_count;

    for (uint32_t i = 0, j = 0; i < slider_count; ++i) {
        if (fx->source.main->header.sliders[i].exists) {
            state->sliders[j].index = i;
            state->sliders[j].value = *fx->var.slider[i];
            ++j;
        }
    }

    // save the serialization
    state->data_size = buffer.size();
    state->data = new uint8_t[state->data_size];
    memcpy(state->data, buffer.data(), state->data_size);

    //
    return state.release();
}

void ysfx_state_free(ysfx_state_t *state)
{
    if (!state)
        return;

    delete[] state->sliders;
    delete[] state->data;
    delete state;
}

ysfx_state_t *ysfx_state_dup(ysfx_state_t *state_in)
{
    if (!state_in)
        return nullptr;

    ysfx_state_u state_out{new ysfx_state_t};

    uint32_t slider_count = state_out->slider_count = state_in->slider_count;
    size_t data_size = state_out->data_size = state_in->data_size;

    state_out->sliders = new ysfx_state_slider_t[slider_count];
    memcpy(state_out->sliders, state_in->sliders, slider_count * sizeof(ysfx_state_slider_t));

    state_out->data = new uint8_t[data_size];
    memcpy(state_out->data, state_in->data, data_size);

    return state_out.release();
}

void ysfx_serialize(ysfx_t *fx)
{
    if (fx->code.serialize) {
        if (fx->must_compute_init)
            ysfx_init(fx);
        NSEEL_code_execute(fx->code.serialize.get());
    }
}

uint32_t ysfx_get_slider_of_var(ysfx_t *fx, EEL_F *var)
{
    auto it = fx->slider_of_var.find(var);
    if (it == fx->slider_of_var.end())
        return ~(uint32_t)0;
    return it->second;
}

const char *ysfx_get_bank_path(ysfx_t *fx)
{
    return fx->source.bank_path.c_str();
}

void ysfx_enum_vars(ysfx_t *fx, ysfx_enum_vars_callback_t *callback, void *userdata)
{
    NSEEL_VM_enumallvars(fx->vm.get(), callback, userdata);
}

ysfx_real *ysfx_find_var(ysfx_t *fx, const char *name)
{
    struct find_data {
        ysfx_real *var = nullptr;
        const char *name = nullptr;
    };
    find_data fd;
    fd.name = name;
    auto callback = [](const char *name, EEL_F *var, void *userdata) -> int {
        find_data *fd = (find_data *)userdata;
        if (strcmp(name, fd->name) != 0)
            return 1;
        fd->var = var;
        return 0;
    };
    NSEEL_VM_enumallvars(fx->vm.get(), +callback, &fd);
    return fd.var;
}

void ysfx_read_vmem(ysfx_t *fx, uint32_t addr, ysfx_real *dest, uint32_t count)
{
    ysfx_eel_ram_reader reader(fx->vm.get(), addr);
    for (uint32_t i = 0; i < count; ++i)
        dest[i] = reader.read_next();
}

bool ysfx_find_data_file(ysfx_t *fx, EEL_F *file, std::string &result)
{
    // 3 possibilities for file
    // - slider
    // - index of filename
    // - string

    std::string filepart;

    bool accept_absolute = false;
    bool accept_relative = false;

    int32_t index = ysfx_eel_round<int32_t>(*file);
    uint32_t slideridx = ysfx_get_slider_of_var(fx, file);
    ysfx_slider_t *slider = nullptr;

    if (slideridx != ~(uint32_t)0)
        slider = &fx->source.main->header.sliders[slideridx];

    if (slider && !slider->path.empty()) {
        int32_t value = ysfx_eel_round<int32_t>(*fx->var.slider[slideridx]);
        if (value < 0 || (uint32_t)value >= slider->enum_names.size())
            return false;

        filepart = slider->path + '/' + slider->enum_names[(uint32_t)value];
        accept_relative = true;
    }
    else if (index >= 0 && (uint32_t)index < fx->source.main->header.filenames.size()) {
        filepart = fx->source.main->header.filenames[(uint32_t)index];
        accept_relative = true;
    }
    else if (ysfx_string_get(fx, *file, filepart)) {
        accept_absolute = true;
        accept_relative = true;
    }
    else
        return false;

    std::vector<std::string> filecandidates;
    filecandidates.reserve(2);

    if (accept_absolute && !ysfx::path_is_relative(filepart.c_str()))
        filecandidates.push_back(filepart);
    else if (accept_relative) {
        filecandidates.push_back(ysfx::path_directory(fx->source.main_file_path.c_str()) + filepart);
        if (!fx->config->data_root.empty())
            filecandidates.push_back(fx->config->data_root + filepart);
    }

    for (const std::string &filepath : filecandidates) {
        if (ysfx::exists(filepath.c_str())) {
            result.assign(filepath);
            return true;
        }
    }

    return false;
}

ysfx_file_type_t ysfx_detect_file_type(ysfx_t *fx, const char *path, void **fmtobj)
{
    if (ysfx::path_has_suffix(path, "txt"))
        return ysfx_file_type_txt;
    if (ysfx::path_has_suffix(path, "raw"))
        return ysfx_file_type_raw;
    for (ysfx_audio_format_t &fmt : fx->config->audio_formats) {
        if (fmt.can_handle(path)) {
            if (fmtobj)
                *fmtobj = &fmt;
            return ysfx_file_type_audio;
        }
    }
    return ysfx_file_type_none;
}

void ysfx_gfx_setup(ysfx_t *fx, ysfx_gfx_config_t *gc)
{
#if !defined(YSFX_NO_GFX)
    bool doinit = false;
    ysfx_scoped_gfx_t scope{fx, doinit};

    ysfx_gfx_state_set_bitmap(fx->gfx.state.get(), gc->pixels, gc->pixel_width, gc->pixel_height, gc->pixel_stride);
    ysfx_real scale = fx->gfx.wants_retina ? gc->scale_factor : 1;
    ysfx_gfx_state_set_scale_factor(fx->gfx.state.get(), scale);

    ysfx_gfx_state_set_callback_data(fx->gfx.state.get(), gc->user_data);
    ysfx_gfx_state_set_show_menu_callback(fx->gfx.state.get(), gc->show_menu);
    ysfx_gfx_state_set_set_cursor_callback(fx->gfx.state.get(), gc->set_cursor);
    ysfx_gfx_state_set_get_drop_file_callback(fx->gfx.state.get(), gc->get_drop_file);
#else
    (void)fx;
    (void)gc;
#endif
}

bool ysfx_gfx_wants_retina(ysfx_t *fx)
{
#if !defined(YSFX_NO_GFX)
    return fx->gfx.wants_retina;
#else
    (void)fx;
    return false;
#endif
}

void ysfx_gfx_add_key(ysfx_t *fx, uint32_t mods, uint32_t key, bool press)
{
#if !defined(YSFX_NO_GFX)
    bool doinit = true;
    ysfx_scoped_gfx_t scope{fx, doinit};

    if (!fx->gfx.ready)
        return;

    ysfx_gfx_state_add_key(fx->gfx.state.get(), mods, key, press);
#else
    (void)fx;
    (void)mods;
    (void)key;
#endif
}

void ysfx_gfx_update_mouse(ysfx_t *fx, uint32_t mods, int32_t xpos, int32_t ypos, uint32_t buttons, ysfx_real wheel, ysfx_real hwheel)
{
#if !defined(YSFX_NO_GFX)
    bool doinit = true;
    ysfx_scoped_gfx_t scope{fx, doinit};

    if (!fx->gfx.ready)
        return;

    *fx->var.mouse_x = (EEL_F)xpos;
    *fx->var.mouse_y = (EEL_F)ypos;
    *fx->var.mouse_wheel += 120 * wheel;
    *fx->var.mouse_hwheel += 120 * hwheel;

    uint32_t mouse_cap = 0;
    if (mods & ysfx_mod_shift)
        mouse_cap |= 8;
    if (mods & ysfx_mod_ctrl)
        mouse_cap |= 4;
    if (mods & ysfx_mod_alt)
        mouse_cap |= 16;
    if (mods & ysfx_mod_super)
        mouse_cap |= 32;
    if (buttons & ysfx_button_left)
        mouse_cap |= 1;
    if (buttons & ysfx_button_middle)
        mouse_cap |= 64;
    if (buttons & ysfx_button_right)
        mouse_cap |= 2;
    *fx->var.mouse_cap = (EEL_F)mouse_cap;

#else
    (void)fx;
    (void)mods;
    (void)xpos;
    (void)ypos;
    (void)buttons;
    (void)vwheel;
    (void)hwheel;
#endif
}

bool ysfx_gfx_run(ysfx_t *fx)
{
#if !defined(YSFX_NO_GFX)
    bool doinit = true;
    ysfx_scoped_gfx_t scope{fx, doinit};

    if (!fx->gfx.ready)
        return false;

    ysfx_gfx_prepare(fx);
    NSEEL_code_execute(fx->code.gfx.get());

    return ysfx_gfx_state_is_dirty(fx->gfx.state.get());
#else
    (void)fx;
#endif
}
