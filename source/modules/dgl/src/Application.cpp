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

#include "ApplicationPrivateData.hpp"

#if defined(__EMSCRIPTEN__)
# include <emscripten/emscripten.h>
#elif defined(DISTRHO_OS_MAC)
# include <CoreFoundation/CoreFoundation.h>
#endif

START_NAMESPACE_DGL

/* define webview start */
#if defined(HAVE_X11) && defined(DISTRHO_OS_LINUX) && defined(DGL_USE_WEB_VIEW)
int dpf_webview_start(int argc, char* argv[]);
#endif

// --------------------------------------------------------------------------------------------------------------------
// build config sentinels

#define BUILD_CONFIG_SENTINEL(NAME) \
   DISTRHO_JOIN_MACRO(_, NAME)::DISTRHO_JOIN_MACRO(_, NAME)() noexcept : ok(false) {}

#ifdef DPF_DEBUG
BUILD_CONFIG_SENTINEL(fail_to_link_is_mismatch_dpf_debug_on)
#else
BUILD_CONFIG_SENTINEL(fail_to_link_is_mismatch_dpf_debug_off)
#endif

#ifdef DGL_USE_FILE_BROWSER
BUILD_CONFIG_SENTINEL(fail_to_link_is_mismatch_dgl_use_file_browser_on)
#else
BUILD_CONFIG_SENTINEL(fail_to_link_is_mismatch_dgl_use_file_browser_off)
#endif

#ifdef DGL_USE_WEB_VIEW
BUILD_CONFIG_SENTINEL(fail_to_link_is_mismatch_dgl_use_web_view_on)
#else
BUILD_CONFIG_SENTINEL(fail_to_link_is_mismatch_dgl_use_web_view_off)
#endif

#ifdef DGL_NO_SHARED_RESOURCES
BUILD_CONFIG_SENTINEL(fail_to_link_is_mismatch_dgl_no_shared_resources_on)
#else
BUILD_CONFIG_SENTINEL(fail_to_link_is_mismatch_dgl_no_shared_resources_off)
#endif

#undef BUILD_CONFIG_SENTINEL

static inline
bool dpf_check_build_status() noexcept
{
   return (
     #ifdef DPF_DEBUG
      fail_to_link_is_mismatch_dpf_debug_on.ok &&
     #else
      fail_to_link_is_mismatch_dpf_debug_off.ok &&
     #endif
     #ifdef DGL_USE_FILE_BROWSER
      fail_to_link_is_mismatch_dgl_use_file_browser_on.ok &&
     #else
      fail_to_link_is_mismatch_dgl_use_file_browser_off.ok &&
     #endif
     #ifdef DGL_USE_WEB_VIEW
      fail_to_link_is_mismatch_dgl_use_web_view_on.ok &&
     #else
      fail_to_link_is_mismatch_dgl_use_web_view_off.ok &&
     #endif
     #ifdef DGL_NO_SHARED_RESOURCES
      fail_to_link_is_mismatch_dgl_no_shared_resources_on.ok &&
     #else
      fail_to_link_is_mismatch_dgl_no_shared_resources_off.ok &&
     #endif
      true
   );
}

// --------------------------------------------------------------------------------------------------------------------

#ifdef __EMSCRIPTEN__
static void app_idle(void* const app)
{
    static_cast<Application*>(app)->idle();
}
#endif

Application::Application(const bool isStandalone, const Type type)
    : pData(new PrivateData(isStandalone, type))
{
    // build config sentinels
   #ifdef DPF_DEBUG
    fail_to_link_is_mismatch_dpf_debug_on.ok = true;
   #else
    fail_to_link_is_mismatch_dpf_debug_off.ok = true;
   #endif
   #ifdef DGL_USE_FILE_BROWSER
    fail_to_link_is_mismatch_dgl_use_file_browser_on.ok = true;
   #else
    fail_to_link_is_mismatch_dgl_use_file_browser_off.ok = true;
   #endif
   #ifdef DGL_USE_WEB_VIEW
    fail_to_link_is_mismatch_dgl_use_web_view_on.ok = true;
   #else
    fail_to_link_is_mismatch_dgl_use_web_view_off.ok = true;
   #endif
   #ifdef DGL_NO_SHARED_RESOURCES
    fail_to_link_is_mismatch_dgl_no_shared_resources_on.ok = true;
   #else
    fail_to_link_is_mismatch_dgl_no_shared_resources_off.ok = true;
   #endif
    DISTRHO_SAFE_ASSERT(dpf_check_build_status());
}

Application::Application(int argc, char* argv[])
    : pData(new PrivateData(true, kTypeAuto))
{
   #if defined(HAVE_X11) && defined(DISTRHO_OS_LINUX) && defined(DGL_USE_WEB_VIEW)
    if (argc >= 2 && std::strcmp(argv[1], "dpf-ld-linux-webview") == 0)
        std::exit(dpf_webview_start(argc, argv));
   #else
    // unused
    (void)argc;
    (void)argv;
   #endif

    // build config sentinels
   #ifdef DPF_DEBUG
    fail_to_link_is_mismatch_dpf_debug_on.ok = true;
   #else
    fail_to_link_is_mismatch_dpf_debug_off.ok = true;
   #endif
   #ifdef DGL_USE_FILE_BROWSER
    fail_to_link_is_mismatch_dgl_use_file_browser_on.ok = true;
   #else
    fail_to_link_is_mismatch_dgl_use_file_browser_off.ok = true;
   #endif
   #ifdef DGL_USE_WEB_VIEW
    fail_to_link_is_mismatch_dgl_use_web_view_on.ok = true;
   #else
    fail_to_link_is_mismatch_dgl_use_web_view_off.ok = true;
   #endif
   #ifdef DGL_NO_SHARED_RESOURCES
    fail_to_link_is_mismatch_dgl_no_shared_resources_on.ok = true;
   #else
    fail_to_link_is_mismatch_dgl_no_shared_resources_off.ok = true;
   #endif
    DISTRHO_SAFE_ASSERT(dpf_check_build_status());
}

Application::~Application()
{
    delete pData;
}

void Application::idle()
{
    pData->idle(0);
}

void Application::exec(const uint idleTimeInMs)
{
    DISTRHO_SAFE_ASSERT_RETURN(pData->isStandalone,);

#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop_arg(app_idle, this, 0, true);
#elif defined(DISTRHO_OS_MAC)
    const CFTimeInterval idleTimeInSecs = static_cast<CFTimeInterval>(idleTimeInMs) / 1000;

    while (! pData->isQuitting)
    {
        pData->idle(0);

        if (CFRunLoopRunInMode(kCFRunLoopDefaultMode, idleTimeInSecs, true) == kCFRunLoopRunFinished)
            break;
    }
#else
    while (! pData->isQuitting)
        pData->idle(idleTimeInMs);
#endif
}

void Application::quit()
{
    pData->quit();
}

bool Application::isQuitting() const noexcept
{
    return pData->isQuitting || pData->isQuittingInNextCycle;
}

bool Application::isStandalone() const noexcept
{
    return pData->isStandalone;
}

double Application::getTime() const
{
    return pData->getTime();
}

Application::Type Application::getType() const noexcept
{
    return pData->isModern ? kTypeModern : kTypeClassic;
}

void Application::addIdleCallback(IdleCallback* const callback)
{
    DISTRHO_SAFE_ASSERT_RETURN(callback != nullptr,)

    pData->idleCallbacks.push_back(callback);
}

void Application::removeIdleCallback(IdleCallback* const callback)
{
    DISTRHO_SAFE_ASSERT_RETURN(callback != nullptr,)

    pData->idleCallbacks.remove(callback);
}

void Application::setClassName(const char* const name)
{
    pData->setClassName(name);
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
