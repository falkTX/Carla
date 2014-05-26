/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
 * Copyright (C) 2014 Damien Zammit <damien@zamaudio.com>
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

#ifndef DGL_IMAGE_TOGGLE_HPP_INCLUDED
#define DGL_IMAGE_TOGGLE_HPP_INCLUDED

#include "Image.hpp"
#include "Widget.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

class ImageToggle : public Widget
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void imageToggleClicked(ImageToggle* imageToggle, int button) = 0;
    };

    ImageToggle(Window& parent, const Image& imageNormal, const Image& imageDown, int id = 0)
        : Widget(parent),
          fImageNormal(imageNormal),
          fImageDown(imageDown),
          fCurImage(&fImageNormal),
          fId(id),
          fCallback(nullptr)
    {
        DISTRHO_SAFE_ASSERT(fImageNormal.getSize() == fImageDown.getSize());

        setSize(fImageNormal.getSize());
    }

    ImageToggle(Widget* widget, const Image& imageNormal, const Image& imageDown, int id = 0)
        : Widget(widget->getParentWindow()),
          fImageNormal(imageNormal),
          fImageDown(imageDown),
          fCurImage(&fImageNormal),
          fId(id),
          fCallback(nullptr)
    {
        DISTRHO_SAFE_ASSERT(fImageNormal.getSize() == fImageDown.getSize());

        setSize(fImageNormal.getSize());
    }

    ImageToggle(const ImageToggle& imageToggle)
        : Widget(imageToggle.getParentWindow()),
          fImageNormal(imageToggle.fImageNormal),
          fImageDown(imageToggle.fImageDown),
          fCurImage(&fImageNormal),
          fId(imageToggle.fId),
          fCallback(imageToggle.fCallback)
    {
        DISTRHO_SAFE_ASSERT(fImageNormal.getSize() == fImageDown.getSize());

        setSize(fImageNormal.getSize());
    }

    int getId() const noexcept
    {
        return fId;
    }

    void setId(int id) noexcept
    {
        fId = id;
    }

    float getValue() const noexcept
    {
        return (fCurImage == &fImageNormal) ? 0.0f : 1.0f;
    }

    void setValue(float value) noexcept
    {
        if (value == 1.0f)
            fCurImage = &fImageDown;
        else if (value == 0.0f)
            fCurImage = &fImageNormal;

        repaint();
    }

    void setCallback(Callback* callback) noexcept
    {
        fCallback = callback;
    }

protected:
     void onDisplay() override
    {
        fCurImage->draw();
    }

     bool onMouse(const MouseEvent& ev) override
    {
        if (! ev.press)
            return false;
        if (! contains(ev.pos))
            return false;

        if (fCurImage != &fImageDown)
            fCurImage = &fImageDown;
        else if (fCurImage != &fImageNormal)
            fCurImage = &fImageNormal;

        repaint();

        if (fCallback != nullptr)
            fCallback->imageToggleClicked(this, ev.button);

        return true;
    }

private:
    Image  fImageNormal;
    Image  fImageDown;
    Image* fCurImage;
    int    fId;

    Callback* fCallback;

    DISTRHO_LEAK_DETECTOR(ImageToggle)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_IMAGE_TOGGLE_HPP_INCLUDED
