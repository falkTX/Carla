/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2025 Filipe Coelho <falktx@falktx.com>
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

#include "../../dgl/src/ApplicationPrivateData.hpp"
#include "../../dgl/src/WindowPrivateData.hpp"
#include "../../dgl/src/pugl.hpp"

#if DISTRHO_PLUGIN_WANT_STATE && DISTRHO_UI_FILE_BROWSER
# include <map>
# include <string>
#endif

#if DISTRHO_UI_USE_WEB_VIEW
# include "extra/WebView.hpp"
#endif

#if defined(DISTRHO_PLUGIN_TARGET_JACK) || defined(DISTRHO_PLUGIN_TARGET_DSSI)
# define DISTRHO_UI_IS_STANDALONE 1
#else
# define DISTRHO_UI_IS_STANDALONE 0
#endif

#if defined(DISTRHO_PLUGIN_TARGET_AU)
# define DISTRHO_UI_USES_SCHEDULED_REPAINTS 1
#else
# define DISTRHO_UI_USES_SCHEDULED_REPAINTS 0
#endif

#if defined(DISTRHO_PLUGIN_TARGET_CLAP) || defined(DISTRHO_PLUGIN_TARGET_VST3)
# define DISTRHO_UI_USES_SIZE_REQUEST 1
#else
# define DISTRHO_UI_USES_SIZE_REQUEST 0
#endif

#if defined(DISTRHO_PLUGIN_TARGET_AU) || defined(DISTRHO_PLUGIN_TARGET_VST2)
# undef DISTRHO_UI_USER_RESIZABLE
# define DISTRHO_UI_USER_RESIZABLE 0
#endif

START_NAMESPACE_DISTRHO

/* define webview start */
#if defined(HAVE_X11) && defined(DISTRHO_OS_LINUX) && DISTRHO_UI_WEB_VIEW
# define DISTRHO_UI_LINUX_WEBVIEW_START
int dpf_webview_start(int argc, char* argv[]);
#endif

// --------------------------------------------------------------------------------------------------------------------
// Plugin Application, will set class name based on plugin details

class PluginApplication : public DGL_NAMESPACE::Application
{
public:
    explicit PluginApplication(const char* className, const Application::Type type)
        : DGL_NAMESPACE::Application(DISTRHO_UI_IS_STANDALONE, type)
    {
       #if defined(__MOD_DEVICES__) || !defined(__EMSCRIPTEN__)
        if (className == nullptr)
        {
            className = (
               #ifdef DISTRHO_PLUGIN_BRAND
                DISTRHO_PLUGIN_BRAND
               #else
                DISTRHO_MACRO_AS_STRING(DISTRHO_NAMESPACE)
               #endif
                "-" DISTRHO_PLUGIN_NAME
            );
        }
        setClassName(className);
       #else
        // unused
        (void)className;
       #endif
    }

    void triggerIdleCallbacks()
    {
        pData->triggerIdleCallbacks();
    }

    void repaintIfNeeeded()
    {
        pData->repaintIfNeeeded();
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginApplication)
};

// --------------------------------------------------------------------------------------------------------------------
// Plugin Window, will pass some Window events to UI

class PluginWindow : public DGL_NAMESPACE::Window
{
    UI* const ui;
    bool initializing;
   #if DGL_ALLOW_DEPRECATED_METHODS
    bool receivedReshapeDuringInit;
   #endif

public:
    explicit PluginWindow(UI* const uiPtr,
                          PluginApplication& app,
                          const uintptr_t parentWindowHandle,
                          const uint width,
                          const uint height,
                          const double scaleFactor)
        : Window(app, parentWindowHandle, width, height, scaleFactor,
                 DISTRHO_UI_USER_RESIZABLE,
                 DISTRHO_UI_USES_SCHEDULED_REPAINTS,
                 DISTRHO_UI_USES_SIZE_REQUEST,
                 false),
          ui(uiPtr),
          initializing(true)
       #if DGL_ALLOW_DEPRECATED_METHODS
        , receivedReshapeDuringInit(false)
       #endif
    {
        if (pData->view == nullptr)
            return;

        // this is called just before creating UI, ensuring proper context to it
        if (pData->initPost())
        {
            puglBackendEnter(pData->view);
            pData->createContextIfNeeded();
        }
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

        initializing = false;
        puglBackendLeave(pData->view);

       #if DGL_ALLOW_DEPRECATED_METHODS
        if (receivedReshapeDuringInit)
        {
            puglBackendEnter(pData->view);
           #if defined(_MSC_VER)
            #pragma warning(push)
            #pragma warning(disable:4996)
           #elif defined(__clang__)
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wdeprecated-declarations"
           #elif defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 460
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
           #endif
            ui->uiReshape(getWidth(), getHeight());
           #if defined(_MSC_VER)
            #pragma warning(pop)
           #elif defined(__clang__)
            #pragma clang diagnostic pop
           #elif defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 460
            #pragma GCC diagnostic pop
           #endif
            puglBackendLeave(pData->view);
        }
       #endif
    }

    // used for temporary windows (VST/CLAP get size without active/visible view)
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

   #if DISTRHO_UI_USES_SIZE_REQUEST
    void setSizeFromHost(const uint width, const uint height)
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

  #if DGL_ALLOW_DEPRECATED_METHODS
   #if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable:4996)
   #elif defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"
   #elif defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 460
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
   #endif
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
   #if defined(_MSC_VER)
    #pragma warning(pop)
   #elif defined(__clang__)
    #pragma clang diagnostic pop
   #elif defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 460
    #pragma GCC diagnostic pop
   #endif
  #endif

    void onScaleFactorChanged(const double scaleFactor) override
    {
        DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr,);

        if (initializing)
            return;

        ui->uiScaleFactorChanged(scaleFactor);
    }

   #if DISTRHO_UI_FILE_BROWSER
    void onFileSelected(const char* filename) override;
   #endif

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWindow)
};

// --------------------------------------------------------------------------------------------------------------------
// UI callbacks

typedef void (*editParamFunc)   (void* ptr, uint32_t rindex, bool started);
typedef void (*setParamFunc)    (void* ptr, uint32_t rindex, float value);
typedef void (*setStateFunc)    (void* ptr, const char* key, const char* value);
typedef void (*sendNoteFunc)    (void* ptr, uint8_t channel, uint8_t note, uint8_t velo);
typedef void (*setSizeFunc)     (void* ptr, uint width, uint height);
typedef bool (*fileRequestFunc) (void* ptr, const char* key);

// --------------------------------------------------------------------------------------------------------------------
// UI private data

struct UI::PrivateData {
    // DGL
    PluginApplication app;
    ScopedPointer<PluginWindow> window;
   #if DISTRHO_UI_USE_WEB_VIEW
    WebViewHandle webview;
   #endif

    // DSP
    double   sampleRate;
    uint32_t parameterOffset;
    void*    dspPtr;

    // UI
    uint bgColor;
    uint fgColor;
    double scaleFactor;
    uintptr_t winId;
   #if DISTRHO_PLUGIN_WANT_STATE && DISTRHO_UI_FILE_BROWSER
    char* uiStateFileKeyRequest;
    std::map<std::string,std::string> lastUsedDirnames;
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

    PrivateData(const char* const appClassName, const DGL_NAMESPACE::Application::Type appType) noexcept
        : app(appClassName, appType),
          window(nullptr),
         #if DISTRHO_UI_USE_WEB_VIEW
          webview(nullptr),
         #endif
          sampleRate(0),
          parameterOffset(0),
          dspPtr(nullptr),
          bgColor(0),
          fgColor(0xffffffff),
          scaleFactor(1.0),
          winId(0),
         #if DISTRHO_PLUGIN_WANT_STATE && DISTRHO_UI_FILE_BROWSER
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
       #if DISTRHO_PLUGIN_WANT_LATENCY
        parameterOffset += 1;
       #endif
      #endif

      #ifdef DISTRHO_PLUGIN_TARGET_LV2
       #if (DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_TIMEPOS || DISTRHO_PLUGIN_WANT_STATE)
        parameterOffset += 1;
       #endif
       #if (DISTRHO_PLUGIN_WANT_MIDI_OUTPUT || DISTRHO_PLUGIN_WANT_STATE)
        parameterOffset += 1;
       #endif
      #endif

       #ifdef DISTRHO_PLUGIN_TARGET_VST3
        parameterOffset += kVst3InternalParameterCount;
       #endif
    }

    ~PrivateData() noexcept
    {
       #if DISTRHO_PLUGIN_WANT_STATE && DISTRHO_UI_FILE_BROWSER
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
        DISTRHO_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        DISTRHO_SAFE_ASSERT_RETURN(value != nullptr,);

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
        DISTRHO_SAFE_ASSERT_RETURN(width != 0 && height != 0,);

        if (setSizeCallbackFunc != nullptr)
            setSizeCallbackFunc(callbacksPtr, width, height);
    }

    // implemented below, after PluginWindow
    bool fileRequestCallback(const char* key);

    static UI::PrivateData* s_nextPrivateData;
    static PluginWindow& createNextWindow(UI* ui, uint width, uint height);
   #if DISTRHO_UI_USE_WEB_VIEW
    static void webViewMessageCallback(void* arg, char* msg);
   #endif
};

// --------------------------------------------------------------------------------------------------------------------
// UI private data fileRequestCallback, which requires PluginWindow definitions

inline bool UI::PrivateData::fileRequestCallback(const char* const key)
{
    if (fileRequestCallbackFunc != nullptr)
        return fileRequestCallbackFunc(callbacksPtr, key);

   #if DISTRHO_PLUGIN_WANT_STATE && DISTRHO_UI_FILE_BROWSER
    std::free(uiStateFileKeyRequest);
    uiStateFileKeyRequest = strdup(key);
    DISTRHO_SAFE_ASSERT_RETURN(uiStateFileKeyRequest != nullptr, false);

    char title[0xff];
    snprintf(title, sizeof(title)-1u, DISTRHO_PLUGIN_NAME ": %s", key);
    title[sizeof(title)-1u] = '\0';

    DGL_NAMESPACE::FileBrowserOptions opts;
    opts.title = title;
    if  (lastUsedDirnames.count(key))
        opts.startDir = lastUsedDirnames[key].c_str();
    return window->openFileBrowser(opts);
   #endif

    return false;
}

// --------------------------------------------------------------------------------------------------------------------
// PluginWindow onFileSelected that require UI::PrivateData definitions

#if DISTRHO_UI_FILE_BROWSER
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

            // save dirname for next time
            if (const char* const lastsep = std::strrchr(filename, DISTRHO_OS_SEP))
                ui->uiData->lastUsedDirnames[key] = std::string(filename, lastsep-filename);
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

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_PRIVATE_DATA_HPP_INCLUDED
