/*
 * Carla Utility Tests
 * Copyright (C) 2013-2017 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#ifdef NDEBUG
# error Build this file with debug ON please
#endif

#include "CarlaLadspaUtils.hpp"
#include "CarlaDssiUtils.cpp"
#include "CarlaLv2Utils.hpp"
#include "CarlaVstUtils.hpp"

// -----------------------------------------------------------------------

static void test_CarlaLadspaUtils()
{
    LADSPA_Descriptor desc;
    carla_zeroStruct(desc);

    LADSPA_RDF_Descriptor rdfDesc;
    delete ladspa_rdf_dup(&rdfDesc);

    is_ladspa_port_good(0x0, 0x0);
    is_ladspa_rdf_descriptor_valid(&rdfDesc, &desc);
    get_default_ladspa_port_value(0x0, -1.0f, 1.0f);
}

// -----------------------------------------------------------------------

static void test_CarlaDssiUtils() noexcept
{
    const char* const ui = find_dssi_ui("/usr/lib/dssi/trivial_sampler.so", "aa");

    CARLA_SAFE_ASSERT(ui != nullptr);

    if (ui != nullptr)
    {
        carla_stdout("%s", ui);
        assert(std::strcmp(ui, "/usr/lib/dssi/trivial_sampler/trivial_sampler_qt") == 0);
        delete[] ui;
    }
}

// -----------------------------------------------------------------------

static LV2_URID test_lv2_uridMap(LV2_URID_Map_Handle, const char*)
{
    return 1;
}

static void test_CarlaLv2Utils()
{
    Lv2WorldClass& lv2World(Lv2WorldClass::getInstance());
    lv2World.initIfNeeded(std::getenv("LV2_PATH"));

    // getPlugin
    const LilvPlugin* const plugin(lv2World.getPluginFromURI("urn:juced:DrumSynth"));
    CARLA_SAFE_ASSERT(plugin != nullptr);

    // getState
    LV2_URID_Map uridMap = { nullptr, test_lv2_uridMap };
    LilvState* const state(lv2World.getStateFromURI("http://arcticanaudio.com/plugins/thefunction#preset001", &uridMap));
    CARLA_SAFE_ASSERT(state != nullptr);
    if (state != nullptr) lilv_state_free(state);

    // load a bunch of plugins to stress test lilv
    delete lv2_rdf_new("http://arcticanaudio.com/plugins/thefunction", true);
    delete lv2_rdf_new("http://kunz.corrupt.ch/products/tal-noisemaker", true);
    delete lv2_rdf_new("http://calf.sourceforge.net/plugins/Reverb", true);
    delete lv2_rdf_new("http://www.openavproductions.com/fabla", true);
    delete lv2_rdf_new("http://invadarecords.com/plugins/lv2/meter", true);
    delete lv2_rdf_new("http://gareus.org/oss/lv2/meters#spectr30stereo", true);
    delete lv2_rdf_new("http://plugin.org.uk/swh-plugins/revdelay", true);
    delete lv2_rdf_new("http://lv2plug.in/plugins/eg-scope#Stereo", true);
    delete lv2_rdf_new("http://kxstudio.sf.net/carla/plugins/carlarack", true);
    delete lv2_rdf_new("http://guitarix.sourceforge.net/plugins/gxautowah#autowah", true);
    delete lv2_rdf_new("http://github.com/blablack/ams-lv2/mixer_4ch", true);
    delete lv2_rdf_new("http://drumgizmo.org/lv2", true);
    delete lv2_rdf_new("http://synthv1.sourceforge.net/lv2", true);
    delete lv2_rdf_new("urn:juced:DrumSynth", true);

    // misc
    is_lv2_port_supported(0x0);
    is_lv2_feature_supported("test1");
    is_lv2_ui_feature_supported("test2");
}

// -----------------------------------------------------------------------

static intptr_t test_vst_dispatcher(AEffect*, int, int, intptr_t, void*, float)
{
    return 0;
}

static void test_CarlaVstUtils() noexcept
{
    AEffect effect;
    carla_zeroStruct(effect);
    effect.dispatcher = test_vst_dispatcher;

    vstPluginCanDo(&effect, "test");
    carla_stdout(vstEffectOpcode2str(effOpen));
    carla_stdout(vstMasterOpcode2str(audioMasterAutomate));
}

// -----------------------------------------------------------------------
// main

int main()
{
    test_CarlaLadspaUtils();
    test_CarlaDssiUtils();
    test_CarlaLv2Utils();
    test_CarlaVstUtils();

    return 0;
}

// -----------------------------------------------------------------------
