/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef DGL_IMAGE_SWITCH_HPP_INCLUDED
#define DGL_IMAGE_SWITCH_HPP_INCLUDED

#include "Image.hpp"
#include "Widget.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

class ImageSwitch : public Widget
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void imageSwitchClicked(ImageSwitch* imageButton, bool down) = 0;
    };

    explicit ImageSwitch(Window& parent, const Image& imageNormal, const Image& imageDown, int id = 0) noexcept;
    explicit ImageSwitch(Widget* widget, const Image& imageNormal, const Image& imageDown, int id = 0) noexcept;
    explicit ImageSwitch(const ImageSwitch& imageSwitch) noexcept;
    ImageSwitch& operator=(const ImageSwitch& imageSwitch) noexcept;

    int getId() const noexcept;
    void setId(int id) noexcept;

    bool isDown() const noexcept;
    void setDown(bool down) noexcept;

    void setCallback(Callback* callback) noexcept;

protected:
     void onDisplay() override;
     bool onMouse(const MouseEvent&) override;

private:
    Image fImageNormal;
    Image fImageDown;
    bool  fIsDown;
    int   fId;

    Callback* fCallback;

    DISTRHO_LEAK_DETECTOR(ImageSwitch)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_IMAGE_SWITCH_HPP_INCLUDED
