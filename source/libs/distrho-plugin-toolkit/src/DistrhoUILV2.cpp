/*
 * DISTHRO Plugin Toolkit (DPT)
 * Copyright (C) 2012 Filipe Coelho <falktx@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * For a full copy of the license see the GPL.txt file
 */

#include "DistrhoDefines.h"

#if defined(DISTRHO_PLUGIN_TARGET_LV2) && DISTRHO_PLUGIN_HAS_UI

#include "DistrhoUIInternal.h"

#include "lv2-sdk/lv2.h"
//#include "lv2-sdk/atom.h"
//#include "lv2-sdk/atom-util.h"
//#include "lv2-sdk/midi.h"
#include "lv2-sdk/patch.h"
#include "lv2-sdk/programs.h"
//#include "lv2-sdk/state.h"
#include "lv2-sdk/urid.h"
#include "lv2-sdk/ui.h"

#include <cassert>
#include <cstring>
#include <thread>

#ifdef DISTRHO_UI_QT4
# include <QtCore/QTimerEvent>
#endif

#ifndef DISTRHO_PLUGIN_URI
# error DISTRHO_PLUGIN_URI undefined!
#endif

#define DISTRHO_LV2UI_USE_EXTENSION_DATA (DISTRHO_PLUGIN_WANT_PROGRAMS)

// -------------------------------------------------

START_NAMESPACE_DISTRHO

#ifdef DISTRHO_UI_QT4
class UILv2 : public QObject
#else
class UILv2
#endif
{
public:
    UILv2(intptr_t parent, LV2UI_Controller controller, LV2UI_Write_Function writeFunction, LV2UI_Widget* widget, const LV2_Feature* const* features)
        : ui(this, parent, setParameterCallback, setStateCallback, uiNoteCallback, uiResizeCallback),
          lv2Controller(controller),
          lv2WriteFunction(writeFunction),
          lv2UiResize(nullptr),
#if DISTRHO_PLUGIN_WANT_STATE
          uridIdPatchMessage(0),
#endif
#ifdef DISTRHO_UI_QT4
          uiTimer(0)
#else
          threadExitNow(false),
          threadRunning(false),
          thread(uiThreadCallback, this)
#endif
    {
        // Get Features
        for (uint32_t i = 0; features[i]; i++)
        {
            if (strcmp(features[i]->URI, LV2_URID__map) == 0)
            {
#if DISTRHO_PLUGIN_WANT_STATE
                LV2_URID_Map* uridMap = (LV2_URID_Map*)features[i]->data;
                uridIdPatchMessage = uridMap->map(uridMap->handle, LV2_PATCH__Message);
#endif
            }

            else if (strcmp(features[i]->URI, LV2_UI__resize) == 0)
                lv2UiResize = (LV2UI_Resize*)features[i]->data;
        }

#ifndef DISTRHO_UI_QT4
        assert(parent);

        if (parent == 0)
            return;
#endif

        if (lv2UiResize)
            lv2UiResize->ui_resize(lv2UiResize->handle, ui.getWidth(), ui.getHeight());

#ifdef DISTRHO_UI_QT4
        uiTimer = startTimer(30);
#endif

        *widget = (void*)ui.getWindowId();
    }

    ~UILv2()
    {
#ifdef DISTRHO_UI_QT4
        if (uiTimer)
            killTimer(uiTimer);
#else
        uiThreadClose();
#endif
    }

    // ---------------------------------------------

    void lv2ui_port_event(uint32_t portIndex, uint32_t bufferSize, uint32_t format, const void* buffer)
    {
        if (format == 0)
        {
            if (bufferSize != sizeof(float))
                return;
            if (int32_t(portIndex - DISTRHO_PLUGIN_NUM_INPUTS - DISTRHO_PLUGIN_NUM_OUTPUTS) < 0)
                return;

            float value = *(float*)buffer;
            ui.parameterChanged(portIndex - DISTRHO_PLUGIN_NUM_INPUTS - DISTRHO_PLUGIN_NUM_OUTPUTS, value);
        }
        // TODO - atom events
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void lv2ui_select_program(uint32_t bank, uint32_t program)
    {
        const uint32_t index = bank * 128 + program;
        ui.programChanged(index);
    }
#endif

    // ---------------------------------------------

protected:
    void setParameterValue(uint32_t index, float value)
    {
        if (lv2WriteFunction)
            lv2WriteFunction(lv2Controller, DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS + index, sizeof(float), 0, &value);
    }

    void setState(const char* key, const char* value)
    {
        // TODO
        (void)key;
        (void)value;

        //if (lv2WriteFunction && uridMap)
        {

        }
    }

    void uiResize(unsigned int width, unsigned int height)
    {
        if (lv2UiResize)
            lv2UiResize->ui_resize(lv2UiResize->handle, width, height);
    }

#ifdef DISTRHO_UI_QT4
    void timerEvent(QTimerEvent* event)
    {
        if (event->timerId() == uiTimer)
            ui.idle();

        QObject::timerEvent(event);
    }
#else
    void uiThreadRun()
    {
        threadRunning = true;

        while (! threadExitNow)
        {
            ui.idle();
            d_msleep(1000 / 25); // 25 FPS
        }

        thread.detach();
        threadRunning = false;
    }

    void uiThreadClose()
    {
        threadExitNow = true;

        while (threadRunning)
            d_msleep(1000 / 25); // 25 FPS
    }
#endif

private:
    UIInternal ui;

    // LV2 Stuff
    LV2UI_Controller     const lv2Controller;
    LV2UI_Write_Function const lv2WriteFunction;
    const LV2UI_Resize*        lv2UiResize;

#if DISTRHO_PLUGIN_WANT_STATE
    LV2_URID uridIdPatchMessage;
#endif

#ifdef DISTRHO_UI_QT4
    int uiTimer;
#else
    // UI Thread
    bool threadExitNow;
    bool threadRunning;
    std::thread thread;
#endif

    // ---------------------------------------------
    // Callbacks

    static void setParameterCallback(void* ptr, uint32_t index, float value)
    {
        UILv2* _this_ = (UILv2*)ptr;
        assert(_this_);

        _this_->setParameterValue(index, value);
    }

    static void setStateCallback(void* ptr, const char* key, const char* value)
    {
#if DISTRHO_PLUGIN_WANT_STATE
        UILv2* _this_ = (UILv2*)ptr;
        assert(_this_);

        _this_->setState(key, value);
#else
        // unused
        (void)ptr;
        (void)key;
        (void)value;
#endif
    }

    static void uiNoteCallback(void* ptr, bool onOff, uint8_t channel, uint8_t note, uint8_t velocity)
    {
#if DISTRHO_PLUGIN_IS_SYNTH
        UILv2* _this_ = (UILv2*)ptr;
        assert(_this_);

        _this_->uiNote(onOff, channel, note, velocity);
#else
        // unused
        (void)ptr;
        (void)onOff;
        (void)channel;
        (void)note;
        (void)velocity;
#endif
    }

    static void uiResizeCallback(void* ptr, unsigned int width, unsigned int height)
    {
        UILv2* _this_ = (UILv2*)ptr;
        assert(_this_);

        _this_->uiResize(width, height);
    }

#ifndef DISTRHO_UI_QT4
    static void uiThreadCallback(void* ptr)
    {
        UILv2* _this_ = (UILv2*)ptr;
        assert(_this_);

        _this_->uiThreadRun();
    }
#endif
};

// -------------------------------------------------

static LV2UI_Handle lv2ui_instantiate(const LV2UI_Descriptor*, const char* uri, const char*, LV2UI_Write_Function writeFunction, LV2UI_Controller controller, LV2UI_Widget* widget, const LV2_Feature* const* features)
{
    if (strcmp(uri, DISTRHO_PLUGIN_URI) != 0)
        return nullptr;

    // Get parent
    intptr_t parent = 0;

    for (uint32_t i = 0; features[i]; i++)
    {
        if (strcmp(features[i]->URI, LV2_UI__parent) == 0)
        {
            parent = (intptr_t)features[i]->data;
            break;
        }
    }

    return new UILv2(parent, controller, writeFunction, widget, features);
}

static void lv2ui_cleanup(LV2UI_Handle instance)
{
    UILv2* ui = (UILv2*)instance;
    assert(ui);

    delete ui;
}

static void lv2ui_port_event(LV2UI_Handle instance, uint32_t portIndex, uint32_t bufferSize, uint32_t format, const void* buffer)
{
    UILv2* ui = (UILv2*)instance;
    assert(ui);

    ui->lv2ui_port_event(portIndex, bufferSize, format, buffer);
}

#if DISTRHO_LV2UI_USE_EXTENSION_DATA
# if DISTRHO_PLUGIN_WANT_PROGRAMS
static void lv2ui_select_program(LV2_Handle instance, uint32_t bank, uint32_t program)
{
    UILv2* ui = (UILv2*)instance;
    assert(ui);

    ui->lv2ui_select_program(bank, program);
}
# endif

static const void* lv2ui_extension_data(const char* uri)
{
# if DISTRHO_PLUGIN_WANT_PROGRAMS
    static const LV2_Programs_UI_Interface programs = { lv2ui_select_program };
    if (strcmp(uri, LV2_PROGRAMS__UIInterface) == 0)
        return &programs;
# endif

    return nullptr;
}
#endif

// -------------------------------------------------

static LV2UI_Descriptor uidescriptor = {
    DISTRHO_UI_URI,
    lv2ui_instantiate,
    lv2ui_cleanup,
    lv2ui_port_event,
#if DISTRHO_LV2UI_USE_EXTENSION_DATA
    lv2ui_extension_data
#else
    /* extension_data */ nullptr
#endif
};

END_NAMESPACE_DISTRHO

// -------------------------------------------------

DISTRHO_PLUGIN_EXPORT
const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
    USE_NAMESPACE_DISTRHO
    return (index == 0) ? &uidescriptor : nullptr;
}

// -------------------------------------------------

#endif // DISTRHO_PLUGIN_TARGET_LV2 && DISTRHO_PLUGIN_HAS_UI
