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

#include "DistrhoDetails.hpp"
#include "DistrhoPluginUtils.hpp"
#include "src/DistrhoPluginChecks.h"

#include <cstddef>

#ifdef DISTRHO_PROPER_CPP11_SUPPORT
# include <cstdint>
#else
# include <stdint.h>
#endif

#if defined(DISTRHO_OS_WASM)
# include <emscripten/emscripten.h>
#elif defined(DISTRHO_OS_WINDOWS)
# include <winsock2.h>
# include <windows.h>
#elif defined(HAVE_X11)
# define Window X11Window
# include <X11/Xresource.h>
# undef Window
#endif

#if DISTRHO_UI_FILE_BROWSER && !defined(DISTRHO_OS_MAC)
# define DISTRHO_PUGL_NAMESPACE_MACRO_HELPER(NS, SEP, FUNCTION) NS ## SEP ## FUNCTION
# define DISTRHO_PUGL_NAMESPACE_MACRO(NS, FUNCTION) DISTRHO_PUGL_NAMESPACE_MACRO_HELPER(NS, _, FUNCTION)
# define x_fib_add_recent          DISTRHO_PUGL_NAMESPACE_MACRO(plugin, x_fib_add_recent)
# define x_fib_cfg_buttons         DISTRHO_PUGL_NAMESPACE_MACRO(plugin, x_fib_cfg_buttons)
# define x_fib_cfg_filter_callback DISTRHO_PUGL_NAMESPACE_MACRO(plugin, x_fib_cfg_filter_callback)
# define x_fib_close               DISTRHO_PUGL_NAMESPACE_MACRO(plugin, x_fib_close)
# define x_fib_configure           DISTRHO_PUGL_NAMESPACE_MACRO(plugin, x_fib_configure)
# define x_fib_filename            DISTRHO_PUGL_NAMESPACE_MACRO(plugin, x_fib_filename)
# define x_fib_free_recent         DISTRHO_PUGL_NAMESPACE_MACRO(plugin, x_fib_free_recent)
# define x_fib_handle_events       DISTRHO_PUGL_NAMESPACE_MACRO(plugin, x_fib_handle_events)
# define x_fib_load_recent         DISTRHO_PUGL_NAMESPACE_MACRO(plugin, x_fib_load_recent)
# define x_fib_recent_at           DISTRHO_PUGL_NAMESPACE_MACRO(plugin, x_fib_recent_at)
# define x_fib_recent_count        DISTRHO_PUGL_NAMESPACE_MACRO(plugin, x_fib_recent_count)
# define x_fib_recent_file         DISTRHO_PUGL_NAMESPACE_MACRO(plugin, x_fib_recent_file)
# define x_fib_save_recent         DISTRHO_PUGL_NAMESPACE_MACRO(plugin, x_fib_save_recent)
# define x_fib_show                DISTRHO_PUGL_NAMESPACE_MACRO(plugin, x_fib_show)
# define x_fib_status              DISTRHO_PUGL_NAMESPACE_MACRO(plugin, x_fib_status)
# define DISTRHO_FILE_BROWSER_DIALOG_HPP_INCLUDED
# define FILE_BROWSER_DIALOG_NAMESPACE DISTRHO_NAMESPACE
# define FILE_BROWSER_DIALOG_DISTRHO_NAMESPACE
START_NAMESPACE_DISTRHO
# include "../extra/FileBrowserDialogImpl.hpp"
END_NAMESPACE_DISTRHO
# define Window X11Window
# include "../extra/FileBrowserDialogImpl.cpp"
# undef Window
#endif

#if DISTRHO_UI_WEB_VIEW && !defined(DISTRHO_OS_MAC)
# define DISTRHO_WEB_VIEW_HPP_INCLUDED
# define WEB_VIEW_NAMESPACE DISTRHO_NAMESPACE
# define WEB_VIEW_DISTRHO_NAMESPACE
START_NAMESPACE_DISTRHO
# include "../extra/WebViewImpl.hpp"
END_NAMESPACE_DISTRHO
# define Window X11Window
# include "../extra/WebViewImpl.cpp"
# undef Window
#endif

#include "src/TopLevelWidgetPrivateData.hpp"
#include "src/WindowPrivateData.hpp"
#include "DistrhoUIPrivateData.hpp"

START_NAMESPACE_DISTRHO

/* ------------------------------------------------------------------------------------------------------------
 * get global scale factor */

#ifdef DISTRHO_OS_MAC
double getDesktopScaleFactor(uintptr_t parentWindowHandle);
#else
static double getDesktopScaleFactor(const uintptr_t parentWindowHandle)
{
    // allow custom scale for testing
    if (const char* const scale = getenv("DPF_SCALE_FACTOR"))
        return std::max(1.0, std::atof(scale));

   #if defined(DISTRHO_OS_WASM)
    return emscripten_get_device_pixel_ratio();
   #elif defined(DISTRHO_OS_WINDOWS)
    if (const HMODULE Shcore = LoadLibraryA("Shcore.dll"))
    {
        typedef HRESULT(WINAPI* PFN_GetProcessDpiAwareness)(HANDLE, DWORD*);
        typedef HRESULT(WINAPI* PFN_GetScaleFactorForMonitor)(HMONITOR, DWORD*);

      #if defined(__GNUC__) && (__GNUC__ >= 9)
       #pragma GCC diagnostic push
       #pragma GCC diagnostic ignored "-Wcast-function-type"
      #endif
        const PFN_GetProcessDpiAwareness GetProcessDpiAwareness
            = (PFN_GetProcessDpiAwareness)GetProcAddress(Shcore, "GetProcessDpiAwareness");
        const PFN_GetScaleFactorForMonitor GetScaleFactorForMonitor
            = (PFN_GetScaleFactorForMonitor)GetProcAddress(Shcore, "GetScaleFactorForMonitor");
      #if defined(__GNUC__) && (__GNUC__ >= 9)
       #pragma GCC diagnostic pop
      #endif

        DWORD dpiAware = 0;
        DWORD scaleFactor = 100;
        if (GetProcessDpiAwareness && GetScaleFactorForMonitor
            && GetProcessDpiAwareness(nullptr, &dpiAware) == 0 && dpiAware != 0)
        {
            const HMONITOR hMon = parentWindowHandle != 0
                                ? MonitorFromWindow((HWND)parentWindowHandle, MONITOR_DEFAULTTOPRIMARY)
                                : MonitorFromPoint(POINT(), MONITOR_DEFAULTTOPRIMARY);
            GetScaleFactorForMonitor(hMon, &scaleFactor);
        }

        FreeLibrary(Shcore);
        return static_cast<double>(scaleFactor) / 100.0;
    }
   #elif defined(HAVE_X11)
    ::Display* const display = XOpenDisplay(nullptr);
    DISTRHO_SAFE_ASSERT_RETURN(display != nullptr, 1.0);

    XrmInitialize();

    double dpi = 96.0;
    if (char* const rms = XResourceManagerString(display))
    {
        if (const XrmDatabase db = XrmGetStringDatabase(rms))
        {
            char* type = nullptr;
            XrmValue value = {};

            if (XrmGetResource(db, "Xft.dpi", "Xft.Dpi", &type, &value)
                && type != nullptr
                && std::strcmp(type, "String") == 0
                && value.addr != nullptr)
            {
                char*        end    = nullptr;
                const double xftDpi = std::strtod(value.addr, &end);
                if (xftDpi > 0.0 && xftDpi < HUGE_VAL)
                    dpi = xftDpi;
            }

            XrmDestroyDatabase(db);
        }
    }

    XCloseDisplay(display);
    return dpi / 96;
   #endif

    return 1.0;

    // might be unused
    (void)parentWindowHandle;
}
#endif // !DISTRHO_OS_MAC

/* ------------------------------------------------------------------------------------------------------------
 * UI::PrivateData special handling */

UI::PrivateData* UI::PrivateData::s_nextPrivateData = nullptr;

PluginWindow& UI::PrivateData::createNextWindow(UI* const ui, uint width, uint height)
{
    UI::PrivateData* const uiData = s_nextPrivateData;
    const double scaleFactor = d_isNotZero(uiData->scaleFactor) ? uiData->scaleFactor : getDesktopScaleFactor(uiData->winId);

    if (d_isNotEqual(scaleFactor, 1.0))
    {
        width *= scaleFactor;
        height *= scaleFactor;
    }

    d_stdout("createNextWindow %u %u %f", width, height, scaleFactor);
    uiData->window = new PluginWindow(ui, uiData->app, uiData->winId, width, height, scaleFactor);

    if (uiData->callbacksPtr != nullptr)
    {
       #if DISTRHO_UI_USE_WEB_VIEW
        String path;
        if (uiData->bundlePath != nullptr)
        {
            path = getResourcePath(uiData->bundlePath);
        }
        else
        {
            path = getBinaryFilename();
            path.truncate(path.rfind(DISTRHO_OS_SEP));
            path += "/resources";
        }

        path.urlEncode();

        // TODO convert win32 paths to web

        WebViewOptions opts;
        opts.initialJS = ""
"editParameter = function(index, started){ postMessage('editparam ' + index + ' ' + (started ? '1' : '0')) };"
"setParameterValue = function(index, value){ postMessage('setparam ' + index + ' ' + value) };"
#if DISTRHO_PLUGIN_WANT_STATE
"setState = function(key, value){ postMessage('setstate ' + key + ' ' + value) };"
"requestStateFile = function(key){ postMessage('reqstatefile ' + key) };"
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
"sendNote = function(channel, note, velocity){ postMessage('sendnote ' + channel + ' ' + note + ' ' + velocity) };"
#endif
        ;
        opts.callback = webViewMessageCallback;
        opts.callbackPtr = uiData;
        uiData->webview = webViewCreate("file://" + path + "/index.html",
                                        uiData->winId != 0 ? uiData->winId : uiData->window->getNativeWindowHandle(),
                                        width,
                                        height,
                                        scaleFactor,
                                        opts);
       #endif
    }
    // If there are no callbacks, this is most likely a temporary window, so ignore idle callbacks
    else
    {
        uiData->window->setIgnoreIdleCallbacks();
    }

    return uiData->window.getObject();
}

#if DISTRHO_UI_USE_WEB_VIEW
void UI::PrivateData::webViewMessageCallback(void* const arg, char* const msg)
{
    UI::PrivateData* const uiData = static_cast<UI::PrivateData*>(arg);

    if (std::strncmp(msg, "setparam ", 9) == 0)
    {
        const char* const strindex = msg + 9;
        char* strvalue = nullptr;
        const ulong index = std::strtoul(strindex, &strvalue, 10);
        DISTRHO_SAFE_ASSERT_RETURN(strvalue != nullptr && strindex != strvalue,);

        float value;
        {
            const ScopedSafeLocale ssl;
            value = std::atof(strvalue);
        }
        uiData->setParamCallback(index + uiData->parameterOffset, value);
        return;
    }

    if (std::strncmp(msg, "editparam ", 10) == 0)
    {
        const char* const strindex = msg + 10;
        char* strvalue = nullptr;
        const ulong index = std::strtoul(strindex, &strvalue, 10);
        DISTRHO_SAFE_ASSERT_RETURN(strvalue != nullptr && strindex != strvalue,);

        const bool started = strvalue[0] != '0';
        uiData->editParamCallback(index + uiData->parameterOffset, started);
        return;
    }

   #if DISTRHO_PLUGIN_WANT_STATE
    if (std::strncmp(msg, "setstate ", 9) == 0)
    {
        char* const key = msg + 9;
        char* const sep = std::strchr(key, ' ');
        DISTRHO_SAFE_ASSERT_RETURN(sep != nullptr,);
        *sep = '\0';
        char* const value = sep + 1;

        uiData->setStateCallback(key, value);
        return;
    }

    if (std::strncmp(msg, "reqstatefile ", 13) == 0)
    {
        const char* const key = msg + 13;
        uiData->fileRequestCallback(key);
        return;
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    if (std::strncmp(msg, "sendnote ", 9) == 0)
    {
        const char* const strchannel = msg + 9;
        char* strnote = nullptr;
        char* strvelocity = nullptr;
        char* end = nullptr;

        const ulong channel = std::strtoul(strchannel, &strnote, 10);
        DISTRHO_SAFE_ASSERT_RETURN(strnote != nullptr && strchannel != strnote,);

        const ulong note = std::strtoul(strnote, &strvelocity, 10);
        DISTRHO_SAFE_ASSERT_RETURN(strvelocity != nullptr && strchannel != strvelocity,);

        const ulong velocity = std::strtoul(strvelocity, &end, 10);
        DISTRHO_SAFE_ASSERT_RETURN(end != nullptr && strvelocity != end,);

        uiData->sendNoteCallback(channel, note, velocity);
        return;
    }
   #endif

    d_stderr("UI received unknown message '%s'", msg);
}
#endif

/* ------------------------------------------------------------------------------------------------------------
 * UI */

UI::UI(const uint width, const uint height, const InternalScalingMode internalScalingMode)
    : UIWidget(UI::PrivateData::createNextWindow(this,
               // width
              #ifdef DISTRHO_UI_DEFAULT_WIDTH
               width == 0 ? DISTRHO_UI_DEFAULT_WIDTH :
              #endif
               width,
               // height
              #ifdef DISTRHO_UI_DEFAULT_HEIGHT
               height == 0 ? DISTRHO_UI_DEFAULT_HEIGHT :
              #endif
               height
               )),
      uiData(UI::PrivateData::s_nextPrivateData)
{
    if (width != 0 && height != 0)
    {
        if (internalScalingMode == kInternalScalingMatchingHost)
        {
            d_stdout("enableInternalScalingWithSize %u %u", width, height);
            getWindow().enableInternalScalingWithSize(width, height, true);
            return;
        }
    }

    Widget::setSize(getWindow().getSize());
}

#if DGL_ALLOW_DEPRECATED_METHODS
UI::UI(const uint width, const uint height, const bool automaticallyScaleAndSetAsMinimumSize)
    : UIWidget(UI::PrivateData::createNextWindow(this,
               // width
              #ifdef DISTRHO_UI_DEFAULT_WIDTH
               width == 0 ? DISTRHO_UI_DEFAULT_WIDTH :
              #endif
               width,
               // height
              #ifdef DISTRHO_UI_DEFAULT_HEIGHT
               height == 0 ? DISTRHO_UI_DEFAULT_HEIGHT :
              #endif
               height
               )),
      uiData(UI::PrivateData::s_nextPrivateData)
{
    if (width != 0 && height != 0)
    {
        Widget::setSize(width, height);

        if (automaticallyScaleAndSetAsMinimumSize)
        {
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
            setGeometryConstraints(width, height, true, true, false);
           #if defined(_MSC_VER)
            #pragma warning(pop)
           #elif defined(__clang__)
            #pragma clang diagnostic pop
           #elif defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 460
            #pragma GCC diagnostic pop
           #endif
        }
    }
   #ifdef DISTRHO_UI_DEFAULT_WIDTH
    else
    {
        Widget::setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
    }
   #endif
}
#endif

UI::~UI()
{
   #if DISTRHO_UI_USE_WEB_VIEW
    if (uiData->webview != nullptr)
        webViewDestroy(uiData->webview);
   #endif
}

/* ------------------------------------------------------------------------------------------------------------
 * Host state */

bool UI::isResizable() const noexcept
{
   #if DISTRHO_UI_USER_RESIZABLE
    return uiData->window->isResizable();
   #else
    return false;
   #endif
}

uint UI::getBackgroundColor() const noexcept
{
    return uiData->bgColor;
}

uint UI::getForegroundColor() const noexcept
{
    return uiData->fgColor;
}

double UI::getSampleRate() const noexcept
{
    return uiData->sampleRate;
}

const char* UI::getBundlePath() const noexcept
{
    return uiData->bundlePath;
}

void UI::editParameter(uint32_t index, bool started)
{
    uiData->editParamCallback(index + uiData->parameterOffset, started);
}

void UI::setParameterValue(uint32_t index, float value)
{
    uiData->setParamCallback(index + uiData->parameterOffset, value);
}

#if DISTRHO_PLUGIN_WANT_STATE
void UI::setState(const char* key, const char* value)
{
    uiData->setStateCallback(key, value);
}
#endif

#if DISTRHO_PLUGIN_WANT_STATE
bool UI::requestStateFile(const char* key)
{
    return uiData->fileRequestCallback(key);
}
#endif

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
void UI::sendNote(uint8_t channel, uint8_t note, uint8_t velocity)
{
    uiData->sendNoteCallback(channel, note, velocity);
}
#endif

#if DISTRHO_UI_FILE_BROWSER
bool UI::openFileBrowser(const FileBrowserOptions& options)
{
    return getWindow().openFileBrowser((DGL_NAMESPACE::FileBrowserOptions&)options);
}
#endif

#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
/* ------------------------------------------------------------------------------------------------------------
 * Direct DSP access */

void* UI::getPluginInstancePointer() const noexcept
{
    return uiData->dspPtr;
}
#endif

/* ------------------------------------------------------------------------------------------------------------
 * DSP/Plugin Callbacks */

void UI::parameterChanged(const uint32_t index, const float value)
{
   #if DISTRHO_UI_USE_WEB_VIEW
    if (uiData->webview != nullptr)
    {
        char msg[128];
        {
            const ScopedSafeLocale ssl;
            std::snprintf(msg, sizeof(msg) - 1,
                          "typeof(parameterChanged) === 'function' && parameterChanged(%u,%f)", index, value);
        }
        webViewEvaluateJS(uiData->webview, msg);
    }
   #else
    // unused
    (void)index;
    (void)value;
   #endif
}

#if DISTRHO_PLUGIN_WANT_PROGRAMS
void UI::programLoaded(const uint32_t index)
{
   #if DISTRHO_UI_USE_WEB_VIEW
    if (uiData->webview != nullptr)
    {
        char msg[128];
        std::snprintf(msg, sizeof(msg) - 1,
                      "typeof(programLoaded) === 'function' && programLoaded(%u)", index);
        webViewEvaluateJS(uiData->webview, msg);
    }
   #else
    // unused
    (void)index;
   #endif
}
#endif

#if DISTRHO_PLUGIN_WANT_STATE
void UI::stateChanged(const char* const key, const char* const value)
{
   #if DISTRHO_UI_USE_WEB_VIEW
    if (uiData->webview != nullptr)
    {
        const size_t keylen = std::strlen(key);
        const size_t valuelen = std::strlen(value);
        const size_t msglen = keylen + valuelen + 60;
        if (char* const msg = static_cast<char*>(std::malloc(msglen)))
        {
            // TODO escape \\'
            std::snprintf(msg, msglen - 1,
                          "typeof(stateChanged) === 'function' && stateChanged('%s','%s')", key, value);
            msg[msglen - 1] = '\0';
            webViewEvaluateJS(uiData->webview, msg);
            std::free(msg);
        }
    }
   #else
    // unused
    (void)key;
    (void)value;
   #endif
}
#endif

/* ------------------------------------------------------------------------------------------------------------
 * DSP/Plugin Callbacks (optional) */

void UI::sampleRateChanged(const double sampleRate)
{
   #if DISTRHO_UI_USE_WEB_VIEW
    if (uiData->webview != nullptr)
    {
        char msg[128];
        {
            const ScopedSafeLocale ssl;
            std::snprintf(msg, sizeof(msg) - 1,
                          "typeof(sampleRateChanged) === 'function' && sampleRateChanged(%f)", sampleRate);
        }
        webViewEvaluateJS(uiData->webview, msg);
    }
   #else
    // unused
    (void)sampleRate;
   #endif
}

/* ------------------------------------------------------------------------------------------------------------
 * UI Callbacks (optional) */

void UI::uiScaleFactorChanged(double)
{
}

std::vector<DGL_NAMESPACE::ClipboardDataOffer> UI::getClipboardDataOfferTypes()
{
    return uiData->window->getClipboardDataOfferTypes();
}

uint32_t UI::uiClipboardDataOffer()
{
    std::vector<DGL_NAMESPACE::ClipboardDataOffer> offers(uiData->window->getClipboardDataOfferTypes());

    for (std::vector<DGL_NAMESPACE::ClipboardDataOffer>::iterator it=offers.begin(), end=offers.end(); it != end;++it)
    {
        const DGL_NAMESPACE::ClipboardDataOffer offer = *it;
        if (std::strcmp(offer.type, "text/plain") == 0)
            return offer.id;
    }

    return 0;
}

void UI::uiFocus(bool, DGL_NAMESPACE::CrossingMode)
{
}

#if DGL_ALLOW_DEPRECATED_METHODS
void UI::uiReshape(uint, uint)
{
}
#endif

#if DISTRHO_UI_FILE_BROWSER
void UI::uiFileBrowserSelected(const char*)
{
}
#endif

/* ------------------------------------------------------------------------------------------------------------
 * UI Resize Handling, internal */

void UI::onResize(const ResizeEvent& ev)
{
    UIWidget::onResize(ev);

   #if ! DISTRHO_UI_USES_SIZE_REQUEST
    if (uiData->initializing)
        return;

    const uint width = ev.size.getWidth();
    const uint height = ev.size.getHeight();
    uiData->setSizeCallback(width, height);
   #endif
}

// NOTE: only used for CLAP and VST3
void UI::requestSizeChange(const uint width, const uint height)
{
   #if DISTRHO_UI_USES_SIZE_REQUEST
    if (uiData->initializing)
        uiData->window->setSizeFromHost(width, height);
    else
        uiData->setSizeCallback(width, height);
   #else
    // unused
    (void)width;
    (void)height;
   #endif
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
