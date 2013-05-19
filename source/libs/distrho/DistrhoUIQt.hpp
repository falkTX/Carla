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

#ifndef __DISTRHO_UI_QT_HPP__
#define __DISTRHO_UI_QT_HPP__

#include "DistrhoUI.hpp"

#include <QtCore/Qt>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include <QtWidgets/QWidget>
#else
# include <QtGui/QWidget>
#endif

START_NAMESPACE_DISTRHO

// -------------------------------------------------
// Qt UI

class QtUI : public UI,
             public QWidget
{
public:
    QtUI();
    virtual ~QtUI() override;

    // ---------------------------------------------
    // UI Helpers

    void setSize(unsigned int width, unsigned int height);

protected:
    // ---------------------------------------------
    // Information

    virtual bool d_resizable() { return false; }
    virtual uint d_minimumWidth() { return 100; }
    virtual uint d_minimumHeight() { return 100; }

    // ---------------------------------------------
    // DSP Callbacks

    virtual void d_parameterChanged(uint32_t index, float value) override = 0;
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    virtual void d_programChanged(uint32_t index) override = 0;
#endif
#if DISTRHO_PLUGIN_WANT_STATE
    virtual void d_stateChanged(const char* key, const char* value) override = 0;
#endif
#if DISTRHO_PLUGIN_IS_SYNTH
    virtual void d_noteReceived(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity) override = 0;
#endif

    // ---------------------------------------------
    // UI Callbacks

    virtual void d_uiIdle() override {}

private:
    friend class UIInternal;

    unsigned int d_width() const override { return width(); }
    unsigned int d_height() const override { return height(); }
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __DISTRHO_UI_QT_HPP__
