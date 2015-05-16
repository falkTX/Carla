/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#include "extra/d_leakdetector.hpp"
#include "src/DistrhoPluginChecks.h"

#if DISTRHO_UI_USE_NANOVG
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
   DPF UI class from where UI instances are created.

   TODO.

   must call setSize during construction,
 */
class UI : public UIWidget
{
public:
   /**
      UI class constructor.
      The UI should be initialized to a default state that matches the plugin side.
    */
    UI();

   /**
      Destructor.
    */
    virtual ~UI();

   /* --------------------------------------------------------------------------------------------------------
    * Host state */

   /**
      Get the current sample rate used in plugin processing.
      @see d_sampleRateChanged(double)
    */
    double d_getSampleRate() const noexcept;

   /**
      TODO: Document this.
    */
    void d_editParameter(const uint32_t index, const bool started);

   /**
      TODO: Document this.
    */
    void d_setParameterValue(const uint32_t index, const float value);

#if DISTRHO_PLUGIN_WANT_STATE
   /**
      TODO: Document this.
    */
    void d_setState(const char* const key, const char* const value);
#endif

#if DISTRHO_PLUGIN_IS_SYNTH
   /**
      TODO: Document this.
    */
    void d_sendNote(const uint8_t channel, const uint8_t note, const uint8_t velocity);
#endif

#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
   /* --------------------------------------------------------------------------------------------------------
    * Direct DSP access - DO NOT USE THIS UNLESS STRICTLY NECESSARY!! */

   /**
      TODO: Document this.
    */
    void* d_getPluginInstancePointer() const noexcept;
#endif

protected:
   /* --------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks */

   /**
      A parameter has changed on the plugin side.
      This is called by the host to inform the UI about parameter changes.
    */
    virtual void d_parameterChanged(uint32_t index, float value) = 0;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
   /**
      The current program has changed on the plugin side.
      This is called by the host to inform the UI about program changes.
    */
    virtual void d_programChanged(uint32_t index) = 0;
#endif

#if DISTRHO_PLUGIN_WANT_STATE
   /**
      A state has changed on the plugin side.
      This is called by the host to inform the UI about state changes.
    */
    virtual void d_stateChanged(const char* key, const char* value) = 0;
#endif

   /* --------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks (optional) */

   /**
      Optional callback to inform the UI about a sample rate change on the plugin side.
      @see d_getSampleRate()
    */
    virtual void d_sampleRateChanged(double newSampleRate);

   /* --------------------------------------------------------------------------------------------------------
    * UI Callbacks (optional) */

   /**
      TODO: Document this.
    */
    virtual void d_uiIdle() {}

   /**
      File browser selected function.
      @see Window::fileBrowserSelected(const char*)
    */
    virtual void d_uiFileBrowserSelected(const char* filename);

   /**
      OpenGL window reshape function, called when parent window is resized.
      You can reimplement this function for a custom OpenGL state.
      @see Window::onReshape(uint,uint)
    */
    virtual void d_uiReshape(uint width, uint height);

   /* --------------------------------------------------------------------------------------------------------
    * UI Resize Handling, internal */

   /**
      OpenGL widget resize function, called when the widget is resized.
      This is overriden here so the host knows when the UI is resized by you.
      @see Widget::onResize(const ResizeEvent&)
    */
    void onResize(const ResizeEvent& ev) override;

    // -------------------------------------------------------------------------------------------------------

private:
    struct PrivateData;
    PrivateData* const pData;
    friend class UIExporter;
    friend class UIExporterWindow;

    // these should not be used
    void setAbsoluteX(int) const noexcept {}
    void setAbsoluteY(int) const noexcept {}
    void setAbsolutePos(int, int) const noexcept {}
    void setAbsolutePos(const DGL::Point<int>&) const noexcept {}
    void setNeedsFullViewport(bool) const noexcept {}

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UI)
};

/* ------------------------------------------------------------------------------------------------------------
 * Create UI, entry point */

/**
   TODO.
 */
extern UI* createUI();

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_HPP_INCLUDED
