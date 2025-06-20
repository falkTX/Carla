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

#ifndef DGL_WIDGET_PRIVATE_DATA_HPP_INCLUDED
#define DGL_WIDGET_PRIVATE_DATA_HPP_INCLUDED

#include "../Widget.hpp"

#include <list>

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

struct Widget::PrivateData {
    Widget* const self;
    TopLevelWidget* const topLevelWidget;
    Widget* const parentWidget;
    uint id;
    char* name;
    bool needsScaling;
    bool visible;
    Size<uint> size;
    std::list<SubWidget*> subWidgets;

    // called via TopLevelWidget
    explicit PrivateData(Widget* const s, TopLevelWidget* const tlw);
    // called via SubWidget
    explicit PrivateData(Widget* const s, Widget* const pw);
    ~PrivateData();

    void displaySubWidgets(uint width, uint height, double autoScaleFactor);

    bool giveKeyboardEventForSubWidgets(const KeyboardEvent& ev);
    bool giveCharacterInputEventForSubWidgets(const CharacterInputEvent& ev);
    bool giveMouseEventForSubWidgets(MouseEvent& ev);
    bool giveMotionEventForSubWidgets(MotionEvent& ev);
    bool giveScrollEventForSubWidgets(ScrollEvent& ev);

    static TopLevelWidget* findTopLevelWidget(Widget* const w);

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_WIDGET_PRIVATE_DATA_HPP_INCLUDED
