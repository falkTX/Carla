/*
 * Carla Native Plugins
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaNative.hpp"
#include "CarlaMutex.hpp"

#include "dgl/StandaloneWindow.hpp"
#include "dgl/Widget.hpp"

#include "libprojectM/projectM.hpp"

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

class ProjectMWidget : public DGL::Widget
{
public:
    ProjectMWidget(DGL::Window& parent)
        : DGL::Widget(parent),
          pm(nullptr)
    {
    }

    ~ProjectMWidget()
    {
        if (pm == nullptr)
            return;

        delete pm;
        pm = nullptr;
    }

    projectM* getPM() noexcept
    {
        return pm;
    }

protected:
    void onDisplay() override
    {
        if (pm == nullptr)
            return;

        pm->renderFrame();
    }

    void onClose() override
    {
    }

    bool onKeyboard(const bool press, const uint32_t key) override
    {
        if (pm == nullptr)
            return false;

        projectMKeycode  pmKey = PROJECTM_K_NONE;
        projectMModifier pmMod = PROJECTM_KMOD_LSHIFT;

        if ((key >= PROJECTM_K_0 && key <= PROJECTM_K_9) ||
            (key >= PROJECTM_K_A && key <= PROJECTM_K_Z) ||
            (key >= PROJECTM_K_a && key <= PROJECTM_K_z))
        {
            pmKey = static_cast<projectMKeycode>(key);
        }
        else
        {
            switch (key)
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

        if (const int mod = getModifiers())
        {
            if (mod & DGL::MODIFIER_CTRL)
                pmMod = PROJECTM_KMOD_LCTRL;
        }

        pm->key_handler(press ? PROJECTM_KEYUP : PROJECTM_KEYDOWN, pmKey, pmMod);

        return true;
    }

    bool onSpecial(const bool press, const DGL::Key key) override
    {
        if (pm == nullptr)
            return false;

        projectMKeycode  pmKey = PROJECTM_K_NONE;
        projectMModifier pmMod = PROJECTM_KMOD_LSHIFT;

        switch (key)
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

        if (const int mod = getModifiers())
        {
            if (mod & DGL::MODIFIER_CTRL)
                pmMod = PROJECTM_KMOD_LCTRL;
        }

        pm->key_handler(press ? PROJECTM_KEYUP : PROJECTM_KEYDOWN, pmKey, pmMod);

        return true;
    }

    void onReshape(const int width, const int height) override
    {
        /* Our shading model--Gouraud (smooth). */
        glShadeModel(GL_SMOOTH);

        /* Set the clear color. */
        glClearColor(0, 0, 0, 0);
        /* Setup our viewport. */
        glViewport(0, 0, width, height);

        /*
        * Change to the projection matrix and set
        * our viewing volume.
        */
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();

        //gluOrtho2D(0.0, (GLfloat) width, 0.0, (GLfloat) height);
        //glOrtho(0, width, height, 0, 0.0f, 1.0f);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glDrawBuffer(GL_BACK);
        glReadBuffer(GL_BACK);
        glEnable(GL_BLEND);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_POINT_SMOOTH);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glLineStipple(2, 0xAAAA);

        if (pm == nullptr)
            pm = new projectM(kSettings); // std::string("/usr/share/projectM/config.inp"));

        pm->projectM_resetGL(width, height);
    }

private:
    projectM* pm;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectMWidget)
};

// -----------------------------------------------------------------------

class ProjectMUI : public DGL::StandaloneWindow
{
public:
    ProjectMUI(const NativeHostDescriptor* const host)
        : DGL::StandaloneWindow(),
          fWidget(getWindow())
    {
        fWindow.setSize(kSettings.windowWidth, kSettings.windowHeight);
        fWindow.setTitle(host->uiName);

        if (host->uiParentId != 0)
            fWindow.setTransientWinId(host->uiParentId);

        fWindow.show();
    }

    projectM* getPM() noexcept
    {
        return fWidget.getPM();
    }

    void idle()
    {
        fApp.idle();
        fWindow.repaint();
    }

    void focus()
    {
        fWindow.focus();
    }

private:
    ProjectMWidget fWidget;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectMUI)
};

// -----------------------------------------------------------------------

class ProjectMPlugin : public NativePluginClass
{
public:
    ProjectMPlugin(const NativeHostDescriptor* const host)
        : NativePluginClass(host),
          fPM(nullptr),
          fUI(nullptr)
    {
    }

    ~ProjectMPlugin() override
    {
        CARLA_SAFE_ASSERT(fUI == nullptr);
    }

protected:
    // -------------------------------------------------------------------
    // Plugin process calls

    void process(float** ins, float**, const uint32_t frames, const NativeMidiEvent* const, const uint32_t) override
    {
        const CarlaMutexLocker csm(fMutex);

        if (fPM == nullptr)
            return;

        fPM->pcm()->addPCMfloat(ins[0] ,frames);
    }

    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        if (show)
        {
            if (fUI == nullptr)
                fUI = new ProjectMUI(getHostHandle());

            fUI->focus();
        }
        else if (fUI != nullptr)
        {
            {
                const CarlaMutexLocker csm(fMutex);
                fPM = nullptr;
            }

            delete fUI;
            fUI = nullptr;
        }
    }

    void uiIdle() override
    {
        if (fUI == nullptr)
            return;

        fUI->idle();

        if (fPM != nullptr)
            return;

        const CarlaMutexLocker csm(fMutex);
        fPM = fUI->getPM();
        CARLA_SAFE_ASSERT(fPM != nullptr);
    }

private:
    projectM*   fPM;
    ProjectMUI* fUI;

    // need to ensure pm is valid
    CarlaMutex fMutex;

    PluginClassEND(ProjectMPlugin)
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectMPlugin)
};

// -----------------------------------------------------------------------

static const NativePluginDescriptor projectmDesc = {
    /* category  */ PLUGIN_CATEGORY_UTILITY,
    /* hints     */ static_cast<NativePluginHints>(/*PLUGIN_IS_RTSAFE|*/PLUGIN_HAS_UI|PLUGIN_USES_PARENT_ID),
    /* supports  */ static_cast<NativePluginSupports>(0x0),
    /* audioIns  */ 1,
    /* audioOuts */ 0,
    /* midiIns   */ 0,
    /* midiOuts  */ 0,
    /* paramIns  */ 0,
    /* paramOuts */ 0,
    /* name      */ "ProjectM",
    /* label     */ "projectm",
    /* maker     */ "falkTX",
    /* copyright */ "GNU GPL v2+",
    PluginDescriptorFILL(ProjectMPlugin)
};

// -----------------------------------------------------------------------

CARLA_EXPORT
void carla_register_native_plugin_projectm()
{
    carla_register_native_plugin(&projectmDesc);
}

// -----------------------------------------------------------------------
