/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "DistrhoUIInternal.hpp"

#include "lv2/atom.h"
#include "lv2/atom-util.h"
#include "lv2/options.h"
#include "lv2/ui.h"
#include "lv2/urid.h"
#include "lv2/lv2_programs.h"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

class UiLv2
{
public:
    UiLv2(const intptr_t winId, const LV2_URID_Map* const uridMap, const LV2UI_Resize* const uiResz, const LV2UI_Touch* uiTouch, const LV2UI_Controller controller, const LV2UI_Write_Function writeFunc)
        : fUI(this, winId, editParameterCallback, setParameterCallback, setStateCallback, sendNoteCallback, uiResizeCallback),
          fUridMap(uridMap),
          fUiResize(uiResz),
          fUiTouch(uiTouch),
          fController(controller),
          fWriteFunction(writeFunc)
    {
        fUiResize->ui_resize(fUiResize->handle, fUI.getWidth(), fUI.getHeight());

        setState("MyKey1", "MyValue1");
        setState("My Key 2", "My Value 2");
    }

    // -------------------------------------------------------------------

    void lv2ui_port_event(const uint32_t rindex, const uint32_t bufferSize, const uint32_t format, const void* const buffer)
    {
        if (format == 0)
        {
            const uint32_t parameterOffset(fUI.getParameterOffset());

            if (rindex < parameterOffset)
                return;
            if (bufferSize != sizeof(float))
                return;

            const float value(*(const float*)buffer);
            fUI.parameterChanged(rindex-parameterOffset, value);

            d_stderr("port changed %i, %i, %f", rindex, parameterOffset, value);
        }
        else
        {
            //fUI.stateChanged(key, value);
        }
    }

    // -------------------------------------------------------------------

    int lv2ui_idle()
    {
        fUI.idle();
        return 0;
    }

    // -------------------------------------------------------------------

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void lv2ui_select_program(const uint32_t bank, const uint32_t program)
    {
        const uint32_t realProgram(bank * 128 + program);

        fUI.programChanged(realProgram);
    }
#endif

    // -------------------------------------------------------------------

protected:
    void editParameterValue(const uint32_t rindex, const bool started)
    {
        if (fUiTouch != nullptr && fUiTouch->touch != nullptr)
            fUiTouch->touch(fUiTouch->handle, rindex, started);
    }

    void setParameterValue(const uint32_t rindex, const float value)
    {
        if (fWriteFunction != nullptr)
            fWriteFunction(fController, rindex, sizeof(float), 0, &value);
    }

    void setState(const char* const key, const char* const value)
    {
        if (fWriteFunction == nullptr)
            return;

        const uint32_t eventInPortIndex(DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS);

        // join key and value
        std::string tmpStr;
        tmpStr += key;
        tmpStr += "\xff";
        tmpStr += value;

        // get size
        const size_t msgSize(tmpStr.size()+1);

        // convert into char[]
        char msg[msgSize];
        std::memcpy(msg, tmpStr.c_str(), msgSize-1);

        // set proper null chars
        msg[std::strlen(key)] = '\0';
        msg[msgSize]          = '\0';

        // reserve atom space
        const size_t atomSize(lv2_atom_pad_size(sizeof(LV2_Atom) + msgSize));
        char         atomBuf[atomSize];
        std::memset(atomBuf, 0, atomSize);

        // set atom info
        LV2_Atom* const atom((LV2_Atom*)atomBuf);
        atom->size = msgSize;
        atom->type = fUridMap->map(fUridMap->handle, "urn:distrho:keyValueState");

        // set atom data
        std::memcpy(atomBuf + sizeof(LV2_Atom), msg, msgSize);

        // send to DSP side
        fWriteFunction(fController, eventInPortIndex, atomSize, fUridMap->map(fUridMap->handle, LV2_ATOM__eventTransfer), atom);
    }

    void sendNote(const uint8_t /*channel*/, const uint8_t /*note*/, const uint8_t /*velocity*/)
    {
    }

    void uiResize(const unsigned int width, const unsigned int height)
    {
        fUI.setSize(width, height);
        fUiResize->ui_resize(fUiResize->handle, width, height);
    }

private:
    UIExporter fUI;

    // LV2 features
    const LV2_URID_Map* const fUridMap;
    const LV2UI_Resize* const fUiResize;
    const LV2UI_Touch*  const fUiTouch;

    // LV2 UI stuff
    const LV2UI_Controller     fController;
    const LV2UI_Write_Function fWriteFunction;

    // -------------------------------------------------------------------
    // Callbacks

    #define uiPtr ((UiLv2*)ptr)

    static void editParameterCallback(void* ptr, uint32_t rindex, bool started)
    {
        uiPtr->editParameterValue(rindex, started);
    }

    static void setParameterCallback(void* ptr, uint32_t rindex, float value)
    {
        uiPtr->setParameterValue(rindex, value);
    }

    static void setStateCallback(void* ptr, const char* key, const char* value)
    {
        uiPtr->setState(key, value);
    }

    static void sendNoteCallback(void* ptr, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        uiPtr->sendNote(channel, note, velocity);
    }

    static void uiResizeCallback(void* ptr, unsigned int width, unsigned int height)
    {
        uiPtr->uiResize(width, height);
    }

    #undef uiPtr
};

// -----------------------------------------------------------------------

static LV2UI_Handle lv2ui_instantiate(const LV2UI_Descriptor*, const char* uri, const char*, LV2UI_Write_Function writeFunction, LV2UI_Controller controller, LV2UI_Widget* widget, const LV2_Feature* const* features)
{
    if (uri == nullptr || std::strcmp(uri, DISTRHO_PLUGIN_URI) != 0)
    {
        d_stderr("Invalid plugin URI");
        return nullptr;
    }

    const LV2_Options_Option* options = nullptr;
    const LV2_URID_Map*       uridMap = nullptr;
    const LV2UI_Resize*      uiResize = nullptr;
    const LV2UI_Touch*       uiTouch  = nullptr;
    void*                    parentId = nullptr;

    for (int i=0; features[i] != nullptr; ++i)
    {
        if (std::strcmp(features[i]->URI, LV2_OPTIONS__options) == 0)
            options = (const LV2_Options_Option*)features[i]->data;
        else if (std::strcmp(features[i]->URI, LV2_URID__map) == 0)
            uridMap = (const LV2_URID_Map*)features[i]->data;
        else if (std::strcmp(features[i]->URI, LV2_UI__resize) == 0)
            uiResize = (const LV2UI_Resize*)features[i]->data;
        else if (std::strcmp(features[i]->URI, LV2_UI__parent) == 0)
            parentId = features[i]->data;
    }

    if (options == nullptr)
    {
        d_stderr("Options feature missing, cannot continue!");
        return nullptr;
    }

    if (uridMap == nullptr)
    {
        d_stderr("URID Map feature missing, cannot continue!");
        return nullptr;
    }

    if (uiResize == nullptr)
    {
        d_stderr("UI Resize feature missing, cannot continue!");
        return nullptr;
    }

    if (parentId == nullptr)
    {
        d_stderr("Parent Window Id missing, cannot continue!");
        return nullptr;
    }

    *widget = parentId;

    const intptr_t winId(*((intptr_t*)&parentId));

    for (int i=0; options[i].key != 0; ++i)
    {
        if (options[i].key == uridMap->map(uridMap->handle, LV2_CORE__sampleRate))
        {
            if (options[i].type == uridMap->map(uridMap->handle, LV2_ATOM__Double))
                d_lastUiSampleRate = *(const double*)options[i].value;
            else
                d_stderr("Host provides sampleRate but has wrong value type");

            break;
        }
    }

    if (d_lastUiSampleRate == 0.0)
        d_lastUiSampleRate = 44100.0;

    return new UiLv2(winId, uridMap, uiResize, uiTouch, controller, writeFunction);
}

#define uiPtr ((UiLv2*)ui)

static void lv2ui_cleanup(LV2UI_Handle ui)
{
    delete uiPtr;
}

static void lv2ui_port_event(LV2UI_Handle ui, uint32_t portIndex, uint32_t bufferSize, uint32_t format, const void* buffer)
{
    uiPtr->lv2ui_port_event(portIndex, bufferSize, format, buffer);
}

// -----------------------------------------------------------------------

static int lv2ui_idle(LV2UI_Handle ui)
{
    return uiPtr->lv2ui_idle();
}

#if DISTRHO_PLUGIN_WANT_PROGRAMS
static void lv2ui_select_program(LV2UI_Handle ui, uint32_t bank, uint32_t program)
{
    uiPtr->lv2ui_select_program(bank, program);
}
#endif

// -----------------------------------------------------------------------

static const void* lv2ui_extension_data(const char* uri)
{
    static const LV2UI_Idle_Interface uiIdle = { lv2ui_idle };

    if (std::strcmp(uri, LV2_UI__idleInterface) == 0)
        return &uiIdle;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    static const LV2_Programs_UI_Interface uiPrograms = { lv2ui_select_program };

    if (std::strcmp(uri, LV2_PROGRAMS__UIInterface) == 0)
        return &uiPrograms;
#endif

    return nullptr;
}

#undef instancePtr

// -----------------------------------------------------------------------

static const LV2UI_Descriptor sLv2UiDescriptor = {
    DISTRHO_UI_URI,
    lv2ui_instantiate,
    lv2ui_cleanup,
    lv2ui_port_event,
    lv2ui_extension_data
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

DISTRHO_PLUGIN_EXPORT
const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
    USE_NAMESPACE_DISTRHO
    return (index == 0) ? &sLv2UiDescriptor : nullptr;
}

// -----------------------------------------------------------------------
