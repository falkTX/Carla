/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * For a full copy of the license see the LGPL.txt file
 */

#ifndef __DGL_IMAGE_BUTTON_HPP__
#define __DGL_IMAGE_BUTTON_HPP__

#include "Image.hpp"
#include "Widget.hpp"

START_NAMESPACE_DGL

// -------------------------------------------------

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
    ImageButton(Window* parent, const Image& imageNormal, const Image& imageHover, const Image& imageDown);
    ImageButton(const ImageButton& imageButton);

    void setCallback(Callback* callback);

protected:
     void onDisplay();
     bool onMouse(int button, bool press, int x, int y);
     bool onMotion(int x, int y);

private:
    Image  fImageNormal;
    Image  fImageHover;
    Image  fImageDown;
    Image* fCurImage;
    int    fCurButton;

    Callback* fCallback;
};

// -------------------------------------------------

END_NAMESPACE_DGL

#endif // __DGL_IMAGE_BUTTON_HPP__
