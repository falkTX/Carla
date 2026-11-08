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

#include "TopLevelWidgetPrivateData.hpp"
#include "../Window.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

TopLevelWidget::TopLevelWidget(Window& windowToMapTo)
    : Widget(this),
      pData(new PrivateData(this, windowToMapTo)) {}

TopLevelWidget::~TopLevelWidget()
{
    delete pData;
}

Application& TopLevelWidget::getApp() const noexcept
{
    return pData->window.getApp();
}

Window& TopLevelWidget::getWindow() const noexcept
{
    return pData->window;
}

void TopLevelWidget::setWidth(const uint width)
{
    pData->window.setWidth(width);
}

void TopLevelWidget::setHeight(const uint height)
{
    pData->window.setHeight(height);
}

void TopLevelWidget::setSize(const uint width, const uint height)
{
    pData->window.setSize(width, height);
}

void TopLevelWidget::setSize(const Size<uint>& size)
{
    pData->window.setSize(size);
}

const void* TopLevelWidget::getClipboard(size_t& dataSize)
{
    return pData->window.getClipboard(dataSize);
}

bool TopLevelWidget::setClipboard(const char* const mimeType, const void* const data, const size_t dataSize)
{
    return pData->window.setClipboard(mimeType, data, dataSize);
}

bool TopLevelWidget::setCursor(const MouseCursor cursor)
{
    return pData->window.setCursor(cursor);
}

bool TopLevelWidget::addIdleCallback(IdleCallback* const callback, const uint timerFrequencyInMs)
{
    return pData->window.addIdleCallback(callback, timerFrequencyInMs);
}

bool TopLevelWidget::removeIdleCallback(IdleCallback* const callback)
{
    return pData->window.removeIdleCallback(callback);
}

double TopLevelWidget::getScaleFactor() const noexcept
{
    return pData->window.getScaleFactor();
}

void TopLevelWidget::repaint() noexcept
{
    pData->window.repaint();
}

void TopLevelWidget::repaint(const Rectangle<uint>& rect) noexcept
{
    pData->window.repaint(rect);
}

void TopLevelWidget::setGeometryConstraints(const uint minimumWidth, const uint minimumHeight, const bool keepAspectRatio)
{
    pData->window.setGeometryConstraints(minimumWidth, minimumHeight, keepAspectRatio);
}

#if DGL_ALLOW_DEPRECATED_METHODS
void TopLevelWidget::setGeometryConstraints(const uint minimumWidth,
                                            const uint minimumHeight,
                                            const bool keepAspectRatio,
                                            const bool automaticallyScale,
                                            const bool resizeNowIfAutoScaling)
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
    pData->window.setGeometryConstraints(minimumWidth,
                                         minimumHeight,
                                         keepAspectRatio,
                                         automaticallyScale,
                                         resizeNowIfAutoScaling);
   #if defined(_MSC_VER)
    #pragma warning(pop)
   #elif defined(__clang__)
    #pragma clang diagnostic pop
   #elif defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 460
    #pragma GCC diagnostic pop
   #endif
}
#endif

// --------------------------------------------------------------------------------------------------------------------

bool TopLevelWidget::onKeyboard(const KeyboardEvent& ev)
{
    return pData->keyboardEvent(ev);
}

bool TopLevelWidget::onCharacterInput(const CharacterInputEvent& ev)
{
    return pData->characterInputEvent(ev);
}

bool TopLevelWidget::onMouse(const MouseEvent& ev)
{
    return pData->mouseEvent(ev);
}

bool TopLevelWidget::onMotion(const MotionEvent& ev)
{
    return pData->motionEvent(ev);
}

bool TopLevelWidget::onScroll(const ScrollEvent& ev)
{
    return pData->scrollEvent(ev);
}

// --------------------------------------------------------------------------------------------------------------------

void TopLevelWidget::requestSizeChange(uint, uint)
{
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
