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

#include "../ImageButton.hpp"

#include <cassert>

START_NAMESPACE_DGL

// -------------------------------------------------

ImageButton::ImageButton(Window* parent, const Image& image)
    : Widget(parent),
      fImageNormal(image),
      fImageHover(image),
      fImageDown(image),
      fCurImage(&fImageNormal),
      fCurButton(-1),
      fCallback(nullptr)
{
}

ImageButton::ImageButton(Window* parent, const Image& imageNormal, const Image& imageHover, const Image& imageDown)
    : Widget(parent),
      fImageNormal(imageNormal),
      fImageHover(imageHover),
      fImageDown(imageDown),
      fCurImage(&fImageNormal),
      fCurButton(-1),
      fCallback(nullptr)
{
    assert(fImageNormal.getSize() == fImageHover.getSize() && fImageHover.getSize() == fImageDown.getSize());

    setSize(fCurImage->getSize());
}

ImageButton::ImageButton(const ImageButton& imageButton)
    : Widget(imageButton.getParent()),
      fImageNormal(imageButton.fImageNormal),
      fImageHover(imageButton.fImageHover),
      fImageDown(imageButton.fImageDown),
      fCurImage(&fImageNormal),
      fCurButton(-1),
      fCallback(imageButton.fCallback)
{
    assert(fImageNormal.getSize() == fImageHover.getSize() && fImageHover.getSize() == fImageDown.getSize());

    setSize(fCurImage->getSize());
}

void ImageButton::setCallback(Callback* callback)
{
    fCallback = callback;
}

void ImageButton::onDisplay()
{
    fCurImage->draw(getPos());
}

bool ImageButton::onMouse(int button, bool press, int x, int y)
{
    if (fCurButton != -1 && ! press)
    {
        if (fCurImage != &fImageNormal)
        {
            fCurImage = &fImageNormal;
            repaint();
        }

        if (! getArea().contains(x, y))
        {
            fCurButton = -1;
            return false;
        }

        if (fCallback != nullptr)
            fCallback->imageButtonClicked(this, fCurButton);

        //if (getArea().contains(x, y))
        //{
        //    fCurImage = &fImageHover;
        //    repaint();
        //}

        fCurButton = -1;

        return true;
    }

    if (press && getArea().contains(x, y))
    {
        if (fCurImage != &fImageDown)
        {
            fCurImage = &fImageDown;
            repaint();
        }

        fCurButton = button;
        return true;
    }

    return false;
}

bool ImageButton::onMotion(int x, int y)
{
    if (fCurButton != -1)
        return true;

    if (getArea().contains(x, y))
    {
        if (fCurImage != &fImageHover)
        {
            fCurImage = &fImageHover;
            repaint();
        }

        return true;
    }
    else
    {
        if (fCurImage != &fImageNormal)
        {
            fCurImage = &fImageNormal;
            repaint();
        }

        return false;
    }
}

// -------------------------------------------------

END_NAMESPACE_DGL
