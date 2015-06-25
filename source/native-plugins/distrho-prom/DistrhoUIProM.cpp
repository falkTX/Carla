/*
 * DISTRHO ProM Plugin
 * Copyright (C) 2015 Filipe Coelho <falktx@falktx.com>
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
 * For a full copy of the license see the LICENSE file.
 */

#include "DistrhoPluginProM.hpp"
#include "DistrhoUIProM.hpp"

#include "libprojectM/projectM.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

#if 0
static const projectM::Settings kSettings = {
    /* meshX        */ 32,
    /* meshY        */ 24,
    /* fps          */ 35,
    /* textureSize  */ 1024,
    /* windowWidth  */ 512,
    /* windowHeight */ 512,
    /* presetURL    */ "/usr/share/projectM/presets",
    /* titleFontURL */ "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans.ttf",
    /* menuFontURL  */ "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSansMono.ttf",
    /* smoothPresetDuration  */ 5,
    /* presetDuration        */ 30,
    /* beatSensitivity       */ 10.0f,
    /* aspectCorrection      */ true,
    /* easterEgg             */ 1.0f,
    /* shuffleEnabled        */ true,
    /* softCutRatingsEnabled */ false
};
#endif

// -----------------------------------------------------------------------

DistrhoUIProM::DistrhoUIProM()
    : UI(512, 512)
{
}

DistrhoUIProM::~DistrhoUIProM()
{
    if (DistrhoPluginProM* const dspPtr = (DistrhoPluginProM*)getPluginInstancePointer())
    {
        const MutexLocker csm(dspPtr->fMutex);
        dspPtr->fPM = nullptr;
    }
}

// -----------------------------------------------------------------------
// DSP Callbacks

void DistrhoUIProM::parameterChanged(uint32_t, float)
{
}

// -----------------------------------------------------------------------
// UI Callbacks

void DistrhoUIProM::uiIdle()
{
    if (fPM == nullptr)
        return;

    repaint();

    if (DistrhoPluginProM* const dspPtr = (DistrhoPluginProM*)getPluginInstancePointer())
    {
        if (dspPtr->fPM != nullptr)
            return;

        const MutexLocker csm(dspPtr->fMutex);
        dspPtr->fPM = fPM;
    }
}

void DistrhoUIProM::uiReshape(uint width, uint height)
{
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel(GL_SMOOTH);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, 0.0f, 1.0f);
    glViewport(0, 0, width, height);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDrawBuffer(GL_BACK);
    glReadBuffer(GL_BACK);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glLineStipple(2, 0xAAAA);

    if (fPM == nullptr)
        //fPM = new projectM(kSettings);
        fPM = new projectM("/usr/share/projectM/config.inp");

    fPM->projectM_resetGL(width, height);
}

// -----------------------------------------------------------------------
// Widget Callbacks

void DistrhoUIProM::onDisplay()
{
    if (fPM == nullptr)
        return;

    fPM->renderFrame();
}

bool DistrhoUIProM::onKeyboard(const KeyboardEvent& ev)
{
    if (fPM == nullptr)
        return false;

    if (ev.press && (ev.key == '1' || ev.key == '+' || ev.key == '-'))
    {
        if (ev.key == '1')
        {
            if (getWidth() != 512 || getHeight() != 512)
                setSize(512, 512);
        }
        else if (ev.key == '+')
        {
            /**/ if (getWidth() < 1100 && getHeight() < 1100)
                setSize(getWidth()+100, getHeight()+100);
            else if (getWidth() != 1100 || getHeight() != 1100)
                    setSize(1100, 1100);
        }
        else if (ev.key == '-')
        {
            /**/ if (getWidth() >= 200 && getHeight() >= 200)
                setSize(getWidth()-100, getHeight()-100);
            else if (getWidth() != 100 || getHeight() != 100)
                setSize(100, 100);
        }

        return true;
    }

    projectMKeycode  pmKey = PROJECTM_K_NONE;
    projectMModifier pmMod = PROJECTM_KMOD_LSHIFT;

    if ((ev.key >= PROJECTM_K_0 && ev.key <= PROJECTM_K_9) ||
        (ev.key >= PROJECTM_K_A && ev.key <= PROJECTM_K_Z) ||
        (ev.key >= PROJECTM_K_a && ev.key <= PROJECTM_K_z))
    {
        pmKey = static_cast<projectMKeycode>(ev.key);
    }
    else
    {
        switch (ev.key)
        {
        case DGL::kCharBackspace:
            pmKey = PROJECTM_K_BACKSPACE;
            break;
        case DGL::kCharEscape:
            pmKey = PROJECTM_K_ESCAPE;
            break;
        case DGL::kCharDelete:
            pmKey = PROJECTM_K_DELETE;
            break;
        }
    }

    if (pmKey == PROJECTM_K_NONE)
        return false;

    if (ev.mod & DGL::kModifierControl)
        pmMod = PROJECTM_KMOD_LCTRL;

    fPM->key_handler(ev.press ? PROJECTM_KEYUP : PROJECTM_KEYDOWN, pmKey, pmMod);

    return true;
}

bool DistrhoUIProM::onSpecial(const SpecialEvent& ev)
{
    if (fPM == nullptr)
        return false;

    projectMKeycode  pmKey = PROJECTM_K_NONE;
    projectMModifier pmMod = PROJECTM_KMOD_LSHIFT;

    switch (ev.key)
    {
    case DGL::kKeyF1:
        pmKey = PROJECTM_K_F1;
        break;
    case DGL::kKeyF2:
        pmKey = PROJECTM_K_F2;
        break;
    case DGL::kKeyF3:
        pmKey = PROJECTM_K_F3;
        break;
    case DGL::kKeyF4:
        pmKey = PROJECTM_K_F4;
        break;
    case DGL::kKeyF5:
        pmKey = PROJECTM_K_F5;
        break;
    case DGL::kKeyF6:
        pmKey = PROJECTM_K_F6;
        break;
    case DGL::kKeyF7:
        pmKey = PROJECTM_K_F7;
        break;
    case DGL::kKeyF8:
        pmKey = PROJECTM_K_F8;
        break;
    case DGL::kKeyF9:
        pmKey = PROJECTM_K_F9;
        break;
    case DGL::kKeyF10:
        pmKey = PROJECTM_K_F10;
        break;
    case DGL::kKeyF11:
        pmKey = PROJECTM_K_F11;
        break;
    case DGL::kKeyF12:
        pmKey = PROJECTM_K_F12;
        break;
    case DGL::kKeyLeft:
        pmKey = PROJECTM_K_LEFT;
        break;
    case DGL::kKeyUp:
        pmKey = PROJECTM_K_UP;
        break;
    case DGL::kKeyRight:
        pmKey = PROJECTM_K_RIGHT;
        break;
    case DGL::kKeyDown:
        pmKey = PROJECTM_K_DOWN;
        break;
    case DGL::kKeyPageUp:
        pmKey = PROJECTM_K_PAGEUP;
        break;
    case DGL::kKeyPageDown:
        pmKey = PROJECTM_K_PAGEDOWN;
        break;
    case DGL::kKeyHome:
        pmKey = PROJECTM_K_HOME;
        break;
    case DGL::kKeyEnd:
        pmKey = PROJECTM_K_END;
        break;
    case DGL::kKeyInsert:
        pmKey = PROJECTM_K_INSERT;
        break;
    case DGL::kKeyShift:
        pmKey = PROJECTM_K_LSHIFT;
        break;
    case DGL::kKeyControl:
        pmKey = PROJECTM_K_LCTRL;
        break;
    case DGL::kKeyAlt:
    case DGL::kKeySuper:
        break;
    }

    if (pmKey == PROJECTM_K_NONE)
        return false;

    if (ev.mod & DGL::kModifierControl)
        pmMod = PROJECTM_KMOD_LCTRL;

    fPM->key_handler(ev.press ? PROJECTM_KEYUP : PROJECTM_KEYDOWN, pmKey, pmMod);

    return true;
}

// -----------------------------------------------------------------------

UI* createUI()
{
    return new DistrhoUIProM();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
