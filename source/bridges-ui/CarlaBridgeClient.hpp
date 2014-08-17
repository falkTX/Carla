/*
 * Carla Bridge Client
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_BRIDGE_CLIENT_HPP_INCLUDED
#define CARLA_BRIDGE_CLIENT_HPP_INCLUDED

#include "CarlaBridgeOsc.hpp"
#include "CarlaBridgeToolkit.hpp"

CARLA_BRIDGE_START_NAMESPACE

// -----------------------------------------------------------------------

class CarlaBridgeClient
{
public:
    CarlaBridgeClient(const char* const uiTitle);
    virtual ~CarlaBridgeClient();

    // ---------------------------------------------------------------------
    // ui initialization

    virtual bool uiInit(const char* const, const char* const);
    virtual void uiIdle() {}
    virtual void uiClose();

    // ---------------------------------------------------------------------
    // ui management

    virtual void* getWidget() const = 0;
    virtual bool isResizable() const = 0;

    // ---------------------------------------------------------------------
    // ui processing

    virtual void setParameter(const int32_t rindex, const float value) = 0;
    virtual void setProgram(const uint32_t index) = 0;
    virtual void setMidiProgram(const uint32_t bank, const uint32_t program) = 0;
    virtual void noteOn(const uint8_t channel, const uint8_t note, const uint8_t velo) = 0;
    virtual void noteOff(const uint8_t channel, const uint8_t note) = 0;

    // ---------------------------------------------------------------------
    // ui toolkit

    void toolkitShow();
    void toolkitHide();
    void toolkitResize(const int width, const int height);
    void toolkitExec(const bool showGui);
    void toolkitQuit();

    // ---------------------------------------------------------------------
    // osc stuff

    void oscInit(const char* const url);
    bool oscIdle(const bool onlyOnce = false) const;
    void oscClose();

    bool isOscControlRegistered() const noexcept;
    void sendOscUpdate() const;

    // ---------------------------------------------------------------------

protected:
    void sendOscConfigure(const char* const key, const char* const value) const;
    void sendOscControl(const int32_t index, const float value) const;
    void sendOscProgram(const uint32_t index) const;
    void sendOscMidiProgram(const uint32_t index) const;
    void sendOscMidi(const uint8_t midiBuf[4]) const;
    void sendOscExiting() const;

#ifdef BRIDGE_LV2
    void sendOscLv2AtomTransfer(const uint32_t portIndex, const char* const atomBuf) const;
    void sendOscLv2UridMap(const uint32_t urid, const char* const uri) const;
#endif

    // ---------------------------------------------------------------------

    void* getContainerId();
    void* getContainerId2();
    bool  uiLibOpen(const char* const filename);
    bool  uiLibClose();
    void* uiLibSymbol(const char* const symbol);
    const char* uiLibError();

    // ---------------------------------------------------------------------

private:
    CarlaBridgeOsc fOsc;
    const CarlaOscData& fOscData;

    struct UI {
        CarlaBridgeToolkit* const toolkit;
        CarlaString filename;
        void* lib;
        bool quit;

        UI(CarlaBridgeToolkit* const toolkit_)
            : toolkit(toolkit_),
              lib(nullptr),
              quit(false)
        {
            CARLA_ASSERT(toolkit != nullptr);
        }

        ~UI()
        {
            delete toolkit;
        }

        void init()
        {
            toolkit->init();
            quit = false;
        }

        void close()
        {
            quit = true;
            toolkit->quit();
        }

#ifdef CARLA_PROPER_CPP11_SUPPORT
        UI() = delete;
        UI(UI&) = delete;
        UI(const UI&) = delete;
#endif
    } fUI;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaBridgeClient)
};

// -----------------------------------------------------------------------

CARLA_BRIDGE_END_NAMESPACE

#endif // CARLA_BRIDGE_CLIENT_HPP_INCLUDED
