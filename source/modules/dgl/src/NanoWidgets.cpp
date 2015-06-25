/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
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

#include "../NanoWidgets.hpp"
#include "Common.hpp"

#define BLENDISH_IMPLEMENTATION
#include "nanovg/nanovg.h"
#include "oui-blendish/blendish.h"
#include "oui-blendish/blendish_resources.h"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

static void registerBlendishResourcesIfNeeded(NVGcontext* const context)
{
    if (nvgFindFont(context, "__dpf_blendish__") >= 0)
        return;

    using namespace blendish_resources;

    bndSetFont(nvgCreateFontMem(context, "__dpf_blendish__", (const uchar*)dejavusans_ttf, dejavusans_ttf_size, 0));
    bndSetIconImage(nvgCreateImageMem(context, 0, (const uchar*)blender_icons16_png, blender_icons16_png_size));
}

// -----------------------------------------------------------------------

struct BlendishButton::PrivateData {
    ButtonImpl impl;
    int iconId;
    DISTRHO_NAMESPACE::String text;

    PrivateData(Widget* const s, const char* const t, const int i) noexcept
        : impl(s),
          iconId(i),
          text(t) {}

    DISTRHO_DECLARE_NON_COPY_STRUCT(PrivateData)
};

// -----------------------------------------------------------------------

BlendishButton::BlendishButton(Window& parent, const char* text, int iconId)
    : NanoWidget(parent),
      pData(new PrivateData(this, text, iconId))
{
    registerBlendishResourcesIfNeeded(getContext());
    _updateBounds();
}

BlendishButton::BlendishButton(NanoWidget* widget, const char* text, int iconId)
    : NanoWidget(widget),
      pData(new PrivateData(this, text, iconId))
{
    registerBlendishResourcesIfNeeded(getContext());
    _updateBounds();
}

BlendishButton::~BlendishButton()
{
    delete pData;
}

int BlendishButton::getIconId() const noexcept
{
    return pData->iconId;
}

void BlendishButton::setIconId(int iconId) noexcept
{
    if (pData->iconId == iconId)
        return;

    pData->iconId = iconId;
    _updateBounds();
    repaint();
}

const char* BlendishButton::getText() const noexcept
{
    return pData->text;
}

void BlendishButton::setText(const char* text) noexcept
{
    if (pData->text == text)
        return;

    pData->text = text;
    _updateBounds();
    repaint();
}

void BlendishButton::setCallback(Callback* callback) noexcept
{
    pData->impl.callback_b = callback;
}

void BlendishButton::onNanoDisplay()
{
    bndToolButton(getContext(),
                  getAbsoluteX(), getAbsoluteY(), getWidth(), getHeight(),
                  0, static_cast<BNDwidgetState>(pData->impl.state), pData->iconId, pData->text);
}

bool BlendishButton::onMouse(const MouseEvent& ev)
{
    return pData->impl.onMouse(ev);
}

bool BlendishButton::onMotion(const MotionEvent& ev)
{
    return pData->impl.onMotion(ev);
}

void BlendishButton::_updateBounds()
{
    const float width  = bndLabelWidth (getContext(), pData->iconId, pData->text);
    const float height = bndLabelHeight(getContext(), pData->iconId, pData->text, width);

    setSize(width, height);
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#include "oui-blendish/blendish_resources.cpp"

// -----------------------------------------------------------------------
