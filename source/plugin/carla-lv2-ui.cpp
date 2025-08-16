// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef HAVE_PYQT
# error This file should not be built
#endif

#include "CarlaLv2Utils.hpp"
#include "CarlaPipeUtils.hpp"

#include "extra/ScopedSafeLocale.hpp"

// --------------------------------------------------------------------------------------------------------------------

class NativePluginUI : public LV2_External_UI_Widget_Compat
{
public:
    NativePluginUI(LV2UI_Write_Function writeFunction, LV2UI_Controller controller,
                   LV2UI_Widget* widget, const LV2_Feature* const* features)
        : fUridMap(nullptr),
          fUridUnmap(nullptr),
          fUridTranser(0),
          fUridTranser2(0),
          fUI()
    {
        run  = extui_run;
        show = extui_show;
        hide = extui_hide;

        fUI.writeFunction = writeFunction;
        fUI.controller = controller;

        const LV2_URID_Map*   uridMap   = nullptr;
        const LV2_URID_Unmap* uridUnmap = nullptr;

        for (int i=0; features[i] != nullptr; ++i)
        {
            /**/ if (std::strcmp(features[i]->URI, LV2_URID__map) == 0)
                uridMap = (const LV2_URID_Map*)features[i]->data;
            else if (std::strcmp(features[i]->URI, LV2_URID__unmap) == 0)
                uridUnmap = (const LV2_URID_Unmap*)features[i]->data;
        }

        if (uridMap == nullptr)
        {
            carla_stderr("Host doesn't provide urid-map feature");
            return;
        }

        fUridMap = uridMap;
        fUridUnmap = uridUnmap;
        fUridTranser = uridMap->map(uridMap->handle, LV2_ATOM__eventTransfer);
        fUridTranser2 = uridMap->map(uridMap->handle, "urn:carla:transmitEv");

        // ---------------------------------------------------------------
        // see if the host supports external-ui

        for (int i=0; features[i] != nullptr; ++i)
        {
            if (std::strcmp(features[i]->URI, LV2_EXTERNAL_UI__Host) == 0 ||
                std::strcmp(features[i]->URI, LV2_EXTERNAL_UI_DEPRECATED_URI) == 0)
            {
                fUI.host = (const LV2_External_UI_Host*)features[i]->data;
                break;
            }
        }

        if (fUI.host != nullptr)
        {
            fUI.name = carla_strdup(fUI.host->plugin_human_id);
            *widget = (LV2_External_UI_Widget_Compat*)this;
            return;
        }

        // ---------------------------------------------------------------
        // no external-ui support, use showInterface

        for (int i=0; features[i] != nullptr; ++i)
        {
            if (std::strcmp(features[i]->URI, LV2_OPTIONS__options) != 0)
                continue;

            const LV2_Options_Option* const options((const LV2_Options_Option*)features[i]->data);
            CARLA_SAFE_ASSERT_BREAK(options != nullptr);

            for (int j=0; options[j].key != 0; ++j)
            {
                if (options[j].key != uridMap->map(uridMap->handle, LV2_UI__windowTitle))
                    continue;

                const char* const title((const char*)options[j].value);
                CARLA_SAFE_ASSERT_BREAK(title != nullptr && title[0] != '\0');

                fUI.name = carla_strdup(title);
                break;
            }
            break;
        }

        if (fUI.name == nullptr)
            fUI.name = carla_strdup("Carla");

        *widget = nullptr;
    }

    ~NativePluginUI()
    {
        if (fUI.isVisible)
            writeAtomMessage("quit");

        fUI.host = nullptr;
        fUI.writeFunction = nullptr;
        fUI.controller = nullptr;
    }

    // ----------------------------------------------------------------------------------------------------------------

    void lv2ui_port_event(uint32_t portIndex, uint32_t bufferSize, uint32_t format, const void* buffer)
    {
        CARLA_SAFE_ASSERT_RETURN(buffer != nullptr,);

        if (format == 0)
        {
            char msg[128];
            const float* const valuePtr = (const float*)buffer;

            {
                const ScopedSafeLocale ssl;
                std::snprintf(msg, 127, "control %u %.12g", portIndex, static_cast<double>(*valuePtr));
            }

            msg[127] = '\0';

            writeAtomMessage(msg);
            return;
        }

        if (format == fUridTranser)
        {
            CARLA_SAFE_ASSERT_RETURN(bufferSize > sizeof(LV2_Atom),);

            const LV2_Atom* const atom = (const LV2_Atom*)buffer;

            if (atom->type == fUridTranser2)
            {
                const char* const msg = (const char*)(atom + 1);

                if (std::strcmp(msg, "quit") == 0)
                {
                    handleUiClosed();
                }

                return;
            }
        }

        if (fUridUnmap != nullptr)
        {
            carla_stdout("lv2ui_port_event %u %u %u:%s %p",
                         portIndex, bufferSize, format, fUridUnmap->unmap(fUridUnmap->handle, format), buffer);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    void lv2ui_select_program(uint32_t bank, uint32_t program) const
    {
        char msg[128];
        std::snprintf(msg, 127, "program %u %u", bank, program);
        msg[127] = '\0';

        writeAtomMessage(msg);
    }

    // ----------------------------------------------------------------------------------------------------------------

    int lv2ui_idle() const
    {
        if (! fUI.isVisible)
            return 1;

        handleUiRun();
        return 0;
    }

    int lv2ui_show()
    {
        handleUiShow();
        return 0;
    }

    int lv2ui_hide()
    {
        handleUiHide();
        return 0;
    }

    // ----------------------------------------------------------------------------------------------------------------

protected:
    void handleUiShow()
    {
        writeAtomMessage("show");
        fUI.isVisible = true;
    }

    void handleUiHide()
    {
        if (fUI.isVisible)
        {
            fUI.isVisible = false;
            writeAtomMessage("hide");
        }
    }

    void handleUiRun() const
    {
        if (fUI.isVisible)
            writeAtomMessage("idle");
    }

    void handleUiClosed()
    {
        fUI.isVisible = false;

        if (fUI.host != nullptr && fUI.host->ui_closed != nullptr && fUI.controller != nullptr)
            fUI.host->ui_closed(fUI.controller);

        fUI.host = nullptr;
        fUI.writeFunction = nullptr;
        fUI.controller = nullptr;
    }

    bool writeAtomMessage(const char* const msg) const
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.writeFunction != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(fUridTranser != 0, false);
        CARLA_SAFE_ASSERT_RETURN(fUridTranser2 != 0, false);

#ifdef DEBUG
        if (std::strcmp(msg, "idle") != 0) {
            carla_debug("writeAtomMessage(%s)", msg);
        }
#endif

        const size_t msgSize  = std::strlen(msg)+1;
        const size_t atomSize = sizeof(LV2_Atom) + msgSize;

        if (atomSize <= 128)
        {
            char atomBuf[atomSize];
            carla_zeroChars(atomBuf, atomSize);

            LV2_Atom* const atom = (LV2_Atom*)atomBuf;
            atom->size = static_cast<uint32_t>(msgSize);
            atom->type = fUridTranser2;
            std::memcpy(atomBuf+sizeof(LV2_Atom), msg, msgSize);

            fUI.writeFunction(fUI.controller, 0, static_cast<uint32_t>(atomSize), fUridTranser, atomBuf);
        }
        else
        {
            char* const atomBuf = new char[atomSize];
            carla_zeroChars(atomBuf, atomSize);

            LV2_Atom* const atom = (LV2_Atom*)atomBuf;
            atom->size = static_cast<uint32_t>(msgSize);
            atom->type = fUridTranser2;
            std::memcpy(atomBuf+sizeof(LV2_Atom), msg, msgSize);

            fUI.writeFunction(fUI.controller, 0, static_cast<uint32_t>(atomSize), fUridTranser, atomBuf);

            delete[] atomBuf;
        }

        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------

private:
    const LV2_URID_Map* fUridMap;
    const LV2_URID_Unmap* fUridUnmap;
    LV2_URID fUridTranser, fUridTranser2;

    struct UI {
        const LV2_External_UI_Host* host;
        LV2UI_Write_Function writeFunction;
        LV2UI_Controller controller;
        const char* name;
        bool isVisible;

        UI()
            : host(nullptr),
              writeFunction(nullptr),
              controller(nullptr),
              name(nullptr),
              isVisible(false) {}

        ~UI()
        {
            if (name != nullptr)
            {
                delete[] name;
                name = nullptr;
            }
        }

        CARLA_DECLARE_NON_COPYABLE(UI);
    } fUI;

    // ----------------------------------------------------------------------------------------------------------------

    #define handlePtr ((NativePluginUI*)handle)

    static void extui_run(LV2_External_UI_Widget_Compat* handle)
    {
        handlePtr->handleUiRun();
    }

    static void extui_show(LV2_External_UI_Widget_Compat* handle)
    {
        carla_debug("extui_show(%p)", handle);
        handlePtr->handleUiShow();
    }

    static void extui_hide(LV2_External_UI_Widget_Compat* handle)
    {
        carla_debug("extui_hide(%p)", handle);
        handlePtr->handleUiHide();
    }

    #undef handlePtr

    // -------------------------------------------------------------------

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativePluginUI)
};

// -----------------------------------------------------------------------
// LV2 UI descriptor functions

static LV2UI_Handle lv2ui_instantiate(const LV2UI_Descriptor*, const char*, const char*,
                                      LV2UI_Write_Function writeFunction, LV2UI_Controller controller,
                                      LV2UI_Widget* widget, const LV2_Feature* const* features)
{
    carla_debug("lv2ui_instantiate(..., %p, %p, %p)", writeFunction, controller, widget, features);

    NativePluginUI* const ui = new NativePluginUI(writeFunction, controller, widget, features);

    // TODO: check ok

    return (LV2UI_Handle)ui;
}

#define uiPtr ((NativePluginUI*)ui)

static void lv2ui_port_event(LV2UI_Handle ui, uint32_t portIndex, uint32_t bufferSize, uint32_t format, const void* buffer)
{
    carla_debug("lv2ui_port_event(%p, %i, %i, %i, %p)", ui, portIndex, bufferSize, format, buffer);
    uiPtr->lv2ui_port_event(portIndex, bufferSize, format, buffer);
}

static void lv2ui_cleanup(LV2UI_Handle ui)
{
    carla_debug("lv2ui_cleanup(%p)", ui);
    delete uiPtr;
}

static void lv2ui_select_program(LV2UI_Handle ui, uint32_t bank, uint32_t program)
{
    carla_debug("lv2ui_select_program(%p, %i, %i)", ui, bank, program);
    uiPtr->lv2ui_select_program(bank, program);
}

static int lv2ui_idle(LV2UI_Handle ui)
{
    return uiPtr->lv2ui_idle();
}

static int lv2ui_show(LV2UI_Handle ui)
{
    carla_debug("lv2ui_show(%p)", ui);
    return uiPtr->lv2ui_show();
}

static int lv2ui_hide(LV2UI_Handle ui)
{
    carla_debug("lv2ui_hide(%p)", ui);
    return uiPtr->lv2ui_hide();
}

static const void* lv2ui_extension_data(const char* uri)
{
    carla_stdout("lv2ui_extension_data(\"%s\")", uri);

    static const LV2UI_Idle_Interface uiidle = { lv2ui_idle };
    static const LV2UI_Show_Interface uishow = { lv2ui_show, lv2ui_hide };
    static const LV2_Programs_UI_Interface uiprograms = { lv2ui_select_program };

    if (std::strcmp(uri, LV2_UI__idleInterface) == 0)
        return &uiidle;
    if (std::strcmp(uri, LV2_UI__showInterface) == 0)
        return &uishow;
    if (std::strcmp(uri, LV2_PROGRAMS__UIInterface) == 0)
        return &uiprograms;

    return nullptr;
}

#undef uiPtr

// -----------------------------------------------------------------------
// Startup code

CARLA_PLUGIN_EXPORT
const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
    carla_debug("lv2ui_descriptor(%i)", index);

    static const LV2UI_Descriptor lv2UiExtDesc = {
    /* URI            */ "http://kxstudio.sf.net/carla/ui-bridge-ext",
    /* instantiate    */ lv2ui_instantiate,
    /* cleanup        */ lv2ui_cleanup,
    /* port_event     */ lv2ui_port_event,
    /* extension_data */ lv2ui_extension_data
    };

    return (index == 0) ? &lv2UiExtDesc : nullptr;
}

// -----------------------------------------------------------------------

#include "CarlaPipeUtils.cpp"

// -----------------------------------------------------------------------
