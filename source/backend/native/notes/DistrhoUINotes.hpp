/*
 * DISTRHO Notes Plugin
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

#ifndef __DISTRHO_UI_NOTES_HPP__
#define __DISTRHO_UI_NOTES_HPP__

#include "DistrhoUIQt4.hpp"
#include "paramspinbox.hpp"

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# include <QtWidgets/QProgressBar>
# include <QtWidgets/QGridLayout>
# include <QtWidgets/QLabel>
# include <QtWidgets/QPushButton>
# include <QtWidgets/QTextEdit>
#else
# include <QtGui/QGridLayout>
# include <QtGui/QLabel>
# include <QtGui/QPushButton>
# include <QtGui/QTextEdit>
#endif

class QResizeEvent;

START_NAMESPACE_DISTRHO

// -------------------------------------------------

class DistrhoUINotes : public Qt4UI
{
    Q_OBJECT

public:
    DistrhoUINotes();
    ~DistrhoUINotes();

protected:
    // ---------------------------------------------
    // Information

    bool d_resizable()
    {
        return true;
    }

    uint d_minimumWidth()
    {
        return 180;
    }

    uint d_minimumHeight()
    {
        return 150;
    }

    // ---------------------------------------------
    // DSP Callbacks

    void d_parameterChanged(uint32_t index, float value);
    void d_stateChanged(const char* key, const char* value);

    // ---------------------------------------------
    // UI Callbacks

    void d_uiIdle();

    // ---------------------------------------------
    // listen for resize events

    void resizeEvent(QResizeEvent*);

private slots:
    void buttonClicked(bool click);
    void progressBarValueChanged(float value);
    void textChanged();

private:
    int fCurPage;
    int fSaveSizeNowChecker;
    int fSaveTextNowChecker;

    QString fNotes[100];

    QTextEdit        fTextEdit;
    QPushButton      fButton;
    ParamProgressBar fProgressBar;
    QLabel           fSpacer;
    QGridLayout      fGridLayout;

    void saveCurrentTextState();
};

// -------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // __DISTRHO_UI_NOTES_HPP__
