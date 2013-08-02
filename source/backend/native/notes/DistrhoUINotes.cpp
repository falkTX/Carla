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
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "DistrhoUINotes.hpp"

#include <QtGui/QResizeEvent>

START_NAMESPACE_DISTRHO

#include "moc_DistrhoUINotes.cpp"

// -----------------------------------------------------------------------

DistrhoUINotes::DistrhoUINotes()
    : QtUI(),
      fCurPage(1),
      fSaveSizeNowChecker(-1),
      fSaveTextNowChecker(-1),
      fTextEdit(this),
      fButton(this),
      fProgressBar(this),
      fSpacer(this),
      fGridLayout(this)
{
    fButton.setCheckable(true);
    fButton.setChecked(true);
    fButton.setText("Edit");
    fButton.setFixedSize(fButton.minimumSizeHint());

    fProgressBar.set_minimum(1);
    fProgressBar.set_maximum(100);
    fProgressBar.set_value(1);

    fSpacer.setText("");
    fSpacer.setFixedSize(5, 5);

    fTextEdit.setReadOnly(false);

    setLayout(&fGridLayout);
    fGridLayout.addWidget(&fTextEdit, 0, 0, 1, 3);
    fGridLayout.addWidget(&fButton, 1, 0, 1, 1);
    fGridLayout.addWidget(&fProgressBar, 1, 1, 1, 1);
    fGridLayout.addWidget(&fSpacer, 1, 2, 1, 1);
    fGridLayout.setContentsMargins(0, 0, 0, 0);

    connect(&fButton, SIGNAL(clicked(bool)), SLOT(buttonClicked(bool)));
    connect(&fProgressBar, SIGNAL(valueChangedFromBar(float)), SLOT(progressBarValueChanged(float)));
    connect(&fTextEdit, SIGNAL(textChanged()), SLOT(textChanged()));

    setSize(300, 200);
}

DistrhoUINotes::~DistrhoUINotes()
{
}

void DistrhoUINotes::saveCurrentTextState()
{
    QString pageKey   = QString("pageText #%1").arg(fCurPage);
    QString pageValue = fTextEdit.toPlainText();

    if (pageValue != fNotes[fCurPage-1])
    {
        fNotes[fCurPage-1] = pageValue;
        d_setState(pageKey.toUtf8().constData(), pageValue.toUtf8().constData());
    }
}

// -----------------------------------------------------------------------
// DSP Callbacks

void DistrhoUINotes::d_parameterChanged(uint32_t index, float value)
{
    if (index != 0)
        return;

    int nextCurPage = value;

    if (nextCurPage != fCurPage && nextCurPage >= 1 && nextCurPage <= 100)
    {
        saveCurrentTextState();
        fCurPage = nextCurPage;

        fTextEdit.setPlainText(fNotes[fCurPage-1]);
        fProgressBar.set_value(fCurPage);
        fProgressBar.update();
    }
}

void DistrhoUINotes::d_stateChanged(const char* key, const char* value)
{
    if (std::strcmp(key, "guiWidth") == 0)
    {
        bool ok;
        int width = QString(value).toInt(&ok);

        if (ok && width > 0)
            setSize(width, height());
    }

    else if (std::strcmp(key, "guiHeight") == 0)
    {
        bool ok;
        int height = QString(value).toInt(&ok);

        if (ok && height > 0)
            setSize(width(), height);
    }

    else if (std::strncmp(key, "pageText #", 10) == 0)
    {
        bool ok;
        int pageIndex = QString(key+10).toInt(&ok);

        if (ok && pageIndex >= 1 && pageIndex <= 100)
        {
            fNotes[pageIndex-1] = QString(value);

            if (pageIndex == fCurPage)
                fTextEdit.setPlainText(fNotes[pageIndex-1]);
        }
    }

    else if (strcmp(key, "readOnly") == 0)
    {
        bool readOnly = !strcmp(value, "yes");
        fButton.setChecked(!readOnly);
        fTextEdit.setReadOnly(readOnly);
    }
}

// -----------------------------------------------------------------------
// UI Callbacks

void DistrhoUINotes::d_uiIdle()
{
    if (fSaveSizeNowChecker == 11)
    {
        d_setState("guiWidth", QString::number(width()).toUtf8().constData());
        d_setState("guiHeight", QString::number(height()).toUtf8().constData());
        fSaveSizeNowChecker = -1;
    }

    if (fSaveTextNowChecker == 11)
    {
        saveCurrentTextState();
        fSaveTextNowChecker = -1;
    }

    if (fSaveSizeNowChecker >= 0)
        fSaveSizeNowChecker++;

    if (fSaveTextNowChecker >= 0)
        fSaveTextNowChecker++;
}

void DistrhoUINotes::resizeEvent(QResizeEvent* event)
{
    fSaveSizeNowChecker = 0;
    QtUI::resizeEvent(event);
}

// -----------------------------------------------------------------------

void DistrhoUINotes::buttonClicked(bool click)
{
    bool readOnly = !click;
    fTextEdit.setReadOnly(readOnly);

    d_setState("readOnly", readOnly ? "yes" : "no");
}

void DistrhoUINotes::progressBarValueChanged(float value)
{
    value = rint(value);

    if (fCurPage == (int)value)
        return;

    // maybe save current text before changing page
    if (fSaveTextNowChecker >= 0 && value >= 1.0f && value <= 100.0f)
    {
        saveCurrentTextState();
        fSaveTextNowChecker = -1;
    }

    // change current page
    d_parameterChanged(0, value);

    // tell host about this change
    d_setParameterValue(0, value);
}

void DistrhoUINotes::textChanged()
{
    fSaveTextNowChecker = 0;
}

// -----------------------------------------------------------------------

UI* createUI()
{
    return new DistrhoUINotes();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
