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

#include "CarlaStyle.hpp"
#include "digitalpeakmeter.hpp"
#include "ledbutton.hpp"
#include "paramspinbox.hpp"
#include "pixmapdial.hpp"
#include "pixmapkeyboard.hpp"

#include <QtGui/QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setStyle(new CarlaStyle());

    DigitalPeakMeter w1(nullptr);
    LEDButton w2(nullptr);
    ParamProgressBar w3(nullptr);
    PixmapDial w4(nullptr);
    PixmapKeyboard w5(nullptr);

    w4.setPixmap(2);

    w1.show();
    w2.show();
    w3.show();
    w4.show();
    w5.show();

    return app.exec();
}
