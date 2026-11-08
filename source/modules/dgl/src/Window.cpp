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
#include "../TopLevelWidget.hpp"

#include "pugl.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// ScopedGraphicsContext

Window::ScopedGraphicsContext::ScopedGraphicsContext(Window& win)
    : window(win),
      ppData(nullptr),
      active(window.pData->view != nullptr && puglBackendEnter(window.pData->view)),
      reenter(false)
{
    if (active)
        window.pData->createContextIfNeeded();
}

Window::ScopedGraphicsContext::ScopedGraphicsContext(Window& win, Window& transientWin)
    : window(win),
      ppData(transientWin.pData),
      active(false),
      reenter(window.pData->view != nullptr)
{
    if (reenter)
    {
        puglBackendLeave(ppData->view);
        active = puglBackendEnter(window.pData->view);
        if (active)
            window.pData->createContextIfNeeded();
    }
}

Window::ScopedGraphicsContext::~ScopedGraphicsContext()
{
    done();
}

void Window::ScopedGraphicsContext::done()
{
    if (active)
    {
        puglBackendLeave(window.pData->view);
        active = false;
    }

    if (reenter)
    {
        reenter = false;
        DISTRHO_SAFE_ASSERT_RETURN(ppData != nullptr,);

        puglBackendEnter(ppData->view);
    }
}

void Window::ScopedGraphicsContext::reinit()
{
    DISTRHO_SAFE_ASSERT_RETURN(!active,);
    DISTRHO_SAFE_ASSERT_RETURN(!reenter,);
    DISTRHO_SAFE_ASSERT_RETURN(ppData != nullptr,);

    reenter = true;
    puglBackendLeave(ppData->view);
    active = puglBackendEnter(window.pData->view);
}

// -----------------------------------------------------------------------
// Window

Window::Window(Application& app)
    : pData(new PrivateData(app, this))
{
    pData->initPost();
}

Window::Window(Application& app, Window& transientParentWindow)
    : pData(new PrivateData(app, this, transientParentWindow.pData))
{
    pData->initPost();
}

Window::Window(Application& app,
               const uintptr_t parentWindowHandle,
               const double scaleFactor,
               const bool resizable)
    : pData(new PrivateData(app, this, parentWindowHandle, scaleFactor, resizable))
{
    pData->initPost();
}

Window::Window(Application& app,
               const uintptr_t parentWindowHandle,
               const uint width,
               const uint height,
               const double scaleFactor,
               const bool resizable)
    : pData(new PrivateData(app, this, parentWindowHandle, width, height, scaleFactor, resizable, false, false))
{
    pData->initPost();
}

Window::Window(Application& app,
               const uintptr_t parentWindowHandle,
               const uint width,
               const uint height,
               const double scaleFactor,
               const bool resizable,
               const bool usesScheduledRepaints,
               const bool usesSizeRequest,
               const bool doPostInit)
    : pData(new PrivateData(app, this, parentWindowHandle, width, height, scaleFactor, resizable,
                            usesScheduledRepaints, usesSizeRequest))
{
    if (doPostInit)
        pData->initPost();
}

Window::~Window()
{
    delete pData;
}

bool Window::isEmbed() const noexcept
{
    return pData->isEmbed;
}

bool Window::isVisible() const noexcept
{
    return pData->isVisible;
}

void Window::setVisible(const bool visible)
{
    if (visible)
        pData->show();
    else
        pData->hide();
}

void Window::show()
{
    pData->show();
}

void Window::hide()
{
    pData->hide();
}

void Window::close()
{
    pData->close();
}

bool Window::isResizable() const noexcept
{
    return pData->view != nullptr
        && puglGetViewHint(pData->view, PUGL_RESIZABLE) == PUGL_TRUE;
}

void Window::setResizable(const bool resizable)
{
    pData->setResizable(resizable);
}

int Window::getOffsetX() const noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(pData->view != nullptr, 0);

    return puglGetPositionHint(pData->view, PUGL_CURRENT_POSITION).x;
}

int Window::getOffsetY() const noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(pData->view != nullptr, 0);

    return puglGetPositionHint(pData->view, PUGL_CURRENT_POSITION).y;
}

Point<int> Window::getOffset() const noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(pData->view != nullptr, Point<int>());

    const PuglPoint pos = puglGetPositionHint(pData->view, PUGL_CURRENT_POSITION);
    return Point<int>(pos.x, pos.y);
}

void Window::setOffsetX(const int x)
{
    setOffset(x, getOffsetY());
}

void Window::setOffsetY(const int y)
{
    setOffset(getOffsetX(), y);
}

void Window::setOffset(const int x, const int y)
{
    // do not call this for embed windows!
    DISTRHO_SAFE_ASSERT_RETURN(!pData->isEmbed,);

    if (pData->view != nullptr)
        puglSetPositionHint(pData->view, PUGL_CURRENT_POSITION, x, y);
}

void Window::setOffset(const Point<int>& offset)
{
    setOffset(offset.getX(), offset.getY());
}

uint Window::getWidth() const noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(pData->view != nullptr, 0);

    const PuglSpan width = puglGetSizeHint(pData->view, PUGL_CURRENT_SIZE).width;
    DISTRHO_SAFE_ASSERT(width > 0);
    return width;
}

uint Window::getHeight() const noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(pData->view != nullptr, 0);

    const PuglSpan height = puglGetSizeHint(pData->view, PUGL_CURRENT_SIZE).height;
    DISTRHO_SAFE_ASSERT(height > 0);
    return height;
}

Size<uint> Window::getSize() const noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(pData->view != nullptr, Size<uint>());

    const PuglArea size = puglGetSizeHint(pData->view, PUGL_CURRENT_SIZE);
    DISTRHO_SAFE_ASSERT(size.width > 0);
    DISTRHO_SAFE_ASSERT(size.height > 0);
    return Size<uint>(size.width, size.height);
}

void Window::setWidth(const uint width)
{
    setSize(width, getHeight());
}

void Window::setHeight(const uint height)
{
    setSize(getWidth(), height);
}

void Window::setSize(uint width, uint height)
{
    DISTRHO_SAFE_ASSERT_UINT2_RETURN(width > 1 && height > 1, width, height,);

    const double scaleFactor = pData->scaleFactor;

    if (pData->isEmbed)
    {
        PuglArea area = puglGetSizeHint(pData->view, PUGL_FIXED_ASPECT);

        if (pData->autoScaling && d_isNotEqual(scaleFactor, 1.0))
        {
            area.width = d_roundToUnsignedInt(area.width * scaleFactor);
            area.height = d_roundToUnsignedInt(area.height * scaleFactor);
            width = d_roundToUnsignedInt(width * scaleFactor);
            height = d_roundToUnsignedInt(height * scaleFactor);
        }

        // handle geometry constraints here
        if (width < area.width)
            width = area.width;

        if (height < area.height)
            height = area.height;

        if (area.width != 0 && area.height != 0)
        {
            const double ratio = static_cast<double>(area.width)
                               / static_cast<double>(area.height);
            const double reqRatio = static_cast<double>(width)
                                  / static_cast<double>(height);

            if (d_isNotEqual(ratio, reqRatio))
            {
                // fix width
                if (reqRatio > ratio)
                    width = d_roundToUnsignedInt(height * ratio);
                // fix height
                else
                    height = d_roundToUnsignedInt(static_cast<double>(width) / ratio);
            }
        }
    }
    else
    {
        if (pData->autoScaling && d_isNotEqual(scaleFactor, 1.0))
        {
            width = d_roundToUnsignedInt(width * scaleFactor);
            height = d_roundToUnsignedInt(height * scaleFactor);
        }
    }

    if (pData->usesSizeRequest)
    {
        DISTRHO_SAFE_ASSERT_RETURN(pData->topLevelWidgets.size() != 0,);

        TopLevelWidget* const topLevelWidget = pData->topLevelWidgets.front();
        DISTRHO_SAFE_ASSERT_RETURN(topLevelWidget != nullptr,);

        topLevelWidget->requestSizeChange(width, height);
    }
    else if (pData->view != nullptr)
    {
        puglSetSizeAndDefault(pData->view, width, height);

        // there are no resize events for closed windows, so short-circuit the top-level widgets here
        if (pData->isClosed)
        {
            for (std::list<TopLevelWidget*>::iterator it = pData->topLevelWidgets.begin(),
                                                      end = pData->topLevelWidgets.end(); it != end; ++it)
            {
                ((Widget*)*it)->setSize(width, height);
            }
        }
    }
}

void Window::setSize(const Size<uint>& size)
{
    setSize(size.getWidth(), size.getHeight());
}

const char* Window::getTitle() const noexcept
{
    return pData->view != nullptr ? puglGetViewString(pData->view, PUGL_WINDOW_TITLE) : "";
}

void Window::setTitle(const char* const title)
{
    if (pData->view != nullptr)
        puglSetViewString(pData->view, PUGL_WINDOW_TITLE, title);
}

bool Window::isIgnoringKeyRepeat() const noexcept
{
    return pData->view != nullptr
        && puglGetViewHint(pData->view, PUGL_IGNORE_KEY_REPEAT) == PUGL_TRUE;
}

void Window::setIgnoringKeyRepeat(const bool ignore) noexcept
{
    if (pData->view != nullptr)
        puglSetViewHint(pData->view, PUGL_IGNORE_KEY_REPEAT, ignore);
}

const void* Window::getClipboard(size_t& dataSize)
{
    return pData->getClipboard(dataSize);
}

bool Window::setClipboard(const char* const mimeType, const void* const data, const size_t dataSize)
{
    return pData->view != nullptr
        && puglSetClipboard(pData->view, mimeType != nullptr ? mimeType : "text/plain", data, dataSize) == PUGL_SUCCESS;
}

bool Window::setCursor(const MouseCursor cursor)
{
    return pData->view != nullptr
        && puglSetCursor(pData->view, static_cast<PuglCursor>(cursor)) == PUGL_SUCCESS;
}

bool Window::addIdleCallback(IdleCallback* const callback, const uint timerFrequencyInMs)
{
    DISTRHO_SAFE_ASSERT_RETURN(callback != nullptr, false)

    return pData->addIdleCallback(callback, timerFrequencyInMs);
}

bool Window::removeIdleCallback(IdleCallback* const callback)
{
    DISTRHO_SAFE_ASSERT_RETURN(callback != nullptr, false)

    return pData->removeIdleCallback(callback);
}

Application& Window::getApp() const noexcept
{
    return pData->app;
}

const GraphicsContext& Window::getGraphicsContext() const noexcept
{
    return pData->getGraphicsContext();
}

uintptr_t Window::getNativeWindowHandle() const noexcept
{
    return pData->view != nullptr ? puglGetNativeView(pData->view) : 0;
}

double Window::getScaleFactor() const noexcept
{
    return pData->scaleFactor;
}

void Window::enableInternalScalingWithSize(uint baseWidth, uint baseHeight, const bool keepAspectRatio)
{
    DISTRHO_SAFE_ASSERT_RETURN(baseWidth > 0,);
    DISTRHO_SAFE_ASSERT_RETURN(baseHeight > 0,);

    const Size<uint> size(getSize());
    const double scaleHorizontal = size.getWidth() / static_cast<double>(baseWidth);
    const double scaleVertical = size.getHeight() / static_cast<double>(baseHeight);

    pData->baseWidth = baseWidth;
    pData->baseHeight = baseHeight;
    pData->autoScaling = true;
    pData->autoScaleFactor = scaleHorizontal < scaleVertical ? scaleHorizontal : scaleVertical;

    if (pData->view == nullptr)
        return;

    const double scaleFactor = pData->scaleFactor;

    if (d_isNotEqual(scaleFactor, 1.0))
    {
        baseWidth = d_roundToUnsignedInt(baseWidth * scaleFactor);
        baseHeight = d_roundToUnsignedInt(baseHeight * scaleFactor);
    }

    puglSetGeometryConstraints(pData->view, baseWidth, baseHeight, keepAspectRatio);
}

void Window::focus()
{
    pData->focus();
}

#ifdef DGL_USE_FILE_BROWSER
bool Window::openFileBrowser(const FileBrowserOptions& options)
{
    return pData->openFileBrowser(options);
}
#endif

#ifdef DGL_USE_WEB_VIEW
bool Window::createWebView(const char* const url, const DGL_NAMESPACE::WebViewOptions& options)
{
    return pData->createWebView(url, options);
}

void Window::evaluateJS(const char* const js)
{
    DISTRHO_SAFE_ASSERT_RETURN(pData->webViewHandle != nullptr,);

    webViewEvaluateJS(pData->webViewHandle, js);
}
#endif

void Window::repaint() noexcept
{
    if (pData->view == nullptr)
        return;

    if (pData->usesScheduledRepaints)
        pData->appData->needsRepaint = true;

    puglObscureView(pData->view);
}

void Window::repaint(const Rectangle<uint>& rect) noexcept
{
    if (pData->view == nullptr)
        return;

    if (pData->usesScheduledRepaints)
        pData->appData->needsRepaint = true;

    int x = static_cast<int>(rect.getX());
    int y = static_cast<int>(rect.getY());
    uint width = rect.getWidth();
    uint height = rect.getHeight();

    if (pData->autoScaling)
    {
        const double autoScaleFactor = pData->autoScaleFactor;

        x = d_roundToIntPositive(x * autoScaleFactor);
        y = d_roundToIntPositive(y * autoScaleFactor);
        width = d_roundToUnsignedInt(width * autoScaleFactor);
        height = d_roundToUnsignedInt(height * autoScaleFactor);
    }

    puglObscureRegion(pData->view, x, y, width, height);
}

void Window::renderToPicture(const char* const filename)
{
    pData->filenameToRenderInto = strdup(filename);
}

void Window::runAsModal(bool blockWait)
{
    pData->runAsModal(blockWait);
}

Size<uint> Window::getGeometryConstraints(bool& keepAspectRatio)
{
    const PuglArea area = puglGetSizeHint(pData->view, PUGL_FIXED_ASPECT);
    keepAspectRatio = area.width != 0 && area.height != 0;
    return Size<uint>(area.width, area.height);
}

void Window::setGeometryConstraints(const uint minimumWidth, const uint minimumHeight, const bool keepAspectRatio)
{
    DISTRHO_SAFE_ASSERT_RETURN(minimumWidth > 0,);
    DISTRHO_SAFE_ASSERT_RETURN(minimumHeight > 0,);

    if (pData->view != nullptr)
        puglSetGeometryConstraints(pData->view, minimumWidth, minimumHeight, keepAspectRatio);
}

#if DGL_ALLOW_DEPRECATED_METHODS
void Window::setGeometryConstraints(uint minimumWidth,
                                    uint minimumHeight,
                                    const bool keepAspectRatio,
                                    const bool automaticallyScale,
                                    bool resizeNowIfAutoScaling)
{
    DISTRHO_SAFE_ASSERT_RETURN(minimumWidth > 0,);
    DISTRHO_SAFE_ASSERT_RETURN(minimumHeight > 0,);

    // prevent auto-scaling up 2x
    if (resizeNowIfAutoScaling && automaticallyScale && pData->autoScaling == automaticallyScale)
        resizeNowIfAutoScaling = false;

    if (automaticallyScale)
    {
        pData->autoScaling = true;
        pData->baseWidth = minimumWidth;
        pData->baseHeight = minimumHeight;
    }
    else
    {
        pData->autoScaling = false;
        pData->baseWidth = 0;
        pData->baseHeight = 0;
    }

    if (pData->view == nullptr)
        return;

    const double scaleFactor = pData->scaleFactor;

    if (automaticallyScale && scaleFactor != 1.0)
    {
        minimumWidth = d_roundToUnsignedInt(minimumWidth * scaleFactor);
        minimumHeight = d_roundToUnsignedInt(minimumHeight * scaleFactor);
    }

    puglSetGeometryConstraints(pData->view, minimumWidth, minimumHeight, keepAspectRatio);

    if (scaleFactor != 1.0 && automaticallyScale && resizeNowIfAutoScaling)
    {
        const Size<uint> size(getSize());

        setSize(d_roundToUnsignedInt(size.getWidth() * scaleFactor),
                d_roundToUnsignedInt(size.getHeight() * scaleFactor));
    }
}
#endif

void Window::setTransientParent(const uintptr_t transientParentWindowHandle)
{
    if (pData->view != nullptr)
        puglSetTransientParent(pData->view, transientParentWindowHandle);
}

std::vector<ClipboardDataOffer> Window::getClipboardDataOfferTypes()
{
    std::vector<ClipboardDataOffer> offerTypes;

    if (pData->view == nullptr)
        return offerTypes;

    if (const uint32_t numTypes = puglGetNumClipboardTypes(pData->view))
    {
        offerTypes.reserve(numTypes);

        for (uint32_t i=0; i<numTypes; ++i)
        {
            const ClipboardDataOffer offer = { i + 1, puglGetClipboardType(pData->view, i) };
            offerTypes.push_back(offer);
        }
    }

    return offerTypes;
}

uint32_t Window::onClipboardDataOffer()
{
    std::vector<ClipboardDataOffer> offers(getClipboardDataOfferTypes());

    for (std::vector<ClipboardDataOffer>::iterator it=offers.begin(), end=offers.end(); it != end;++it)
    {
        const ClipboardDataOffer offer = *it;
        if (std::strcmp(offer.type, "text/plain") == 0)
            return offer.id;
    }

    return 0;
}

bool Window::onClose()
{
    return true;
}

void Window::onFocus(bool, CrossingMode)
{
}

#if DGL_ALLOW_DEPRECATED_METHODS
void Window::onReshape(uint, uint)
{
}
#endif

void Window::onScaleFactorChanged(double)
{
}

#ifdef DGL_USE_FILE_BROWSER
void Window::onFileSelected(const char*)
{
}
#endif

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
