// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "carla_frontend.h"

#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated-copy-with-user-provided-copy"
# pragma clang diagnostic ignored "-Wdeprecated-register"
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wclass-memaccess"
# pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif

#include "ui_aboutdialog.h"

#ifdef __clang__
# pragma clang diagnostic pop
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic pop
#endif

// --------------------------------------------------------------------------------------------------------------------
// About Dialog

class AboutDialog : public QDialog
{
    Ui_AboutDialog ui;

    // ----------------------------------------------------------------------------------------------------------------

public:
    explicit AboutDialog(QWidget* parent, CarlaHostHandle hostHandle, bool isControl, bool isPlugin);
};

// --------------------------------------------------------------------------------------------------------------------
