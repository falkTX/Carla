/*
 * DISTRHO ProM Plugin
 * Copyright (C) 2014 Filipe Coelho <falktx@falktx.com>
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

// -----------------------------------------------------------------------

DistrhoUIProM::DistrhoUIProM()
    : UI()
{
}

DistrhoUIProM::~DistrhoUIProM()
{
    if (DistrhoPluginProM* const dspPtr = (DistrhoPluginProM*)d_getPluginInstancePointer())
    {
        const MutexLocker csm(dspPtr->fMutex);
        dspPtr->fPM = nullptr;
    }
}

// -----------------------------------------------------------------------
// DSP Callbacks

void DistrhoUIProM::d_parameterChanged(uint32_t, float)
{
}

// -----------------------------------------------------------------------
// UI Callbacks

void DistrhoUIProM::d_uiIdle()
{
    if (fPM == nullptr)
        return;

    repaint();

    if (DistrhoPluginProM* const dspPtr = (DistrhoPluginProM*)d_getPluginInstancePointer())
    {
        if (dspPtr->fPM != nullptr)
            return;

        const MutexLocker csm(dspPtr->fMutex);
        dspPtr->fPM = fPM;
    }
}

void DistrhoUIProM::d_uiReshape(int width, int height)
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
        fPM = new projectM(kSettings); // std::string("/usr/share/projectM/config.inp"));

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
        case DGL::CHAR_BACKSPACE:
            pmKey = PROJECTM_K_BACKSPACE;
            break;
        case DGL::CHAR_ESCAPE:
            pmKey = PROJECTM_K_ESCAPE;
            break;
        case DGL::CHAR_DELETE:
            pmKey = PROJECTM_K_DELETE;
            break;
        }
    }

    if (pmKey == PROJECTM_K_NONE)
        return false;

    if (ev.mod & DGL::MODIFIER_CTRL)
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
    case DGL::KEY_F1:
        pmKey = PROJECTM_K_F1;
        break;
    case DGL::KEY_F2:
        pmKey = PROJECTM_K_F2;
        break;
    case DGL::KEY_F3:
        pmKey = PROJECTM_K_F3;
        break;
    case DGL::KEY_F4:
        pmKey = PROJECTM_K_F4;
        break;
    case DGL::KEY_F5:
        pmKey = PROJECTM_K_F5;
        break;
    case DGL::KEY_F6:
        pmKey = PROJECTM_K_F6;
        break;
    case DGL::KEY_F7:
        pmKey = PROJECTM_K_F7;
        break;
    case DGL::KEY_F8:
        pmKey = PROJECTM_K_F8;
        break;
    case DGL::KEY_F9:
        pmKey = PROJECTM_K_F9;
        break;
    case DGL::KEY_F10:
        pmKey = PROJECTM_K_F10;
        break;
    case DGL::KEY_F11:
        pmKey = PROJECTM_K_F11;
        break;
    case DGL::KEY_F12:
        pmKey = PROJECTM_K_F12;
        break;
    case DGL::KEY_LEFT:
        pmKey = PROJECTM_K_LEFT;
        break;
    case DGL::KEY_UP:
        pmKey = PROJECTM_K_UP;
        break;
    case DGL::KEY_RIGHT:
        pmKey = PROJECTM_K_RIGHT;
        break;
    case DGL::KEY_DOWN:
        pmKey = PROJECTM_K_DOWN;
        break;
    case DGL::KEY_PAGE_UP:
        pmKey = PROJECTM_K_PAGEUP;
        break;
    case DGL::KEY_PAGE_DOWN:
        pmKey = PROJECTM_K_PAGEDOWN;
        break;
    case DGL::KEY_HOME:
        pmKey = PROJECTM_K_HOME;
        break;
    case DGL::KEY_END:
        pmKey = PROJECTM_K_END;
        break;
    case DGL::KEY_INSERT:
        pmKey = PROJECTM_K_INSERT;
        break;
    case DGL::KEY_SHIFT:
        pmKey = PROJECTM_K_LSHIFT;
        break;
    case DGL::KEY_CTRL:
        pmKey = PROJECTM_K_LCTRL;
        break;
    case DGL::KEY_ALT:
    case DGL::KEY_SUPER:
        break;
    }

    if (pmKey == PROJECTM_K_NONE)
        return false;

    if (ev.mod & DGL::MODIFIER_CTRL)
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
