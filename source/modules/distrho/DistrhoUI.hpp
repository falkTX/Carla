/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
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

#include "extra/LeakDetector.hpp"
#include "src/DistrhoPluginChecks.h"

#ifndef HAVE_DGL
# include "extra/ExternalWindow.hpp"
typedef DISTRHO_NAMESPACE::ExternalWindow UIWidget;
#elif DISTRHO_UI_USE_NANOVG
# include "../dgl/NanoVG.hpp"
typedef DGL::NanoWidget UIWidget;
#else
# include "../dgl/Widget.hpp"
typedef DGL::Widget UIWidget;
#endif

START_NAMESPACE_DISTRHO

/* ------------------------------------------------------------------------------------------------------------
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
   /**
      UI class constructor.
      The UI should be initialized to a default state that matches the plugin side.
    */
    UI(uint width = 0, uint height = 0);

   /**
      Destructor.
    */
    virtual ~UI();

   /* --------------------------------------------------------------------------------------------------------
    * Host state */

   /**
      Get the current sample rate used in plugin processing.
      @see sampleRateChanged(double)
    */
    double getSampleRate() const noexcept;

   /**
      editParameter.
      @TODO Document this.
    */
    void editParameter(uint32_t index, bool started);

   /**
      setParameterValue.
      @TODO Document this.
    */
    void setParameterValue(uint32_t index, float value);

#if DISTRHO_PLUGIN_WANT_STATE
   /**
      setState.
      @TODO Document this.
    */
    void setState(const char* key, const char* value);
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
   /**
      sendNote.
      @TODO Document this.
    */
    void sendNote(uint8_t channel, uint8_t note, uint8_t velocity);
#endif

#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
   /* --------------------------------------------------------------------------------------------------------
    * Direct DSP access - DO NOT USE THIS UNLESS STRICTLY NECESSARY!! */

   /**
      getPluginInstancePointer.
      @TODO Document this.
    */
    void* getPluginInstancePointer() const noexcept;
#endif

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
   /* --------------------------------------------------------------------------------------------------------
    * External UI helpers */

   /**
      Get the bundle path that will be used for the next UI.
      @note: This function is only valid during createUI(),
             it will return null when called from anywhere else.
    */
    static const char* getNextBundlePath() noexcept;

# if DISTRHO_PLUGIN_HAS_EMBED_UI
   /**
      Get the Window Id that will be used for the next created window.
      @note: This function is only valid during createUI(),
             it will return 0 when called from anywhere else.
    */
    static uintptr_t getNextWindowId() noexcept;
# endif
#endif

protected:
   /* --------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks */

   /**
      A parameter has changed on the plugin side.@n
      This is called by the host to inform the UI about parameter changes.
    */
    virtual void parameterChanged(uint32_t index, float value) = 0;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
   /**
      A program has been loaded on the plugin side.@n
      This is called by the host to inform the UI about program changes.
    */
    virtual void programLoaded(uint32_t index) = 0;
#endif

#if DISTRHO_PLUGIN_WANT_STATE
   /**
      A state has changed on the plugin side.@n
      This is called by the host to inform the UI about state changes.
    */
    virtual void stateChanged(const char* key, const char* value) = 0;
#endif

   /* --------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks (optional) */

   /**
      Optional callback to inform the UI about a sample rate change on the plugin side.
      @see getSampleRate()
    */
    virtual void sampleRateChanged(double newSampleRate);

#ifdef HAVE_DGL
   /* --------------------------------------------------------------------------------------------------------
    * UI Callbacks (optional) */

   /**
      uiIdle.
      @TODO Document this.
    */
    virtual void uiIdle() {}

   /**
      File browser selected function.
      @see Window::fileBrowserSelected(const char*)
    */
    virtual void uiFileBrowserSelected(const char* filename);

   /**
      OpenGL window reshape function, called when parent window is resized.
      You can reimplement this function for a custom OpenGL state.
      @see Window::onReshape(uint,uint)
    */
    virtual void uiReshape(uint width, uint height);

   /* --------------------------------------------------------------------------------------------------------
    * UI Resize Handling, internal */

   /**
      OpenGL widget resize function, called when the widget is resized.
      This is overriden here so the host knows when the UI is resized by you.
      @see Widget::onResize(const ResizeEvent&)
    */
    void onResize(const ResizeEvent& ev) override;
#endif

    // -------------------------------------------------------------------------------------------------------

private:
    struct PrivateData;
    PrivateData* const pData;
    friend class UIExporter;
    friend class UIExporterWindow;

#ifdef HAVE_DGL
    // these should not be used
    void setAbsoluteX(int) const noexcept {}
    void setAbsoluteY(int) const noexcept {}
    void setAbsolutePos(int, int) const noexcept {}
    void setAbsolutePos(const DGL::Point<int>&) const noexcept {}
#endif

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UI)
};

/** @} */

/* ------------------------------------------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_HPP_INCLUDED
