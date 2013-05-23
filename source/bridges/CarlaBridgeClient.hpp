/*
 * Carla Bridge Client
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#ifndef __CARLA_BRIDGE_CLIENT_HPP__
#define __CARLA_BRIDGE_CLIENT_HPP__

#include "CarlaBridgeOsc.hpp"
#include "CarlaBridgeToolkit.hpp"

CARLA_BRIDGE_START_NAMESPACE

#if 0
} // Fix editor indentation
#endif

/*!
 * @defgroup CarlaBridgeClient Carla Bridge Client
 *
 * The Carla Bridge Client.
 * @{
 */

class CarlaBridgeClient
{
public:
    CarlaBridgeClient(const char* const uiTitle);
    virtual ~CarlaBridgeClient();

#ifdef BUILD_BRIDGE_UI
    // ---------------------------------------------------------------------
    // ui initialization

    virtual bool uiInit(const char* const, const char* const);
    virtual void uiClose();

    // ---------------------------------------------------------------------
    // ui management

    virtual void* getWidget() const = 0;
    virtual bool isResizable() const = 0;
    virtual bool needsReparent() const = 0;
#endif

#ifdef BUILD_BRIDGE_PLUGIN
    // ---------------------------------------------------------------------
    // plugin management

    virtual void saveNow() = 0;
    virtual void setCustomData(const char* const type, const char* const key, const char* const value) = 0;
    virtual void setChunkData(const char* const filePath) = 0;
#endif

    // ---------------------------------------------------------------------
    // processing

    virtual void setParameter(const int32_t rindex, const float value) = 0;

#ifndef BUILD_BRIDGE_PLUGIN
    virtual void setProgram(const uint32_t index) = 0;
    virtual void setMidiProgram(const uint32_t bank, const uint32_t program) = 0;
    virtual void noteOn(const uint8_t channel, const uint8_t note, const uint8_t velo) = 0;
    virtual void noteOff(const uint8_t channel, const uint8_t note) = 0;
#endif

    // ---------------------------------------------------------------------
    // osc stuff

    void oscInit(const char* const url);
    bool oscIdle();
    void oscClose();

    bool isOscControlRegistered() const;
    void sendOscUpdate();

#ifdef BUILD_BRIDGE_PLUGIN
    void sendOscBridgeUpdate();
    void sendOscBridgeError(const char* const error);
#endif

#ifdef BUILD_BRIDGE_UI
    // ---------------------------------------------------------------------
    // toolkit

    void toolkitShow();
    void toolkitHide();
    void toolkitResize(const int width, const int height);
    void toolkitExec(const bool showGui);
    void toolkitQuit();
#endif

    // ---------------------------------------------------------------------

protected:
    void sendOscConfigure(const char* const key, const char* const value);
    void sendOscControl(const int32_t index, const float value);
    void sendOscProgram(const int32_t index);
    void sendOscMidiProgram(const int32_t index);
    void sendOscMidi(const uint8_t midiBuf[4]);
    void sendOscExiting();

#ifdef BRIDGE_LV2
    void sendOscLv2AtomTransfer(const int32_t portIndex, const char* const atomBuf);
    void sendOscLv2UridMap(const uint32_t urid, const char* const uri);
#endif

    // ---------------------------------------------------------------------

#ifdef BUILD_BRIDGE_UI
    void* getContainerId();
    bool  uiLibOpen(const char* const filename);
    bool  uiLibClose();
    void* uiLibSymbol(const char* const symbol);
    const char* uiLibError();
#endif

    // ---------------------------------------------------------------------

private:
    CarlaBridgeOsc kOsc;

#ifdef BUILD_BRIDGE_UI
    struct UI {
        CarlaBridgeToolkit* const toolkit;
        CarlaString filename;
        void* lib;
        bool quit;

        UI(CarlaBridgeToolkit* const toolkit_)
            : toolkit(toolkit_),
              lib(nullptr),
              quit(false) {}

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

    } fUI;
#else
    friend class CarlaPluginClient;
#endif

    const CarlaOscData* fOscData;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaBridgeClient)
};

/**@}*/

CARLA_BRIDGE_END_NAMESPACE

#endif // __CARLA_BRIDGE_CLIENT_HPP__
