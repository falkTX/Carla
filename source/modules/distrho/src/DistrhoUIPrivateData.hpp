/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2022 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_UI_PRIVATE_DATA_HPP_INCLUDED
#define DISTRHO_UI_PRIVATE_DATA_HPP_INCLUDED

#include "../DistrhoUI.hpp"

#ifdef DISTRHO_PLUGIN_TARGET_VST3
# include "DistrhoPluginVST.hpp"
#endif

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
# include "../extra/Sleep.hpp"
// TODO import and use file browser here
#else
# include "../../dgl/src/ApplicationPrivateData.hpp"
# include "../../dgl/src/WindowPrivateData.hpp"
# include "../../dgl/src/pugl.hpp"
#endif

#if defined(DISTRHO_PLUGIN_TARGET_JACK) || defined(DISTRHO_PLUGIN_TARGET_DSSI)
# define DISTRHO_UI_IS_STANDALONE 1
#else
# define DISTRHO_UI_IS_STANDALONE 0
#endif

#ifdef DISTRHO_PLUGIN_TARGET_VST3
# define DISTRHO_UI_IS_VST3 1
#else
# define DISTRHO_UI_IS_VST3 0
#endif

#ifdef DISTRHO_PLUGIN_TARGET_VST2
# undef DISTRHO_UI_USER_RESIZABLE
# define DISTRHO_UI_USER_RESIZABLE 0
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// Plugin Application, will set class name based on plugin details

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
struct PluginApplication
{
    DGL_NAMESPACE::IdleCallback* idleCallback;
    UI* ui;

    explicit PluginApplication()
        : idleCallback(nullptr),
          ui(nullptr) {}

    void addIdleCallback(DGL_NAMESPACE::IdleCallback* const cb)
    {
        DISTRHO_SAFE_ASSERT_RETURN(cb != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(idleCallback == nullptr,);

        idleCallback = cb;
    }

    bool isQuitting() const noexcept
    {
        return ui->isQuitting();
    }

    bool isStandalone() const noexcept
    {
        return DISTRHO_UI_IS_STANDALONE;
    }

    void exec()
    {
        while (ui->isRunning())
        {
            d_msleep(30);
            idleCallback->idleCallback();
        }

        if (! ui->isQuitting())
            ui->close();
    }

    // these are not needed
    void idle() {}
    void quit() {}
    void triggerIdleCallbacks() {}

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginApplication)
};
#else
class PluginApplication : public DGL_NAMESPACE::Application
{
public:
    explicit PluginApplication()
        : DGL_NAMESPACE::Application(DISTRHO_UI_IS_STANDALONE)
    {
#ifndef DISTRHO_OS_WASM
        const char* const className = (
#ifdef DISTRHO_PLUGIN_BRAND
            DISTRHO_PLUGIN_BRAND
#else
            DISTRHO_MACRO_AS_STRING(DISTRHO_NAMESPACE)
#endif
            "-" DISTRHO_PLUGIN_NAME
        );
        setClassName(className);
#endif
    }

    void triggerIdleCallbacks()
    {
        pData->triggerIdleCallbacks();
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginApplication)
};
#endif

// -----------------------------------------------------------------------
// Plugin Window, will pass some Window events to UI

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
class PluginWindow
{
    UI* const ui;

public:
    explicit PluginWindow(UI* const uiPtr, PluginApplication& app)
        : ui(uiPtr)
    {
        app.ui = ui;
    }

    // fetch cached data
    uint getWidth() const noexcept { return ui->pData.width; }
    uint getHeight() const noexcept { return ui->pData.height; }
    double getScaleFactor() const noexcept { return ui->pData.scaleFactor; }

    // direct mappings
    void close() { ui->close(); }
    void focus() { ui->focus(); }
    void show() { ui->show(); }
    bool isResizable() const noexcept { return ui->isResizable(); }
    bool isVisible() const noexcept { return ui->isVisible(); }
    void setTitle(const char* const title) { ui->setTitle(title); }
    void setVisible(const bool visible) { ui->setVisible(visible); }
    uintptr_t getNativeWindowHandle() const noexcept { return ui->getNativeWindowHandle(); }
    void getGeometryConstraints(uint& minimumWidth, uint& minimumHeight, bool& keepAspectRatio) const noexcept
    {
        minimumWidth = ui->pData.minWidth;
        minimumHeight = ui->pData.minHeight;
        keepAspectRatio = ui->pData.keepAspectRatio;
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWindow)
};
#else // DISTRHO_PLUGIN_HAS_EXTERNAL_UI
class PluginWindow : public DGL_NAMESPACE::Window
{
    UI* const ui;
    bool initializing;
    bool receivedReshapeDuringInit;

public:
    explicit PluginWindow(UI* const uiPtr,
                          PluginApplication& app,
                          const uintptr_t parentWindowHandle,
                          const uint width,
                          const uint height,
                          const double scaleFactor)
        : Window(app, parentWindowHandle, width, height, scaleFactor,
                 DISTRHO_UI_USER_RESIZABLE, DISTRHO_UI_IS_VST3, false),
          ui(uiPtr),
          initializing(true),
          receivedReshapeDuringInit(false)
    {
        if (pData->view == nullptr)
            return;

        // this is called just before creating UI, ensuring proper context to it
        if (pData->initPost())
            puglBackendEnter(pData->view);
    }

    ~PluginWindow() override
    {
        if (pData->view != nullptr)
            puglBackendLeave(pData->view);
    }

    // called after creating UI, restoring proper context
    void leaveContext()
    {
        if (pData->view == nullptr)
            return;

        if (receivedReshapeDuringInit)
            ui->uiReshape(getWidth(), getHeight());

        initializing = false;
        puglBackendLeave(pData->view);
    }

    // used for temporary windows (VST2/3 get size without active/visible view)
    void setIgnoreIdleCallbacks(const bool ignore = true)
    {
        pData->ignoreIdleCallbacks = ignore;
    }

    // called right before deleting UI, ensuring correct context
    void enterContextForDeletion()
    {
        if (pData->view != nullptr)
            puglBackendEnter(pData->view);
    }

   #ifdef DISTRHO_PLUGIN_TARGET_VST3
    void setSizeForVST3(const uint width, const uint height)
    {
        puglSetSizeAndDefault(pData->view, width, height);
    }
   #endif

    std::vector<DGL_NAMESPACE::ClipboardDataOffer> getClipboardDataOfferTypes()
    {
        return Window::getClipboardDataOfferTypes();
    }

protected:
    uint32_t onClipboardDataOffer() override
    {
        DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr, 0);

        if (initializing)
            return 0;

        return ui->uiClipboardDataOffer();
    }

    void onFocus(const bool focus, const DGL_NAMESPACE::CrossingMode mode) override
    {
        DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr,);

        if (initializing)
            return;

        ui->uiFocus(focus, mode);
    }

    void onReshape(const uint width, const uint height) override
    {
        DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr,);

        if (initializing)
        {
            receivedReshapeDuringInit = true;
            return;
        }

        ui->uiReshape(width, height);
    }

    void onScaleFactorChanged(const double scaleFactor) override
    {
        DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr,);

        if (initializing)
            return;

        ui->uiScaleFactorChanged(scaleFactor);
    }

# if DISTRHO_UI_FILE_BROWSER
    void onFileSelected(const char* filename) override;
# endif

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWindow)
};
#endif // DISTRHO_PLUGIN_HAS_EXTERNAL_UI

// -----------------------------------------------------------------------
// UI callbacks

typedef void (*editParamFunc)   (void* ptr, uint32_t rindex, bool started);
typedef void (*setParamFunc)    (void* ptr, uint32_t rindex, float value);
typedef void (*setStateFunc)    (void* ptr, const char* key, const char* value);
typedef void (*sendNoteFunc)    (void* ptr, uint8_t channel, uint8_t note, uint8_t velo);
typedef void (*setSizeFunc)     (void* ptr, uint width, uint height);
typedef bool (*fileRequestFunc) (void* ptr, const char* key);

// -----------------------------------------------------------------------
// UI private data

struct UI::PrivateData {
    // DGL
    PluginApplication app;
    ScopedPointer<PluginWindow> window;

    // DSP
    double   sampleRate;
    uint32_t parameterOffset;
    void*    dspPtr;

    // UI
    uint bgColor;
    uint fgColor;
    double scaleFactor;
    uintptr_t winId;
#if DISTRHO_UI_FILE_BROWSER && !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
    char* uiStateFileKeyRequest;
#endif
    char* bundlePath;

    // Ignore initial resize events while initializing
    bool initializing;

    // Callbacks
    void*           callbacksPtr;
    editParamFunc   editParamCallbackFunc;
    setParamFunc    setParamCallbackFunc;
    setStateFunc    setStateCallbackFunc;
    sendNoteFunc    sendNoteCallbackFunc;
    setSizeFunc     setSizeCallbackFunc;
    fileRequestFunc fileRequestCallbackFunc;

    PrivateData() noexcept
        : app(),
          window(nullptr),
          sampleRate(0),
          parameterOffset(0),
          dspPtr(nullptr),
          bgColor(0),
          fgColor(0xffffffff),
          scaleFactor(1.0),
          winId(0),
#if DISTRHO_UI_FILE_BROWSER && !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
          uiStateFileKeyRequest(nullptr),
#endif
          bundlePath(nullptr),
          initializing(true),
          callbacksPtr(nullptr),
          editParamCallbackFunc(nullptr),
          setParamCallbackFunc(nullptr),
          setStateCallbackFunc(nullptr),
          sendNoteCallbackFunc(nullptr),
          setSizeCallbackFunc(nullptr),
          fileRequestCallbackFunc(nullptr)
    {
#if defined(DISTRHO_PLUGIN_TARGET_DSSI) || defined(DISTRHO_PLUGIN_TARGET_LV2)
        parameterOffset += DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS;
# if DISTRHO_PLUGIN_WANT_LATENCY
        parameterOffset += 1;
# endif
#endif

#ifdef DISTRHO_PLUGIN_TARGET_LV2
# if (DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_TIMEPOS || DISTRHO_PLUGIN_WANT_STATE)
        parameterOffset += 1;
# endif
# if (DISTRHO_PLUGIN_WANT_MIDI_OUTPUT || DISTRHO_PLUGIN_WANT_STATE)
        parameterOffset += 1;
# endif
#endif

#ifdef DISTRHO_PLUGIN_TARGET_VST3
        parameterOffset += kVst3InternalParameterCount;
#endif
    }

    ~PrivateData() noexcept
    {
#if DISTRHO_UI_FILE_BROWSER && !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
        std::free(uiStateFileKeyRequest);
#endif
        std::free(bundlePath);
    }

    void editParamCallback(const uint32_t rindex, const bool started)
    {
        if (editParamCallbackFunc != nullptr)
            editParamCallbackFunc(callbacksPtr, rindex, started);
    }

    void setParamCallback(const uint32_t rindex, const float value)
    {
        if (setParamCallbackFunc != nullptr)
            setParamCallbackFunc(callbacksPtr, rindex, value);
    }

    void setStateCallback(const char* const key, const char* const value)
    {
        if (setStateCallbackFunc != nullptr)
            setStateCallbackFunc(callbacksPtr, key, value);
    }

    void sendNoteCallback(const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        if (sendNoteCallbackFunc != nullptr)
            sendNoteCallbackFunc(callbacksPtr, channel, note, velocity);
    }

    void setSizeCallback(const uint width, const uint height)
    {
        if (setSizeCallbackFunc != nullptr)
            setSizeCallbackFunc(callbacksPtr, width, height);
    }

    // implemented below, after PluginWindow
    bool fileRequestCallback(const char* const key);

    static UI::PrivateData* s_nextPrivateData;
#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
    static ExternalWindow::PrivateData createNextWindow(UI* ui, uint width, uint height);
#else
    static PluginWindow& createNextWindow(UI* ui, uint width, uint height);
#endif
};

// -----------------------------------------------------------------------
// UI private data fileRequestCallback, which requires PluginWindow definitions

inline bool UI::PrivateData::fileRequestCallback(const char* const key)
{
    if (fileRequestCallbackFunc != nullptr)
        return fileRequestCallbackFunc(callbacksPtr, key);

#if DISTRHO_PLUGIN_WANT_STATE && DISTRHO_UI_FILE_BROWSER && !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
    std::free(uiStateFileKeyRequest);
    uiStateFileKeyRequest = strdup(key);
    DISTRHO_SAFE_ASSERT_RETURN(uiStateFileKeyRequest != nullptr, false);

    char title[0xff];
    snprintf(title, sizeof(title)-1u, DISTRHO_PLUGIN_NAME ": %s", key);
    title[sizeof(title)-1u] = '\0';

    DGL_NAMESPACE::FileBrowserOptions opts;
    opts.title = title;
    return window->openFileBrowser(opts);
#endif

    return false;
}

// -----------------------------------------------------------------------
// PluginWindow onFileSelected that require UI::PrivateData definitions

#if DISTRHO_UI_FILE_BROWSER && !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
inline void PluginWindow::onFileSelected(const char* const filename)
{
    DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr,);

    if (initializing)
        return;

   #if DISTRHO_PLUGIN_WANT_STATE
    if (char* const key = ui->uiData->uiStateFileKeyRequest)
    {
        ui->uiData->uiStateFileKeyRequest = nullptr;
        if (filename != nullptr)
        {
            // notify DSP
            ui->setState(key, filename);
            // notify UI
            ui->stateChanged(key, filename);
        }
        std::free(key);
        return;
    }
   #endif

    puglBackendEnter(pData->view);
    ui->uiFileBrowserSelected(filename);
    puglBackendLeave(pData->view);
}
#endif

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_PRIVATE_DATA_HPP_INCLUDED
