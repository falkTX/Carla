/*
 * Carla Tests
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "dgl/Image.hpp"
#include "dgl/StandaloneWindow.hpp"

#include "DGL1_Artwork.hpp"

USE_NAMESPACE_DGL;

class MyWidget : public Widget
{
public:
    MyWidget(StandaloneWindow* win)
        : Widget(win->getWindow()),
          fWindow(win->getWindow())
    {
        {
            using namespace DGL1_Artwork;
            fImage.loadFromMemory(start_here_kxstudioData,
                                  Size<int>(start_here_kxstudioWidth, start_here_kxstudioHeight),
                                  GL_BGRA, GL_UNSIGNED_BYTE);
        }

        fWindow->setSize(fImage.getWidth(), fImage.getHeight());
        fWindow->setWindowTitle("DGL Test");
    }

protected:
    void onDisplay() override
    {
        fImage.draw();
    }

private:
    Window* const fWindow;

    Image fImage;
};

int main()
{
    StandaloneWindow win;
    MyWidget widget(&win);
    win.exec();
    return 0;
}
