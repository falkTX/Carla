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

#ifndef DGL_NANO_WIDGETS_HPP_INCLUDED
#define DGL_NANO_WIDGETS_HPP_INCLUDED

#include "NanoVG.hpp"
#include "../distrho/extra/String.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

class BlendishButton : public NanoWidget
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void blendishButtonClicked(BlendishButton* blendishButton, int button) = 0;
    };

    explicit BlendishButton(Window& parent, const char* text = "", int iconId = -1);
    explicit BlendishButton(NanoWidget* widget, const char* text = "", int iconId = -1);
    ~BlendishButton() override;

    int getIconId() const noexcept;
    void setIconId(int iconId) noexcept;

    const char* getText() const noexcept;
    void setText(const char* text) noexcept;

    void setCallback(Callback* callback) noexcept;

protected:
     void onNanoDisplay() override;
     bool onMouse(const MouseEvent&) override;
     bool onMotion(const MotionEvent&) override;

private:
    struct PrivateData;
    PrivateData* const pData;

    void _updateBounds();

    DISTRHO_LEAK_DETECTOR(BlendishButton)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_NANO_WIDGETS_HPP_INCLUDED
