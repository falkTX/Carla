/*
 * Carla Bridge UI
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

#ifndef CARLA_BRIDGE_UI_HPP_INCLUDED
#define CARLA_BRIDGE_UI_HPP_INCLUDED

#include "CarlaBridgeToolkit.hpp"
#include "CarlaLibUtils.hpp"
#include "CarlaPipeUtils.hpp"

#ifdef BRIDGE_LV2
# include "lv2/atom.h"
# include "lv2/urid.h"
#endif

//#include "CarlaBridgeToolkit.hpp"
//#include "CarlaLibUtils.hpp"

CARLA_BRIDGE_START_NAMESPACE

/*!
 * @defgroup CarlaBridgeUIAPI Carla UI Bridge API
 *
 * The Carla UI Bridge API.
 * @{
 */

// -----------------------------------------------------------------------

class CarlaBridgeUI : public CarlaPipeClient
{
protected:
    /*!
     * Constructor.
     */
    CarlaBridgeUI() noexcept;

    /*!
     * Destructor.
     */
    virtual ~CarlaBridgeUI() /*noexcept*/;

    // ---------------------------------------------------------------------

    bool libOpen(const char* const filename) noexcept;
    void* libSymbol(const char* const symbol) const noexcept;
    const char* libError() const noexcept;

    // ---------------------------------------------------------------------
    // DSP Callbacks

    virtual void dspParameterChanged(const uint32_t index, const float value) = 0;
    virtual void dspProgramChanged(const uint32_t index) = 0;
    virtual void dspMidiProgramChanged(const uint32_t bank, const uint32_t program) = 0;
    virtual void dspStateChanged(const char* const key, const char* const value) = 0;
    virtual void dspNoteReceived(const bool onOff, const uint8_t channel, const uint8_t note, const uint8_t velocity) = 0;

#ifdef BRIDGE_LV2
    virtual void dspAtomReceived(const uint32_t index, const LV2_Atom* const atom) = 0;
    virtual void dspURIDReceived(const LV2_URID urid, const char* const uri) = 0;
#endif

public:
    // ---------------------------------------------------------------------
    // UI initialization

  virtual bool init(const int argc, const char* argv[]);
  virtual void exec(const bool showUI);
  virtual void idleUI() {}

    // ---------------------------------------------------------------------
    // UI management

    /*!
     * Get the widget associated with this UI.
     * This can be a Gtk widget, Qt widget or a native Window handle depending on the compile target.
     */
    virtual void* getWidget() const noexcept = 0;

    /*!
     * Options.
     */
    struct Options {
        /*!
         * UI is resizable by the user.
         * The UI can still sometimes resize itself internally if this is false.
         */
        bool isResizable;

        /*!
         * Use the Carla PRO theme if possible.
         */
        bool useTheme;

        /*!
         * Use the Carla PRO theme colors if possible.
         * This implies useTheme to be true.
         */
        bool useThemeColors;

        /*!
         * Window title.
         */
        const char* windowTitle;

        /*!
         * Transient window id (parent), null if zero.
         */
        uintptr_t transientWindowId;

        /*!
         * Constructor for default options.
         */
        Options() noexcept
            : isResizable(true),
              useTheme(true),
              useThemeColors(true),
              windowTitle(nullptr),
              transientWindowId(0) {}
    };

    /*!
     * Get options associated with this UI.
     */
    virtual const Options& getOptions() const noexcept = 0;

    // ---------------------------------------------------------------------

protected:
    bool fQuitReceived;
#ifdef BRIDGE_LV2
    bool fUridMapComplete;
#endif
    CarlaBridgeToolkit* fToolkit;

    lib_t fLib;
    CarlaString fLibFilename;

    /*! @internal */
    bool msgReceived(const char* const msg) noexcept override;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaBridgeUI)
};

/**@}*/

// -----------------------------------------------------------------------

CARLA_BRIDGE_END_NAMESPACE

#endif // CARLA_BRIDGE_UI_HPP_INCLUDED

// -----------------------------------------------------------------------

#if 0
    // ---------------------------------------------------------------------
    // ui initialization

    virtual bool uiInit(const char*, const char* const, const char* const);
    virtual void uiIdle() {}
    virtual void uiClose();

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

    // ---------------------------------------------------------------------

    const char* uiLibError();

    template<typename Func>
    Func uiLibSymbol(const char* const symbol)
    {
        CARLA_SAFE_ASSERT_RETURN(fUI.lib != nullptr, nullptr);
        carla_debug("CarlaBridgeUI::uiLibSymbol(\"%s\")", symbol);

        return lib_symbol<Func>(fUI.lib, symbol);
    }
#endif

#if 0
    struct UI {
        CarlaBridgeToolkit* const toolkit;
        CarlaString filename;
        lib_t lib;
        bool quit;

        UI(CarlaBridgeToolkit* const toolkit_)
            : toolkit(toolkit_),
              filename(),
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
        CARLA_DECLARE_NON_COPY_STRUCT(UI)
#endif
    } fUI;
#endif
