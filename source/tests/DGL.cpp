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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "dgl/App.hpp"
#include "dgl/Geometry.hpp"
#include "dgl/Image.hpp"
#include "dgl/ImageAboutWindow.hpp"
#include "dgl/ImageButton.hpp"
#include "dgl/ImageKnob.hpp"
#include "dgl/ImageSlider.hpp"
#include "dgl/Widget.hpp"
#include "dgl/Window.hpp"

#include <cstdio>

int main()
{
    USE_NAMESPACE_DGL;

    msleep(1);

    Point<int> pi;
    Point<long> pl;
    Point<float> pf;
    Point<double> pd;
    Point<unsigned> pu;

    Size<int> si;
    Size<long> sl;
    Size<float> sf;
    Size<double> sd;
    Size<unsigned> su;

    Rectangle<int> ri;
    Rectangle<long> rl;
    Rectangle<float> rf;
    Rectangle<double> rd;
    Rectangle<unsigned> ru;

    Image i;

    App app;
    Window win(app);
    win.setSize(500, 500);
    win.show();

//     for (int i=0; i < 1000; ++i)
//     {
//         app.idle();
//
//         if (app.isQuiting())
//             break;

//         if (i % 30 == 0)
//             printf("SIZE: %i:%i\n", win.getWidth(), win.getHeight());

//         msleep(10);
//     }

    app.exec();

    return 0;
}

#include "dgl/src/ImageAboutWindow.cpp"
#include "dgl/src/App.cpp"
#include "dgl/src/Base.cpp"
#include "dgl/src/Geometry.cpp"
#include "dgl/src/Image.cpp"
#include "dgl/src/ImageButton.cpp"
#include "dgl/src/ImageKnob.cpp"
#include "dgl/src/ImageSlider.cpp"
#include "dgl/src/Widget.cpp"
#include "dgl/src/Window.cpp"
