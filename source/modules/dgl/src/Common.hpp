/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
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

#ifndef DGL_COMMON_HPP_INCLUDED
#define DGL_COMMON_HPP_INCLUDED

#include "../ImageWidgets.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

struct ButtonImpl {
    enum State {
        kStateNormal = 0,
        kStateHover,
        kStateDown
    };

    int button;
    int state;
    Widget* self;

    ImageButton::Callback* callback_img;

    ButtonImpl(Widget* const s) noexcept
        : button(-1),
          state(kStateNormal),
          self(s),
          callback_img(nullptr) {}

    bool onMouse(const Widget::MouseEvent& ev)
    {
        // button was released, handle it now
        if (button != -1 && ! ev.press)
        {
            DISTRHO_SAFE_ASSERT(state == kStateDown);

            // release button
            const int button2 = button;
            button = -1;

            // cursor was moved outside the button bounds, ignore click
            if (! self->contains(ev.pos))
            {
                state = kStateNormal;
                self->repaint();
                return true;
            }

            // still on bounds, register click
            state = kStateHover;
            self->repaint();

            if (callback_img != nullptr)
                callback_img->imageButtonClicked((ImageButton*)self, button2);

            return true;
        }

        // button was pressed, wait for release
        if (ev.press && self->contains(ev.pos))
        {
            button = ev.button;
            state  = kStateDown;
            self->repaint();
            return true;
        }

        return false;
    }

    bool onMotion(const Widget::MotionEvent& ev)
    {
        // keep pressed
        if (button != -1)
            return true;

        if (self->contains(ev.pos))
        {
            // check if entering hover
            if (state == kStateNormal)
            {
                state = kStateHover;
                self->repaint();
                return true;
            }
        }
        else
        {
            // check if exiting hover
            if (state == kStateHover)
            {
                state = kStateNormal;
                self->repaint();
                return true;
            }
        }

        return false;
    }

    DISTRHO_PREVENT_HEAP_ALLOCATION
    DISTRHO_DECLARE_NON_COPY_STRUCT(ButtonImpl)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_APP_PRIVATE_DATA_HPP_INCLUDED
