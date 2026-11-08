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

#include "WindowPrivateData.hpp"
#include "TopLevelWidgetPrivateData.hpp"

#include "pugl.hpp"

// #define DGL_DEBUG_EVENTS

#if defined(DEBUG) && defined(DGL_DEBUG_EVENTS)
# ifdef DISTRHO_PROPER_CPP11_SUPPORT
#  include <cinttypes>
# else
#  include <inttypes.h>
# endif
#endif

#ifdef DISTRHO_OS_WINDOWS
# include <windows.h>
#endif

START_NAMESPACE_DGL

#ifdef DISTRHO_OS_WINDOWS
# include "pugl-upstream/src/win.h"
#endif

#ifdef DGL_DEBUG_EVENTS
# define DGL_DBG(msg)  d_stdout("%s", msg);
# define DGL_DBGp(...) d_stdout(__VA_ARGS__);
#else
# define DGL_DBG(msg)
# define DGL_DBGp(...)
#endif

#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480

#define FOR_EACH_TOP_LEVEL_WIDGET(it) \
  for (std::list<TopLevelWidget*>::iterator it = topLevelWidgets.begin(); it != topLevelWidgets.end(); ++it)

#define FOR_EACH_TOP_LEVEL_WIDGET_INV(rit) \
  for (std::list<TopLevelWidget*>::reverse_iterator rit = topLevelWidgets.rbegin(); rit != topLevelWidgets.rend(); ++rit)

// -----------------------------------------------------------------------

static double getScaleFactor(const PuglView* const view)
{
    // allow custom scale for testing
    if (const char* const scale = getenv("DPF_SCALE_FACTOR"))
        return std::max(1.0, std::atof(scale));

    if (view != nullptr)
        return puglGetScaleFactor(view);

    return 1.0;
}

static PuglView* puglNewViewWithTransientParent(PuglWorld* const world, PuglView* const transientParentView)
{
    if (world == nullptr)
        return nullptr;

    if (PuglView* const view = puglNewView(world))
    {
        puglSetTransientParent(view, puglGetNativeView(transientParentView));
        return view;
    }

    return nullptr;
}

static PuglView* puglNewViewWithParentWindow(PuglWorld* const world, const uintptr_t parentWindowHandle)
{
    if (world == nullptr)
        return nullptr;

    if (PuglView* const view = puglNewView(world))
    {
        puglSetParent(view, parentWindowHandle);

        if (parentWindowHandle != 0)
            puglSetPositionHint(view, PUGL_DEFAULT_POSITION, 0, 0);

        return view;
    }

    return nullptr;
}

// -----------------------------------------------------------------------

Window::PrivateData::PrivateData(Application& a, Window* const s)
    : app(a),
      appData(a.pData),
      self(s),
      view(appData->world != nullptr ? puglNewView(appData->world) : nullptr),
      topLevelWidgets(),
      isClosed(true),
      isVisible(false),
      isEmbed(false),
      usesScheduledRepaints(false),
      usesSizeRequest(false),
      scaleFactor(DGL_NAMESPACE::getScaleFactor(view)),
      autoScaling(false),
      autoScaleFactor(1.0),
      baseWidth(0),
      baseHeight(0),
      ignoreIdleCallbacks(false),
      waitingForClipboardData(false),
      waitingForClipboardEvents(false),
      clipboardTypeId(0),
      filenameToRenderInto(nullptr),
     #ifdef DGL_USE_FILE_BROWSER
      fileBrowserHandle(nullptr),
     #endif
     #ifdef DGL_USE_WEB_VIEW
      webViewHandle(nullptr),
     #endif
      modal()
{
    initPre(DEFAULT_WIDTH, DEFAULT_HEIGHT, false);
}

Window::PrivateData::PrivateData(Application& a, Window* const s, PrivateData* const ppData)
    : app(a),
      appData(a.pData),
      self(s),
      view(puglNewViewWithTransientParent(appData->world, ppData->view)),
      topLevelWidgets(),
      isClosed(true),
      isVisible(false),
      isEmbed(false),
      usesScheduledRepaints(false),
      usesSizeRequest(false),
      scaleFactor(ppData->scaleFactor),
      autoScaling(false),
      autoScaleFactor(1.0),
      baseWidth(0),
      baseHeight(0),
      ignoreIdleCallbacks(false),
      waitingForClipboardData(false),
      waitingForClipboardEvents(false),
      clipboardTypeId(0),
      filenameToRenderInto(nullptr),
     #ifdef DGL_USE_FILE_BROWSER
      fileBrowserHandle(nullptr),
     #endif
     #ifdef DGL_USE_WEB_VIEW
      webViewHandle(nullptr),
     #endif
      modal(ppData)
{
    initPre(DEFAULT_WIDTH, DEFAULT_HEIGHT, false);
}

Window::PrivateData::PrivateData(Application& a, Window* const s,
                                 const uintptr_t parentWindowHandle,
                                 const double scale, const bool resizable)
    : app(a),
      appData(a.pData),
      self(s),
      view(puglNewViewWithParentWindow(appData->world, parentWindowHandle)),
      topLevelWidgets(),
      isClosed(parentWindowHandle == 0),
      isVisible(parentWindowHandle != 0),
      isEmbed(parentWindowHandle != 0),
      usesScheduledRepaints(false),
      usesSizeRequest(false),
      scaleFactor(scale != 0.0 ? scale : DGL_NAMESPACE::getScaleFactor(view)),
      autoScaling(false),
      autoScaleFactor(1.0),
      baseWidth(0),
      baseHeight(0),
      ignoreIdleCallbacks(false),
      waitingForClipboardData(false),
      waitingForClipboardEvents(false),
      clipboardTypeId(0),
      filenameToRenderInto(nullptr),
     #ifdef DGL_USE_FILE_BROWSER
      fileBrowserHandle(nullptr),
     #endif
     #ifdef DGL_USE_WEB_VIEW
      webViewHandle(nullptr),
     #endif
      modal()
{
    initPre(DEFAULT_WIDTH, DEFAULT_HEIGHT, resizable);
}

Window::PrivateData::PrivateData(Application& a, Window* const s,
                                 const uintptr_t parentWindowHandle,
                                 const uint width, const uint height,
                                 const double scale, const bool resizable,
                                 const bool _usesScheduledRepaints,
                                 const bool _usesSizeRequest)
    : app(a),
      appData(a.pData),
      self(s),
      view(puglNewViewWithParentWindow(appData->world, parentWindowHandle)),
      topLevelWidgets(),
      isClosed(parentWindowHandle == 0),
      isVisible(parentWindowHandle != 0 && view != nullptr),
      isEmbed(parentWindowHandle != 0),
      usesScheduledRepaints(_usesScheduledRepaints),
      usesSizeRequest(_usesSizeRequest),
      scaleFactor(scale != 0.0 ? scale : DGL_NAMESPACE::getScaleFactor(view)),
      autoScaling(false),
      autoScaleFactor(1.0),
      baseWidth(0),
      baseHeight(0),
      ignoreIdleCallbacks(false),
      waitingForClipboardData(false),
      waitingForClipboardEvents(false),
      clipboardTypeId(0),
      filenameToRenderInto(nullptr),
     #ifdef DGL_USE_FILE_BROWSER
      fileBrowserHandle(nullptr),
     #endif
     #ifdef DGL_USE_WEB_VIEW
      webViewHandle(nullptr),
     #endif
      modal()
{
    initPre(width != 0 ? width : DEFAULT_WIDTH, height != 0 ? height : DEFAULT_HEIGHT, resizable);
}

Window::PrivateData::~PrivateData()
{
    appData->idleCallbacks.remove(this);
    appData->windows.remove(self);
    std::free(filenameToRenderInto);

    if (view == nullptr)
        return;

    if (isEmbed)
    {
       #ifdef DGL_USE_FILE_BROWSER
        if (fileBrowserHandle != nullptr)
            fileBrowserClose(fileBrowserHandle);
       #endif
       #ifdef DGL_USE_WEB_VIEW
        if (webViewHandle != nullptr)
            webViewDestroy(webViewHandle);
       #endif
        puglHide(view);
        appData->oneWindowClosed();
        isClosed = true;
        isVisible = false;
    }

    destroyContext();
    puglFreeView(view);
}

// -----------------------------------------------------------------------

void Window::PrivateData::initPre(const uint width, const uint height, const bool resizable)
{
    appData->windows.push_back(self);
    appData->idleCallbacks.push_back(this);
    memset(graphicsContext, 0, sizeof(graphicsContext));

    if (view == nullptr)
    {
        d_stderr2("Failed to create Pugl view, everything will fail!");
        return;
    }

    puglSetMatchingBackendForCurrentBuild(view);
    puglSetHandle(view, this);

    puglSetViewHint(view, PUGL_RESIZABLE, resizable ? PUGL_TRUE : PUGL_FALSE);
    puglSetViewHint(view, PUGL_IGNORE_KEY_REPEAT, PUGL_FALSE);
   #if defined(DGL_USE_RGBA) && DGL_USE_RGBA
    puglSetViewHint(view, PUGL_DEPTH_BITS, 24);
   #else
    puglSetViewHint(view, PUGL_DEPTH_BITS, 16);
   #endif
    puglSetViewHint(view, PUGL_STENCIL_BITS, 8);

    // PUGL_SAMPLES ??
    puglSetEventFunc(view, puglEventCallback);

    // setting size triggers system-level calls, do it last
    puglSetSizeAndDefault(view, width, height);
}

bool Window::PrivateData::initPost()
{
    if (view == nullptr)
        return false;

    // create view now, as a few methods we allow devs to use require it
    if (puglRealize(view) != PUGL_SUCCESS)
    {
        view = nullptr;
        d_stderr2("Failed to realize Pugl view, everything will fail!");
        return false;
    }

    if (isEmbed)
    {
        appData->oneWindowShown();
        puglShow(view, PUGL_SHOW_PASSIVE);
    }

    return true;
}

// -----------------------------------------------------------------------

void Window::PrivateData::close()
{
    DGL_DBG("Window close\n");
    // DGL_DBGp("Window close DBG %i %i %p\n", isEmbed, isClosed, appData);

    if (isEmbed || isClosed)
        return;

    isClosed = true;
    hide();
    appData->oneWindowClosed();
}

// -----------------------------------------------------------------------

void Window::PrivateData::show()
{
    if (isVisible)
    {
        DGL_DBG("Window show matches current visible state, ignoring request\n");
        return;
    }
    if (isEmbed)
    {
        DGL_DBG("Window show cannot be called when embedded\n");
        return;
    }

    DGL_DBG("Window show called\n");

    if (view == nullptr)
        return;

    if (isClosed)
    {
        isClosed = false;
        appData->oneWindowShown();

#if defined(DISTRHO_OS_WINDOWS)
        puglWin32ShowCentered(view);
#elif defined(DISTRHO_OS_MAC)
        puglMacOSShowCentered(view);
#else
        puglShow(view, PUGL_SHOW_RAISE);
#endif
    }
    else
    {
#ifdef DISTRHO_OS_WINDOWS
        puglWin32RestoreWindow(view);
#else
        puglShow(view, PUGL_SHOW_RAISE);
#endif
    }

    isVisible = true;
}

void Window::PrivateData::hide()
{
    if (isEmbed)
    {
        DGL_DBG("Window hide cannot be called when embedded\n");
        return;
    }
    if (! isVisible)
    {
        DGL_DBG("Window hide matches current visible state, ignoring request\n");
        return;
    }

    DGL_DBG("Window hide called\n");

    if (modal.enabled)
        stopModal();

   #ifdef DGL_USE_FILE_BROWSER
    if (fileBrowserHandle != nullptr)
    {
        fileBrowserClose(fileBrowserHandle);
        fileBrowserHandle = nullptr;
    }
   #endif

   #ifdef DGL_USE_WEB_VIEW
    if (webViewHandle != nullptr)
    {
        webViewDestroy(webViewHandle);
        webViewHandle = nullptr;
    }
   #endif

    puglHide(view);

    isVisible = false;
}

// -----------------------------------------------------------------------

void Window::PrivateData::focus()
{
    if (view == nullptr)
        return;

    if (! isEmbed)
        puglRaiseWindow(view);

    puglGrabFocus(view);
}

// -----------------------------------------------------------------------

void Window::PrivateData::setResizable(const bool resizable)
{
    DISTRHO_SAFE_ASSERT_RETURN(! isEmbed,);

    DGL_DBG("Window setResizable called\n");

    puglSetResizable(view, resizable);
}

// --------------------------------------------------------------------------------------------------------------------

const GraphicsContext& Window::PrivateData::getGraphicsContext() const noexcept
{
    return reinterpret_cast<const GraphicsContext&>(graphicsContext);
}

// -----------------------------------------------------------------------

void Window::PrivateData::idleCallback()
{
#ifdef DGL_USE_FILE_BROWSER
    if (fileBrowserHandle != nullptr && fileBrowserIdle(fileBrowserHandle))
    {
        self->onFileSelected(fileBrowserGetPath(fileBrowserHandle));
        fileBrowserClose(fileBrowserHandle);
        fileBrowserHandle = nullptr;
    }
#endif

#ifdef DGL_USE_WEB_VIEW
    if (webViewHandle != nullptr)
        webViewIdle(webViewHandle);
#endif
}

// -----------------------------------------------------------------------
// idle callback stuff

bool Window::PrivateData::addIdleCallback(IdleCallback* const callback, const uint timerFrequencyInMs)
{
    if (ignoreIdleCallbacks || view == nullptr)
        return false;

    if (timerFrequencyInMs == 0)
    {
        appData->idleCallbacks.push_back(callback);
        return true;
    }

    return puglStartTimer(view, (uintptr_t)callback, static_cast<double>(timerFrequencyInMs) / 1000.0) == PUGL_SUCCESS;
}

bool Window::PrivateData::removeIdleCallback(IdleCallback* const callback)
{
    if (ignoreIdleCallbacks || view == nullptr)
        return false;

    if (std::find(appData->idleCallbacks.begin(),
                  appData->idleCallbacks.end(), callback) != appData->idleCallbacks.end())
    {
        appData->idleCallbacks.remove(callback);
        return true;
    }

    return puglStopTimer(view, (uintptr_t)callback) == PUGL_SUCCESS;
}

#ifdef DGL_USE_FILE_BROWSER
// -----------------------------------------------------------------------
// file browser dialog

bool Window::PrivateData::openFileBrowser(const FileBrowserOptions& options)
{
    if (fileBrowserHandle != nullptr)
        fileBrowserClose(fileBrowserHandle);

    FileBrowserOptions options2 = options;

    if (options2.title == nullptr)
        options2.title = puglGetViewString(view, PUGL_WINDOW_TITLE);

    options2.className = puglGetViewString(view, PUGL_CLASS_NAME);

    fileBrowserHandle = fileBrowserCreate(isEmbed,
                                          puglGetNativeView(view),
                                         #if DGL_ALLOW_DEPRECATED_METHODS
                                          autoScaling ? autoScaleFactor :
                                         #endif
                                          scaleFactor,
                                          options2);

    return fileBrowserHandle != nullptr;
}
#endif // DGL_USE_FILE_BROWSER

#ifdef DGL_USE_WEB_VIEW
// -----------------------------------------------------------------------
// file browser dialog

bool Window::PrivateData::createWebView(const char* const url, const DGL_NAMESPACE::WebViewOptions& options)
{
    if (webViewHandle != nullptr)
        webViewDestroy(webViewHandle);

    const PuglArea size = puglGetSizeHint(view, PUGL_CURRENT_SIZE);
    uint initialWidth = size.width - options.offset.x;
    uint initialHeight = size.height - options.offset.y;

    webViewOffset = Point<int>(options.offset.x, options.offset.y);

    webViewHandle = webViewCreate(url,
                                  puglGetNativeView(view),
                                  initialWidth,
                                  initialHeight,
                                 #if DGL_ALLOW_DEPRECATED_METHODS
                                  autoScaling ? autoScaleFactor :
                                 #endif
                                  scaleFactor,
                                  options);

    return webViewHandle != nullptr;
}
#endif // DGL_USE_WEB_VIEW

// -----------------------------------------------------------------------
// modal handling

void Window::PrivateData::startModal()
{
    DGL_DBG("Window modal loop starting...");
    DISTRHO_SAFE_ASSERT_RETURN(modal.parent != nullptr, show());

    // activate modal mode for this window
    modal.enabled = true;

    // make parent give focus to us
    modal.parent->modal.child = this;

    // make sure both parent and ourselves are visible
    modal.parent->show();
    show();

#ifdef DISTRHO_OS_MAC
    puglMacOSAddChildWindow(modal.parent->view, view);
#endif

    DGL_DBG("Ok\n");
}

void Window::PrivateData::stopModal()
{
    DGL_DBG("Window modal loop stopping...");

    // deactivate modal mode
    modal.enabled = false;

    // safety checks, make sure we have a parent and we are currently active as the child to give focus to
    if (modal.parent == nullptr)
        return;
    if (modal.parent->modal.child != this)
        return;

#ifdef DISTRHO_OS_MAC
    puglMacOSRemoveChildWindow(modal.parent->view, view);
#endif

    // stop parent from giving focus to us, so it behaves like normal
    modal.parent->modal.child = nullptr;

    // refocus main window after closing child
    if (! modal.parent->isClosed)
    {
        const Widget::MotionEvent ev;
        modal.parent->onPuglMotion(ev);
        modal.parent->focus();
    }

    DGL_DBG("Ok\n");
}

void Window::PrivateData::runAsModal(const bool blockWait)
{
    DGL_DBGp("Window::PrivateData::runAsModal %i\n", blockWait);
    startModal();

    if (blockWait)
    {
        DISTRHO_SAFE_ASSERT_RETURN(appData->isStandalone,);

        while (isVisible && modal.enabled)
            appData->idle(10);

        stopModal();
    }
    else
    {
        appData->idle(0);
    }
}

// -----------------------------------------------------------------------
// pugl events

void Window::PrivateData::onPuglConfigure(const uint width, const uint height)
{
    DISTRHO_SAFE_ASSERT_INT2_RETURN(width > 1 && height > 1, width, height,);

    DGL_DBGp("PUGL: onReshape : %d %d\n", width, height);

    createContextIfNeeded();

    if (autoScaling)
    {
        const double scaleHorizontal = width  / static_cast<double>(baseWidth);
        const double scaleVertical   = height / static_cast<double>(baseHeight);
        autoScaleFactor = scaleHorizontal < scaleVertical ? scaleHorizontal : scaleVertical;
    }
    else
    {
        autoScaleFactor = 1.0;
    }

    const uint uwidth = d_roundToUnsignedInt(width / autoScaleFactor);
    const uint uheight = d_roundToUnsignedInt(height / autoScaleFactor);

   #ifdef DGL_USE_WEB_VIEW
    if (webViewHandle != nullptr)
        webViewResize(webViewHandle,
                      uwidth - webViewOffset.getX(),
                      uheight - webViewOffset.getY(),
                     #if DGL_ALLOW_DEPRECATED_METHODS
                      autoScaling ? autoScaleFactor :
                     #endif
                      scaleFactor);
   #endif

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
    self->onReshape(uwidth, uheight);
   #if defined(_MSC_VER)
    #pragma warning(pop)
   #elif defined(__clang__)
    #pragma clang diagnostic pop
   #elif defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 460
    #pragma GCC diagnostic pop
   #endif
  #endif

    FOR_EACH_TOP_LEVEL_WIDGET(it)
    {
        TopLevelWidget* const widget = *it;

        /* Some special care here, we call Widget::setSize instead of the TopLevelWidget one.
         * This is because we want TopLevelWidget::setSize to handle both window and widget size,
         * but we dont want to change window size here, because we are the window..
         * So we just call the Widget specific method manually.
         *
         * Alternatively, we could expose a resize function on the pData, like done with the display function.
         * But there is nothing extra we need to do in there, so this works fine.
         */
        ((Widget*)widget)->setSize(uwidth, uheight);
    }

    // always repaint after a resize
    puglObscureView(view);
}

void Window::PrivateData::onPuglExpose()
{
    // DGL_DBG("PUGL: onPuglExpose\n");

    puglOnDisplayPrepare(view);

    startContext();

    FOR_EACH_TOP_LEVEL_WIDGET(it)
    {
        TopLevelWidget* const widget(*it);

        if (widget->isVisible())
            widget->pData->display();
    }

    if (char* const filename = filenameToRenderInto)
    {
        const PuglArea size = puglGetSizeHint(view, PUGL_CURRENT_SIZE);
        filenameToRenderInto = nullptr;
        renderToPicture(filename, getGraphicsContext(), size.width, size.height);
        std::free(filename);
    }

    endContext();
}

void Window::PrivateData::onPuglClose()
{
    DGL_DBG("PUGL: onClose\n");

#ifndef DISTRHO_OS_MAC
    // if we are running as standalone we can prevent closing in certain conditions
    if (appData->isStandalone)
    {
        // a child window is active, gives focus to it
        if (modal.child != nullptr)
            return modal.child->focus();

        // ask window if we should close
        if (! self->onClose())
            return;
    }
#endif

    if (modal.enabled)
        stopModal();

    if (modal.child != nullptr)
    {
        modal.child->close();
        modal.child = nullptr;
    }

    close();
}

void Window::PrivateData::onPuglFocus(const bool focus, const CrossingMode mode)
{
    DGL_DBGp("onPuglFocus : %i %i | %i\n", focus, mode, isClosed);

    if (isClosed)
        return;

    if (modal.child != nullptr)
        return modal.child->focus();

    self->onFocus(focus, mode);
}

void Window::PrivateData::onPuglKey(const Widget::KeyboardEvent& ev)
{
    DGL_DBGp("onPuglKey : %i %u %u\n", ev.press, ev.key, ev.keycode);

    if (modal.child != nullptr)
        return modal.child->focus();

    FOR_EACH_TOP_LEVEL_WIDGET_INV(rit)
    {
        TopLevelWidget* const widget(*rit);

        if (widget->isVisible() && widget->onKeyboard(ev))
            break;
    }
}

void Window::PrivateData::onPuglText(const Widget::CharacterInputEvent& ev)
{
    DGL_DBGp("onPuglText : %u %u %s\n", ev.keycode, ev.character, ev.string);

    if (modal.child != nullptr)
        return modal.child->focus();

    FOR_EACH_TOP_LEVEL_WIDGET_INV(rit)
    {
        TopLevelWidget* const widget(*rit);

        if (widget->isVisible() && widget->onCharacterInput(ev))
            break;
    }
}

void Window::PrivateData::onPuglMouse(const Widget::MouseEvent& ev)
{
    DGL_DBGp("onPuglMouse : %i %i %f %f\n", ev.button, ev.press, ev.pos.getX(), ev.pos.getY());

    if (modal.child != nullptr)
        return modal.child->focus();

    FOR_EACH_TOP_LEVEL_WIDGET_INV(rit)
    {
        TopLevelWidget* const widget(*rit);

        if (widget->isVisible() && widget->onMouse(ev))
            break;
    }
}

void Window::PrivateData::onPuglMotion(const Widget::MotionEvent& ev)
{
    DGL_DBGp("onPuglMotion : %f %f\n", ev.pos.getX(), ev.pos.getY());

    if (modal.child != nullptr)
        return modal.child->focus();

    FOR_EACH_TOP_LEVEL_WIDGET_INV(rit)
    {
        TopLevelWidget* const widget(*rit);

        if (widget->isVisible() && widget->onMotion(ev))
            break;
    }
}

void Window::PrivateData::onPuglScroll(const Widget::ScrollEvent& ev)
{
    DGL_DBGp("onPuglScroll : %f %f %f %f\n", ev.pos.getX(), ev.pos.getY(), ev.delta.getX(), ev.delta.getY());

    if (modal.child != nullptr)
        return modal.child->focus();

    FOR_EACH_TOP_LEVEL_WIDGET_INV(rit)
    {
        TopLevelWidget* const widget(*rit);

        if (widget->isVisible() && widget->onScroll(ev))
            break;
    }
}

const void* Window::PrivateData::getClipboard(size_t& dataSize)
{
    clipboardTypeId = 0;
    waitingForClipboardData = true,
    waitingForClipboardEvents = true;

    // begin clipboard dance here
    if (puglPaste(view) != PUGL_SUCCESS)
    {
        dataSize = 0;
        waitingForClipboardEvents = false;
        return nullptr;
    }

   #ifdef DGL_USING_X11
    // wait for type request, clipboardTypeId must be != 0 to be valid
    int retry = static_cast<int>(2 / 0.03);
    while (clipboardTypeId == 0 && waitingForClipboardData && --retry >= 0)
    {
        if (puglX11UpdateWithoutExposures(appData->world) != PUGL_SUCCESS)
            break;
    }
   #endif

    if (clipboardTypeId == 0)
    {
        dataSize = 0;
        waitingForClipboardEvents = false;
        return nullptr;
    }

   #ifdef DGL_USING_X11
    // wait for actual data (assumes offer was accepted)
    retry = static_cast<int>(2 / 0.03);
    while (waitingForClipboardData && --retry >= 0)
    {
        if (puglX11UpdateWithoutExposures(appData->world) != PUGL_SUCCESS)
            break;
    }
   #endif

    if (clipboardTypeId == 0)
    {
        dataSize = 0;
        waitingForClipboardEvents = false;
        return nullptr;
    }

    waitingForClipboardEvents = false;
    return puglGetClipboard(view, clipboardTypeId - 1, &dataSize);
}

uint32_t Window::PrivateData::onClipboardDataOffer()
{
    DGL_DBG("onClipboardDataOffer\n");

    if ((clipboardTypeId = self->onClipboardDataOffer()) != 0)
        return clipboardTypeId;

    // stop waiting for data, it was rejected
    waitingForClipboardData = false;
    return 0;
}

void Window::PrivateData::onClipboardData(const uint32_t typeId)
{
    if (clipboardTypeId != typeId)
        clipboardTypeId = 0;

    waitingForClipboardData = false;
}

#if defined(DEBUG) && defined(DGL_DEBUG_EVENTS)
static int printEvent(const PuglEvent* event, const char* prefix, const bool verbose);
#endif

PuglStatus Window::PrivateData::puglEventCallback(PuglView* const view, const PuglEvent* const event)
{
    Window::PrivateData* const pData = (Window::PrivateData*)puglGetHandle(view);
#if defined(DEBUG) && defined(DGL_DEBUG_EVENTS)
    if (event->type != PUGL_TIMER && event->type != PUGL_EXPOSE && event->type != PUGL_MOTION) {
        printEvent(event, "pugl event: ", true);
    }
#endif

    if (pData->waitingForClipboardEvents)
    {
        switch (event->type)
        {
        case PUGL_UPDATE:
        case PUGL_EXPOSE:
        case PUGL_FOCUS_IN:
        case PUGL_FOCUS_OUT:
        case PUGL_KEY_PRESS:
        case PUGL_KEY_RELEASE:
        case PUGL_TEXT:
        case PUGL_POINTER_IN:
        case PUGL_POINTER_OUT:
        case PUGL_BUTTON_PRESS:
        case PUGL_BUTTON_RELEASE:
        case PUGL_MOTION:
        case PUGL_SCROLL:
        case PUGL_TIMER:
        case PUGL_LOOP_ENTER:
        case PUGL_LOOP_LEAVE:
            return PUGL_SUCCESS;
        case PUGL_DATA_OFFER:
        case PUGL_DATA:
            break;
        default:
            d_stdout("Got event %d while waitingForClipboardEvents", event->type);
            break;
        }
    }

    switch (event->type)
    {
    ///< No event
    case PUGL_NOTHING:
        break;

    ///< View realized, a #PuglRealizeEvent
    case PUGL_REALIZE:
        if (! pData->isEmbed && ! puglGetTransientParent(view))
        {
           #if defined(DISTRHO_OS_WINDOWS) && defined(DGL_WINDOWS_ICON_ID)
            WNDCLASSEX wClass = {};
            const HINSTANCE hInstance = GetModuleHandle(nullptr);

            if (GetClassInfoEx(hInstance, view->world->strings[PUGL_CLASS_NAME], &wClass))
                wClass.hIcon = LoadIcon(nullptr, MAKEINTRESOURCE(DGL_WINDOWS_ICON_ID));

            SetClassLongPtr(view->impl->hwnd, GCLP_HICON, (LONG_PTR) LoadIcon(hInstance, MAKEINTRESOURCE(DGL_WINDOWS_ICON_ID)));
           #endif
           #ifdef DGL_USING_X11
            puglX11SetWindowType(view, pData->appData->isStandalone);
           #endif
        }
        break;

    ///< View unrealizeed, a #PuglUnrealizeEvent
    case PUGL_UNREALIZE:
        break;

    ///< View configured, a #PuglConfigureEvent
    case PUGL_CONFIGURE:
        // unused x, y (double)
        pData->onPuglConfigure(event->configure.width, event->configure.height);
        break;

    ///< View ready to draw, a #PuglUpdateEvent
    case PUGL_UPDATE:
        break;

    ///< View must be drawn, a #PuglExposeEvent
    case PUGL_EXPOSE:
        // unused x, y, width, height (double)
        pData->onPuglExpose();
        break;

    ///< View will be closed, a #PuglCloseEvent
    case PUGL_CLOSE:
        pData->onPuglClose();
        break;

    ///< Keyboard focus entered view, a #PuglFocusEvent
    case PUGL_FOCUS_IN:
    ///< Keyboard focus left view, a #PuglFocusEvent
    case PUGL_FOCUS_OUT:
        pData->onPuglFocus(event->type == PUGL_FOCUS_IN,
                           static_cast<CrossingMode>(event->focus.mode));
        break;

    ///< Key pressed, a #PuglKeyEvent
    case PUGL_KEY_PRESS:
    ///< Key released, a #PuglKeyEvent
    case PUGL_KEY_RELEASE:
    {
        // unused x, y, xRoot, yRoot (double)
        Widget::KeyboardEvent ev;
        ev.mod     = event->key.state;
        ev.flags   = event->key.flags;
        ev.time    = d_roundToUnsignedInt(event->key.time * 1000.0);
        ev.press   = event->type == PUGL_KEY_PRESS;
        ev.key     = event->key.key;
        ev.keycode = event->key.keycode;

        // keyboard events must always be lowercase
        if (ev.key >= 'A' && ev.key <= 'Z')
        {
            ev.key += 'a' - 'A'; // A-Z -> a-z
            ev.mod |= kModifierShift;
        }

        pData->onPuglKey(ev);
        break;
    }

    ///< Character entered, a #PuglTextEvent
    case PUGL_TEXT:
    {
        // unused x, y, xRoot, yRoot (double)
        Widget::CharacterInputEvent ev;
        ev.mod       = event->text.state;
        ev.flags     = event->text.flags;
        ev.time      = d_roundToUnsignedInt(event->text.time * 1000.0);
        ev.keycode   = event->text.keycode;
        ev.character = event->text.character;
        std::strncpy(ev.string, event->text.string, sizeof(ev.string));
        pData->onPuglText(ev);
        break;
    }

    ///< Pointer entered view, a #PuglCrossingEvent
    case PUGL_POINTER_IN:
        break;
    ///< Pointer left view, a #PuglCrossingEvent
    case PUGL_POINTER_OUT:
        break;

    ///< Mouse button pressed, a #PuglButtonEvent
    case PUGL_BUTTON_PRESS:
    ///< Mouse button released, a #PuglButtonEvent
    case PUGL_BUTTON_RELEASE:
    {
        Widget::MouseEvent ev;
        ev.mod    = event->button.state;
        ev.flags  = event->button.flags;
        ev.time   = d_roundToUnsignedInt(event->button.time * 1000.0);
        ev.button = event->button.button + 1;
        ev.press  = event->type == PUGL_BUTTON_PRESS;
       #if DGL_ALLOW_DEPRECATED_METHODS
        if (pData->autoScaling && 0)
        {
            const double scaleFactor = pData->autoScaleFactor;
            ev.pos = Point<double>(event->button.x / scaleFactor, event->button.y / scaleFactor);
        }
        else
       #endif
        {
            ev.pos = Point<double>(event->button.x, event->button.y);
        }
        ev.absolutePos = ev.pos;
        pData->onPuglMouse(ev);
        break;
    }

    ///< Pointer moved, a #PuglMotionEvent
    case PUGL_MOTION:
    {
        Widget::MotionEvent ev;
        ev.mod   = event->motion.state;
        ev.flags = event->motion.flags;
        ev.time  = d_roundToUnsignedInt(event->motion.time * 1000.0);
       #if DGL_ALLOW_DEPRECATED_METHODS
        if (pData->autoScaling && 0)
        {
            const double scaleFactor = pData->autoScaleFactor;
            ev.pos = Point<double>(event->motion.x / scaleFactor, event->motion.y / scaleFactor);
        }
        else
       #endif
        {
            ev.pos = Point<double>(event->motion.x, event->motion.y);
        }
        ev.absolutePos = ev.pos;
        pData->onPuglMotion(ev);
        break;
    }

    ///< Scrolled, a #PuglScrollEvent
    case PUGL_SCROLL:
    {
        Widget::ScrollEvent ev;
        ev.mod       = event->scroll.state;
        ev.flags     = event->scroll.flags;
        ev.time      = d_roundToUnsignedInt(event->scroll.time * 1000.0);
       #if DGL_ALLOW_DEPRECATED_METHODS
        if (pData->autoScaling && 0)
        {
            const double scaleFactor = pData->autoScaleFactor;
            ev.pos   = Point<double>(event->scroll.x / scaleFactor, event->scroll.y / scaleFactor);
            ev.delta = Point<double>(event->scroll.dx / scaleFactor, event->scroll.dy / scaleFactor);
        }
        else
       #endif
        {
            ev.pos   = Point<double>(event->scroll.x, event->scroll.y);
            ev.delta = Point<double>(event->scroll.dx, event->scroll.dy);
        }
        ev.direction = static_cast<ScrollDirection>(event->scroll.direction);
        ev.absolutePos = ev.pos;
        pData->onPuglScroll(ev);
        break;
    }

    ///< Custom client message, a #PuglClientEvent
    case PUGL_CLIENT:
        break;

    ///< Timer triggered, a #PuglTimerEvent
    case PUGL_TIMER:
        if (IdleCallback* const idleCallback = reinterpret_cast<IdleCallback*>(event->timer.id))
            idleCallback->idleCallback();
        break;

    ///< Recursive loop left, a #PuglLoopLeaveEvent
    case PUGL_LOOP_ENTER:
        break;

    ///< Recursive loop left, a #PuglEventLoopLeave
    case PUGL_LOOP_LEAVE:
        break;

    ///< Data offered from clipboard, a #PuglDataOfferEvent
    case PUGL_DATA_OFFER:
        if (const uint32_t offerTypeId = pData->onClipboardDataOffer())
            puglAcceptOffer(view, &event->offer, offerTypeId - 1);
        break;

    ///< Data available from clipboard, a #PuglDataEvent
    case PUGL_DATA:
        pData->onClipboardData(event->data.typeIndex + 1);
        break;
    }

    return PUGL_SUCCESS;
}

// -----------------------------------------------------------------------

#if defined(DEBUG) && defined(DGL_DEBUG_EVENTS)
static int printModifiers(const uint32_t mods)
{
	return fprintf(stderr, "Modifiers:%s%s%s%s\n",
	               (mods & PUGL_MOD_SHIFT) ? " Shift"   : "",
	               (mods & PUGL_MOD_CTRL)  ? " Ctrl"    : "",
	               (mods & PUGL_MOD_ALT)   ? " Alt"     : "",
	               (mods & PUGL_MOD_SUPER) ? " Super" : "");
}

static int printEvent(const PuglEvent* event, const char* prefix, const bool verbose)
{
#define FFMT            "%6.1f"
#define PFMT            FFMT " " FFMT
#define PRINT(fmt, ...) d_stdout(fmt, __VA_ARGS__), 1

	switch (event->type) {
	case PUGL_NOTHING:
		return 0;
	case PUGL_KEY_PRESS:
		return PRINT("%sKey press   code %3u key  U+%04X\n",
		             prefix,
		             event->key.keycode,
		             event->key.key);
	case PUGL_KEY_RELEASE:
		return PRINT("%sKey release code %3u key  U+%04X\n",
		             prefix,
		             event->key.keycode,
		             event->key.key);
	case PUGL_TEXT:
		return PRINT("%sText entry  code %3u char U+%04X (%s)\n",
		             prefix,
		             event->text.keycode,
		             event->text.character,
		             event->text.string);
	case PUGL_BUTTON_PRESS:
	case PUGL_BUTTON_RELEASE:
		return (PRINT("%sMouse %u %s at " PFMT " ",
		              prefix,
		              event->button.button,
		              (event->type == PUGL_BUTTON_PRESS) ? "down" : "up  ",
		              event->button.x,
		              event->button.y) +
		        printModifiers(event->scroll.state));
	case PUGL_SCROLL:
		return (PRINT("%sScroll %5.1f %5.1f at " PFMT " ",
		              prefix,
		              event->scroll.dx,
		              event->scroll.dy,
		              event->scroll.x,
		              event->scroll.y) +
		        printModifiers(event->scroll.state));
	case PUGL_POINTER_IN:
		return PRINT("%sMouse enter  at " PFMT "\n",
		             prefix,
		             event->crossing.x,
		             event->crossing.y);
	case PUGL_POINTER_OUT:
		return PRINT("%sMouse leave  at " PFMT "\n",
		             prefix,
		             event->crossing.x,
		             event->crossing.y);
	case PUGL_FOCUS_IN:
		return PRINT("%sFocus in %i\n",
		             prefix,
		             event->focus.mode);
	case PUGL_FOCUS_OUT:
		return PRINT("%sFocus out %i\n",
		             prefix,
		             event->focus.mode);
	case PUGL_CLIENT:
		return PRINT("%sClient %" PRIXPTR " %" PRIXPTR "\n",
		             prefix,
		             event->client.data1,
		             event->client.data2);
	case PUGL_TIMER:
		return PRINT("%sTimer %" PRIuPTR "\n", prefix, event->timer.id);
	default:
		break;
	}

	if (verbose) {
		switch (event->type) {
		case PUGL_REALIZE:
			return PRINT("%sRealize\n", prefix);
		case PUGL_UNREALIZE:
			return PRINT("%sUnrealize\n", prefix);
		case PUGL_CONFIGURE:
			return PRINT("%sConfigure %d %d %d %d\n",
			             prefix,
			             event->configure.x,
			             event->configure.y,
			             event->configure.width,
			             event->configure.height);
		case PUGL_UPDATE:
			return 0; // fprintf(stderr, "%sUpdate\n", prefix);
		case PUGL_EXPOSE:
			return PRINT("%sExpose    %d %d %d %d\n",
			             prefix,
			             event->expose.x,
			             event->expose.y,
			             event->expose.width,
			             event->expose.height);
		case PUGL_CLOSE:
			return PRINT("%sClose\n", prefix);
		case PUGL_MOTION:
			return PRINT("%sMouse motion at " PFMT "\n",
			             prefix,
			             event->motion.x,
			             event->motion.y);
		default:
			return PRINT("%sUnknown event type %d\n", prefix, (int)event->type);
		}
	}

#undef PRINT
#undef PFMT
#undef FFMT

	return 0;
}
#endif

#undef DGL_DBG
#undef DGL_DBGF

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
