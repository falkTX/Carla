/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

#ifndef DGL_IMAGE_BUTTON_HPP_INCLUDED
#define DGL_IMAGE_BUTTON_HPP_INCLUDED

#include "Image.hpp"
#include "Widget.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

class ImageButton : public Widget
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void imageButtonClicked(ImageButton* imageButton, int button) = 0;
    };

    ImageButton(Window* parent, const Image& image);
    ImageButton(Widget* widget, const Image& image);
    ImageButton(Window* parent, const Image& imageNormal, const Image& imageHover, const Image& imageDown);
    ImageButton(Widget* widget, const Image& imageNormal, const Image& imageHover, const Image& imageDown);
    ImageButton(const ImageButton& imageButton);

    void setCallback(Callback* callback);

protected:
     void onDisplay() override;
     bool onMouse(int button, bool press, int x, int y) override;
     bool onMotion(int x, int y) override;

private:
    Image  fImageNormal;
    Image  fImageHover;
    Image  fImageDown;
    Image* fCurImage;
    int    fCurButton;

    Callback* fCallback;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_IMAGE_BUTTON_HPP_INCLUDED
