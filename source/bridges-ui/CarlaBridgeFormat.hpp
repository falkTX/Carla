// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CARLA_BRIDGE_FORMAT_HPP_INCLUDED
#define CARLA_BRIDGE_FORMAT_HPP_INCLUDED

#include "CarlaBridgeUI.hpp"

#include "CarlaLibUtils.hpp"
#include "CarlaPipeUtils.hpp"

#include "extra/String.hpp"

#include "lv2/atom.h"
#include "lv2/urid.h"

#include <vector>

CARLA_BRIDGE_UI_START_NAMESPACE

/*!
 * @defgroup CarlaBridgeUIAPI Carla UI Bridge API
 *
 * The Carla UI Bridge API.
 * @{
 */

// -----------------------------------------------------------------------

class CarlaBridgeFormat : public CarlaPipeClient
{
protected:
    /*!
     * Constructor.
     */
    CarlaBridgeFormat() noexcept;

    /*!
     * Destructor.
     */
    virtual ~CarlaBridgeFormat() /*noexcept*/;

    // ---------------------------------------------------------------------

    bool libOpen(const char* filename) noexcept;
    void* libSymbol(const char* symbol) const noexcept;
    const char* libError() const noexcept;

    // ---------------------------------------------------------------------
    // DSP Callbacks

    virtual void dspParameterChanged(uint32_t index, float value) = 0;
    virtual void dspParameterChanged(const char* uri, float value) = 0;
    virtual void dspProgramChanged(uint32_t index) = 0;
    virtual void dspMidiProgramChanged(uint32_t bank, uint32_t program) = 0;
    virtual void dspStateChanged(const char* key, const char* value) = 0;
    virtual void dspNoteReceived(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity) = 0;

    virtual void dspAtomReceived(uint32_t index, const LV2_Atom* atom) = 0;
    virtual void dspURIDReceived(LV2_URID urid, const char* uri) = 0;

    struct BridgeFormatOptions {
        double sampleRate;
        uint32_t bgColor;
        uint32_t fgColor;
        float uiScale;
        bool isStandalone;
        bool useTheme;
        bool useThemeColors;
        const char* windowTitle;
        uintptr_t transientWindowId;
    };

    virtual void uiOptionsChanged(const BridgeFormatOptions& opts) = 0;

public:
    // ---------------------------------------------------------------------
    // UI initialization

    virtual bool init(int argc, const char* argv[]);
    virtual void exec(bool showUI);
    virtual void idleUI() {}

    // ---------------------------------------------------------------------
    // UI management

    /*!
     * Get the widget associated with this UI.
     * This can be a Gtk widget, Qt widget or a native Window handle depending on the compile target.
     */
    virtual void* getWidget() const noexcept = 0;

#ifndef CARLA_OS_MAC
    /*!
     * TESTING
     */
    virtual void setScaleFactor(double scaleFactor) = 0;
#endif

    /*!
     * TESTING
     */
    virtual void uiResized(uint width, uint height) = 0;

    /*!
     * Options.
     */
    struct Options {
        /*!
         * UI is standalone, not controlled by another application.
         */
        bool isStandalone;

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
        String windowTitle;

        /*!
         * Transient window id (parent), zero if unset.
         */
        uintptr_t transientWindowId;

        /*!
         * Constructor for default options.
         */
        Options() noexcept
            : isStandalone(true),
              isResizable(true),
              useTheme(true),
              useThemeColors(true),
              windowTitle("TestUI"),
              transientWindowId(0) {}
    };

    /*!
     * Get options associated with this UI.
     */
    virtual const Options& getOptions() const noexcept = 0;

    // ---------------------------------------------------------------------

protected:
    bool fQuitReceived;
    bool fGotOptions;
    int  fLastMsgTimer;
    CarlaBridgeToolkit* fToolkit;

    lib_t fLib;
    String fLibFilename;
    std::vector<uint8_t> fBase64ReservedChunk;

    /*! @internal */
    bool msgReceived(const char* msg) noexcept override;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaBridgeFormat)
};

/**@}*/

// -----------------------------------------------------------------------

CARLA_BRIDGE_UI_END_NAMESPACE

#endif // CARLA_BRIDGE_FORMAT_HPP_INCLUDED
