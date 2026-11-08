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

#ifndef DGL_SUBWIDGET_PRIVATE_DATA_HPP_INCLUDED
#define DGL_SUBWIDGET_PRIVATE_DATA_HPP_INCLUDED

#include "../SubWidget.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

struct SubWidget::PrivateData {
    SubWidget* const self;
    Widget* const selfw;
    Widget* const parentWidget;
    Point<int> absolutePos;
    Point<int> margin;
    bool needsFullViewportForDrawing; // needed for widgets drawing out of bounds
    bool needsViewportScaling; // needed for NanoVG
    bool skipDrawing; // for context reuse in NanoVG based guis
    double viewportScaleFactor; // auto-scaling for NanoVG

    explicit PrivateData(SubWidget* const s, Widget* const pw);
    ~PrivateData();

    // NOTE display function is different depending on build type, must call displaySubWidgets at the end
    void display(uint width, uint height, double autoScaleFactor);

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_SUBWIDGET_PRIVATE_DATA_HPP_INCLUDED
