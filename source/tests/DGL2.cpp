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
#include "CarlaUtils.hpp"

#include "NekoArtwork.hpp"

// this is just a test file!
#include <QtCore/QThread>

USE_NAMESPACE_DGL;

class NekoWidget : public Widget,
                          QThread
{
public:
    NekoWidget(StandaloneWindow* win)
        : Widget(win->getWindow()),
          fPos(0),
          fTimer(0),
          fTimerSpeed(20),
          fCurAction(kActionNone),
          fCurImage(&fImages.sit)
    {
        // load images
        {
            using namespace NekoArtwork;

            fImages.background.loadFromMemory(backgroundData, backgroundWidth, backgroundHeight, GL_BGR);

            #define JOIN(a, b) a ## b
            #define LOAD_IMAGE(NAME) fImages.NAME.loadFromMemory(JOIN(NAME, Data), JOIN(NAME, Width), JOIN(NAME, Height));

            LOAD_IMAGE(sit)
            LOAD_IMAGE(tail)
            LOAD_IMAGE(claw1)
            LOAD_IMAGE(claw2)
            LOAD_IMAGE(scratch1)
            LOAD_IMAGE(scratch2)
            LOAD_IMAGE(run1)
            LOAD_IMAGE(run2)
            LOAD_IMAGE(run3)
            LOAD_IMAGE(run4)

            #undef JOIN
            #undef LOAD_IMAGE
        }

        QThread::start();
    }

    ~NekoWidget()
    {
        hide();
        QThread::quit();
        QThread::wait();
    }

protected:
    void onDisplay() override
    {
        int x = fPos+118;
        int y = -2;

        if (fCurImage == &fImages.claw1 || fCurImage == &fImages.claw2)
        {
            x += 2;
            y += 12;
        }

        fImages.background.draw();
        fCurImage->draw(x, y);
    }

    void idle()
    {
        if (++fTimer % fTimerSpeed != 0) // target is 25ms
            return;

        if (fTimer == fTimerSpeed*9)
        {
            if (fCurAction == kActionNone)
                fCurAction = static_cast<Action>(rand() % kActionCount);
            else
                fCurAction = kActionNone;

            fTimer = 0;
        }

        switch (fCurAction)
        {
        case kActionNone:
            if (fCurImage == &fImages.sit)
                fCurImage = &fImages.tail;
            else
                fCurImage = &fImages.sit;
            break;

        case kActionClaw:
            if (fCurImage == &fImages.claw1)
                fCurImage = &fImages.claw2;
            else
                fCurImage = &fImages.claw1;
            break;

        case kActionScratch:
            if (fCurImage == &fImages.scratch1)
                fCurImage = &fImages.scratch2;
            else
                fCurImage = &fImages.scratch1;
            break;

        case kActionRunRight:
            if (fTimer == 0 && fPos > 20*9)
            {
                // run the other way
                --fTimer;
                fCurAction = kActionRunLeft;
                idle();
                break;
            }

            fPos += 20;

            if (fCurImage == &fImages.run1)
                fCurImage = &fImages.run2;
            else
                fCurImage = &fImages.run1;
            break;

        case kActionRunLeft:
            if (fTimer == 0 && fPos < 20*9)
            {
                // run the other way
                --fTimer;
                fCurAction = kActionRunRight;
                idle();
                break;
            }

            fPos -= 20;

            if (fCurImage == &fImages.run3)
                fCurImage = &fImages.run4;
            else
                fCurImage = &fImages.run3;
            break;

        case kActionCount:
            break;
        }

        repaint();
    }

    void run() override
    {
        while (isVisible())
        {
            idle();
            carla_msleep(10);
        }
    }

private:
    enum Action {
        kActionNone, // bounce tail
        kActionClaw,
        kActionScratch,
        kActionRunRight,
        kActionRunLeft,
        kActionCount
    };

    struct Images {
        Image background;
        Image sit;
        Image tail;
        Image claw1;
        Image claw2;
        Image scratch1;
        Image scratch2;
        Image run1;
        Image run2;
        Image run3;
        Image run4;
    } fImages;

    int fPos;
    int fTimer;
    int fTimerSpeed;

    Action fCurAction;
    Image* fCurImage;
};

int main()
{
    StandaloneWindow win;
    win.setSize(NekoArtwork::backgroundWidth, NekoArtwork::backgroundHeight);
    win.setWindowTitle("Neko Test");
    NekoWidget widget(&win);
    win.exec();
    return 0;
}
