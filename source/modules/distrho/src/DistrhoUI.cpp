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

#include "src/DistrhoPluginChecks.h"
#include "src/DistrhoDefines.h"

#include <cstddef>

#ifdef DISTRHO_PROPER_CPP11_SUPPORT
# include <cstdint>
#else
# include <stdint.h>
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
# define FILE_BROWSER_DIALOG_DISTRHO_NAMESPACE
START_NAMESPACE_DISTRHO
# include "../extra/FileBrowserDialogImpl.hpp"
END_NAMESPACE_DISTRHO
# include "../extra/FileBrowserDialogImpl.cpp"
#endif

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
# if defined(DISTRHO_OS_WINDOWS)
#  include <winsock2.h>
#  include <windows.h>
# elif defined(HAVE_X11)
#  include <X11/Xresource.h>
# endif
#else
# include "src/TopLevelWidgetPrivateData.hpp"
# include "src/WindowPrivateData.hpp"
#endif

#include "DistrhoUIPrivateData.hpp"

START_NAMESPACE_DISTRHO

/* ------------------------------------------------------------------------------------------------------------
 * Static data, see DistrhoUIInternal.hpp */

const char* g_nextBundlePath  = nullptr;
#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
uintptr_t   g_nextWindowId    = 0;
double      g_nextScaleFactor = 1.0;
#endif

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
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

#if defined(DISTRHO_OS_WINDOWS)
    if (const HMODULE Shcore = LoadLibraryA("Shcore.dll"))
    {
        typedef HRESULT(WINAPI* PFN_GetProcessDpiAwareness)(HANDLE, DWORD*);
        typedef HRESULT(WINAPI* PFN_GetScaleFactorForMonitor)(HMONITOR, DWORD*);

# if defined(__GNUC__) && (__GNUC__ >= 9)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcast-function-type"
# endif
        const PFN_GetProcessDpiAwareness GetProcessDpiAwareness
            = (PFN_GetProcessDpiAwareness)GetProcAddress(Shcore, "GetProcessDpiAwareness");
        const PFN_GetScaleFactorForMonitor GetScaleFactorForMonitor
            = (PFN_GetScaleFactorForMonitor)GetProcAddress(Shcore, "GetScaleFactorForMonitor");
# if defined(__GNUC__) && (__GNUC__ >= 9)
#  pragma GCC diagnostic pop
# endif

        DWORD dpiAware = 0;
        DWORD scaleFactor = 100;
        if (GetProcessDpiAwareness && GetScaleFactorForMonitor
            && GetProcessDpiAwareness(nullptr, &dpiAware) == 0 && dpiAware != 0)
        {
            const HMONITOR hMon = parentWindowHandle != 0
                                ? MonitorFromWindow((HWND)parentWindowHandle, MONITOR_DEFAULTTOPRIMARY)
                                : MonitorFromPoint(POINT{0,0}, MONITOR_DEFAULTTOPRIMARY);
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

#endif

/* ------------------------------------------------------------------------------------------------------------
 * UI::PrivateData special handling */

UI::PrivateData* UI::PrivateData::s_nextPrivateData = nullptr;

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
ExternalWindow::PrivateData
#else
PluginWindow&
#endif
UI::PrivateData::createNextWindow(UI* const ui, const uint width, const uint height)
{
    UI::PrivateData* const pData = s_nextPrivateData;
#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
    pData->window = new PluginWindow(ui, pData->app);
    ExternalWindow::PrivateData ewData;
    ewData.parentWindowHandle = pData->winId;
    ewData.width = width;
    ewData.height = height;
    ewData.scaleFactor = pData->scaleFactor != 0.0 ? pData->scaleFactor : getDesktopScaleFactor(pData->winId);
    ewData.title = DISTRHO_PLUGIN_NAME;
    ewData.isStandalone = DISTRHO_UI_IS_STANDALONE;
    return ewData;
#else
    pData->window = new PluginWindow(ui, pData->app, pData->winId, width, height, pData->scaleFactor);

    // If there are no callbacks, this is most likely a temporary window, so ignore idle callbacks
    if (pData->callbacksPtr == nullptr)
        pData->window->setIgnoreIdleCallbacks();

    return pData->window.getObject();
#endif
}

/* ------------------------------------------------------------------------------------------------------------
 * UI */

UI::UI(const uint width, const uint height, const bool automaticallyScaleAndSetAsMinimumSize)
    : UIWidget(UI::PrivateData::createNextWindow(this, width, height)),
      uiData(UI::PrivateData::s_nextPrivateData)
{
#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
    if (width != 0 && height != 0)
    {
        Widget::setSize(width, height);

        if (automaticallyScaleAndSetAsMinimumSize)
            setGeometryConstraints(width, height, true, true, true);
    }
#else
    // unused
    (void)automaticallyScaleAndSetAsMinimumSize;
#endif
}

UI::~UI()
{
}

/* ------------------------------------------------------------------------------------------------------------
 * Host state */

bool UI::isResizable() const noexcept
{
#if DISTRHO_UI_USER_RESIZABLE
# if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
    return true;
# else
    return uiData->window->isResizable();
# endif
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

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
/* ------------------------------------------------------------------------------------------------------------
 * External UI helpers (static calls) */

const char* UI::getNextBundlePath() noexcept
{
    return g_nextBundlePath;
}

double UI::getNextScaleFactor() noexcept
{
    return g_nextScaleFactor;
}

# if DISTRHO_PLUGIN_HAS_EMBED_UI
uintptr_t UI::getNextWindowId() noexcept
{
    return g_nextWindowId;
}
# endif
#endif // DISTRHO_PLUGIN_HAS_EXTERNAL_UI

/* ------------------------------------------------------------------------------------------------------------
 * DSP/Plugin Callbacks (optional) */

void UI::sampleRateChanged(double)
{
}

/* ------------------------------------------------------------------------------------------------------------
 * UI Callbacks (optional) */

void UI::uiScaleFactorChanged(double)
{
}

#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
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

void UI::uiReshape(uint, uint)
{
    // NOTE this must be the same as Window::onReshape
    pData->fallbackOnResize();
}
#endif // !DISTRHO_PLUGIN_HAS_EXTERNAL_UI

#if DISTRHO_UI_FILE_BROWSER
void UI::uiFileBrowserSelected(const char*)
{
}
#endif

/* ------------------------------------------------------------------------------------------------------------
 * UI Resize Handling, internal */

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
void UI::sizeChanged(const uint width, const uint height)
{
    UIWidget::sizeChanged(width, height);

    uiData->setSizeCallback(width, height);
}
#else
void UI::onResize(const ResizeEvent& ev)
{
    UIWidget::onResize(ev);

#ifndef DISTRHO_PLUGIN_TARGET_VST3
    if (uiData->initializing)
        return;

    const uint width = ev.size.getWidth();
    const uint height = ev.size.getHeight();
    uiData->setSizeCallback(width, height);
#endif
}

// NOTE: only used for VST3
void UI::requestSizeChange(const uint width, const uint height)
{
# ifdef DISTRHO_PLUGIN_TARGET_VST3
    if (uiData->initializing)
        uiData->window->setSizeForVST3(width, height);
    else
        uiData->setSizeCallback(width, height);
# else
    // unused
    (void)width;
    (void)height;
# endif
}
#endif

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
