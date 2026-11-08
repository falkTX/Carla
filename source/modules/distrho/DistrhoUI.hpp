/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2026 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_UI_HPP_INCLUDED
#define DISTRHO_UI_HPP_INCLUDED

#include "DistrhoDetails.hpp"
#include "extra/LeakDetector.hpp"
#include "src/DistrhoPluginChecks.h"

#ifdef DGL_CAIRO
# include "Cairo.hpp"
#endif
#ifdef DGL_OPENGL
# include "OpenGL.hpp"
#endif
#ifdef DGL_VULKAN
# include "Vulkan.hpp"
#endif

#if DISTRHO_UI_USE_CUSTOM
# include DISTRHO_UI_CUSTOM_INCLUDE_PATH
typedef DISTRHO_UI_CUSTOM_WIDGET_TYPE UIWidget;
#elif DISTRHO_UI_USE_CAIRO
# include "../dgl/Cairo.hpp"
typedef DGL_NAMESPACE::CairoTopLevelWidget UIWidget;
#elif DISTRHO_UI_USE_NANOVG
# include "../dgl/NanoVG.hpp"
typedef DGL_NAMESPACE::NanoTopLevelWidget UIWidget;
#else
# include "../dgl/TopLevelWidget.hpp"
typedef DGL_NAMESPACE::TopLevelWidget UIWidget;
#endif

#if DISTRHO_UI_FILE_BROWSER
# include "extra/FileBrowserDialog.hpp"
#endif

#include <vector>

START_NAMESPACE_DISTRHO

class PluginWindow;

/* --------------------------------------------------------------------------------------------------------------------
 * DPF UI */

/**
   @addtogroup MainClasses
   @{
 */

/**
   DPF UI class from where UI instances are created.

   @note You must call setSize during construction,
   @TODO Detailed information about this class.
 */
class UI : public UIWidget
{
public:
    enum InternalScalingMode {
        kNoInternalScaling,
        kInternalScalingMatchingHost,
    };

   /**
      UI class constructor.
      The UI should be initialized to a default state that matches the plugin side.

      The @p width and @p height arguments are meant to be used without any OS-level UI scaling.
      Scaling will be automatically done internally if needed, matching the OS and host.

      @see getScaleFactor
      @see setGeometryConstraints
    */
    UI(uint width = 0, uint height = 0, InternalScalingMode internalScalingMode = kNoInternalScaling);

   #if DGL_ALLOW_DEPRECATED_METHODS
   /** DEPRECATED DO NOT USE.
    * The old deprecated constructor allowed for an optional `bool automaticallyScaleAndSetAsMinimumSize`.
    * This turned out to be not be a good idea; now the scaling is done automatically while minimum size is not.
    */
    DISTRHO_DEPRECATED_BY("UI(width, height)")
    UI(uint width, uint height, bool automaticallyScaleAndSetAsMinimumSize);
   #endif

   /**
      Destructor.
    */
    ~UI() override;

   /* -----------------------------------------------------------------------------------------------------------------
    * Host state */

   /**
      Check if this UI window is resizable (by the user or window manager).
      There are situations where an UI supports resizing but the plugin host does not, so this could return false.

      You might want to add a resize handle for such cases, so the user is still allowed to resize the window.
      (programatically resizing a window is always possible, but the same is not true for the window manager)
    */
    bool isResizable() const noexcept;

   /**
      Get the color used for UI background (i.e. window color) in RGBA format.
      Returns 0 by default, in case of error or lack of host support.

      The following example code can be use to extract individual colors:
      ```
      int red   = (bgColor >> 24) & 0xff;
      int green = (bgColor >> 16) & 0xff;
      int blue  = (bgColor >>  8) & 0xff;
      ```
      @see Color::fromRGB
    */
    uint getBackgroundColor() const noexcept;

   /**
      Get the color used for UI foreground (i.e. text color) in RGBA format.
      Returns 0xffffffff by default, in case of error or lack of host support.

      The following example code can be use to extract individual colors:
      ```
      int red   = (fgColor >> 24) & 0xff;
      int green = (fgColor >> 16) & 0xff;
      int blue  = (fgColor >>  8) & 0xff;
      ```
      @see Color::fromRGB
    */
    uint getForegroundColor() const noexcept;

   /**
      Get the current sample rate used in plugin processing.
      @see sampleRateChanged(double)
    */
    double getSampleRate() const noexcept;

   /**
      Get the bundle path where the UI resides.@n
      Can return null if the UI is not available in a bundle (if it is a single binary).
      @see getBinaryFilename
    */
    const char* getBundlePath() const noexcept;

   /**
      editParameter.

      Touch/pressed-down event.
      Lets the host know the user is tweaking a parameter.
      Required in some hosts to record automation.
    */
    void editParameter(uint32_t index, bool started);

   /**
      setParameterValue.

      Change a parameter value in the Plugin.
    */
    void setParameterValue(uint32_t index, float value);

#if DISTRHO_PLUGIN_WANT_STATE
   /**
      setState.
      @TODO Document this.
    */
    void setState(const char* key, const char* value);

   /**
      Request a new file from the host, matching the properties of a state key.@n
      This will use the native host file browser if available, otherwise a DPF built-in file browser is used.@n
      Response will be sent asynchronously to stateChanged, with the matching key and the new file as the value.@n
      It is not possible to know if the action was cancelled by the user.

      @return Success if a file-browser was opened, otherwise false.
      @note You cannot request more than one file at a time.
    */
    bool requestStateFile(const char* key);
#endif

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
   /**
      Send a single MIDI note from the UI to the plugin DSP side.@n
      A note with zero velocity will be sent as note-off (MIDI 0x80), otherwise note-on (MIDI 0x90).
    */
    void sendNote(uint8_t channel, uint8_t note, uint8_t velocity);
#endif

#if DISTRHO_UI_FILE_BROWSER
   /**
      Open a file browser dialog with this window as transient parent.@n
      A few options can be specified to setup the dialog.@n
      The @a DISTRHO_NAMESPACE::FileBrowserOptions::className variable is automatically set in this call.

      If a path is selected, onFileSelected() will be called with the user chosen path.
      If the user cancels or does not pick a file, onFileSelected() will be called with nullptr as filename.

      This function does not block the event loop.

      @note This is exactly the same API as provided by the Window class, but redeclared here for convenience.
    */
    bool openFileBrowser(const DISTRHO_NAMESPACE::FileBrowserOptions& options = FileBrowserOptions());
#endif

#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
   /* -----------------------------------------------------------------------------------------------------------------
    * Direct DSP access - DO NOT USE THIS UNLESS STRICTLY NECESSARY!! */

   /**
      getPluginInstancePointer.
      @TODO Document this.
    */
    void* getPluginInstancePointer() const noexcept;
#endif

protected:
   /* -----------------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks */

   /**
      A parameter has changed on the plugin side.@n
      This is called by the host to inform the UI about parameter changes.
    */
    virtual void parameterChanged(uint32_t index, float value);

#if DISTRHO_PLUGIN_WANT_PROGRAMS
   /**
      A program has been loaded on the plugin side.@n
      This is called by the host to inform the UI about program changes.
    */
    virtual void programLoaded(uint32_t index);
#endif

#if DISTRHO_PLUGIN_WANT_STATE
   /**
      A state has changed on the plugin side.@n
      This is called by the host to inform the UI about state changes.
    */
    virtual void stateChanged(const char* key, const char* value);
#endif

   /* -----------------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks (optional) */

   /**
      Optional callback to inform the UI about a sample rate change on the plugin side.
      @see getSampleRate()
    */
    virtual void sampleRateChanged(double newSampleRate);

   /* -----------------------------------------------------------------------------------------------------------------
    * UI Callbacks (optional) */

   /**
      UI idle function, called to give idle time to the plugin UI directly from the host.
      This is called right after OS event handling and Window idle events (within the same cycle).
      There are no guarantees in terms of timing.
      @see addIdleCallback(IdleCallback*, uint).
    */
    virtual void uiIdle() {}

   /**
      Window scale factor function, called when the scale factor changes.
      This function is for plugin UIs to be able to override Window::onScaleFactorChanged(double).

      The default implementation does nothing.
      WARNING function needs a proper name
    */
    virtual void uiScaleFactorChanged(double scaleFactor);

   /**
      Get the types available for the data in a clipboard.
      Must only be called within the context of uiClipboardDataOffer.
    */
    std::vector<DGL_NAMESPACE::ClipboardDataOffer> getClipboardDataOfferTypes();

   /**
      Window clipboard data offer function, called when clipboard has data present, possibly with several datatypes.
      While handling this event, the data types can be investigated with getClipboardDataOfferTypes() to decide whether to accept the offer.

      Reimplement and return a non-zero id to accept the clipboard data offer for a particular type.
      UIs must ignore any type they do not recognize.

      The default implementation accepts the "text/plain" mimetype.
    */
    virtual uint32_t uiClipboardDataOffer();

   /**
      Windows focus function, called when the window gains or loses the keyboard focus.
      This function is for plugin UIs to be able to override Window::onFocus(bool, CrossingMode).

      The default implementation does nothing.
    */
    virtual void uiFocus(bool focus, DGL_NAMESPACE::CrossingMode mode);

   #if DGL_ALLOW_DEPRECATED_METHODS
   /** DEPRECATED DO NOT USE */
    DISTRHO_DEPRECATED
    virtual void uiReshape(uint width, uint height);
   #endif

   #if DISTRHO_UI_FILE_BROWSER
   /**
      Window file selected function, called when a path is selected by the user, as triggered by openFileBrowser().
      This function is for plugin UIs to be able to override Window::onFileSelected(const char*).

      This action happens after the user confirms the action, so the file browser dialog will be closed at this point.
      The default implementation does nothing.

      If you need to use files as plugin state, please setup and use states with kStateIsFilenamePath instead.
    */
    virtual void uiFileBrowserSelected(const char* filename);
   #endif

   /* -----------------------------------------------------------------------------------------------------------------
    * UI Resize Handling, internal */

   /**
      Widget resize function, called when the widget is resized.
      This is overriden here so the host knows when the UI is resized by you.
      @see Widget::onResize(const ResizeEvent&)
    */
    void onResize(const ResizeEvent& ev) override;

    // ----------------------------------------------------------------------------------------------------------------

private:
    struct PrivateData;
    PrivateData* const uiData;
    friend class PluginWindow;
    friend class UIExporter;
   /** @internal */
    void requestSizeChange(uint width, uint height) override;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UI)
};

/** @} */

/* --------------------------------------------------------------------------------------------------------------------
 * Create UI, entry point */

/**
   @addtogroup EntryPoints
   @{
 */

/**
   createUI.
   @TODO Document this.
 */
extern UI* createUI();

/** @} */

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_HPP_INCLUDED
