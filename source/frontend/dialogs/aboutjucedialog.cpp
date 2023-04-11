/*
 * Carla plugin host
 * Copyright (C) 2011-2023 Filipe Coelho <falktx@falktx.com>
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

#include "aboutjucedialog.hpp"

#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated-copy-with-user-provided-copy"
# pragma clang diagnostic ignored "-Wdeprecated-register"
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wclass-memaccess"
# pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif

#include "ui_aboutjucedialog.h"

#ifdef __clang__
# pragma clang diagnostic pop
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic pop
#endif

#include "CarlaFrontend.h"
#include "CarlaUtils.h"

// --------------------------------------------------------------------------------------------------------------------
// Jack Application Dialog

struct AboutJuceDialog::Self {
    Ui_AboutJuceDialog ui;

    Self() {}

    static Self& create()
    {
        Self* const self = new Self();
        return *self;
    }
};

AboutJuceDialog::AboutJuceDialog(QWidget* const parent)
    : QDialog(parent),
      self(Self::create())
{
    self.ui.setupUi(this);

    // -------------------------------------------------------------------------------------------------------------
    // UI setup

    self.ui.l_text2->setText(tr("This program uses JUCE version") + " " + carla_get_juce_version() + ".");

    adjustSize();
    setFixedSize(size());

    Qt::WindowFlags flags = windowFlags();
    flags &= ~Qt::WindowContextHelpButtonHint;

   #ifdef CARLA_OS_WIN
    flags |= Qt::MSWindowsFixedSizeDialogHint;
   #endif

    setWindowFlags(flags);

   #ifdef CARLA_OS_MAC
    if (parent != nullptr)
        setWindowModality(Qt::WindowModal);
   #endif
}

AboutJuceDialog::~AboutJuceDialog()
{
    delete &self;
}

// --------------------------------------------------------------------------------------------------------------------

void carla_frontend_createAndExecAboutJuceDialog(void* const parent)
{
    AboutJuceDialog(reinterpret_cast<QWidget*>(parent)).exec();
}

// --------------------------------------------------------------------------------------------------------------------
