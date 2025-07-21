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

#include "ui_jackappdialog.h"

#ifdef __clang__
# pragma clang diagnostic pop
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic pop
#endif

#include "qcarlastring.hpp"

// --------------------------------------------------------------------------------------------------------------------
// Jack Application Dialog

class JackAppDialog : public QDialog
{
    enum {
        UI_SESSION_NONE,
        UI_SESSION_LADISH,
        UI_SESSION_NSM,
    };

    struct PrivateData;
    PrivateData* const p;

    Ui_JackAppDialog ui;

    // ----------------------------------------------------------------------------------------------------------------

public:
    explicit JackAppDialog(QWidget* parent, const char* projectFilename);
    ~JackAppDialog() override;

    // ----------------------------------------------------------------------------------------------------------------
    // public methods

    struct CommandAndFlags {
        QString command;
        QString name;
        QString labelSetup;
    };
    CommandAndFlags getCommandAndFlags() const;

    // ----------------------------------------------------------------------------------------------------------------
    // private methods

private:
    void checkIfButtonBoxShouldBeEnabled(int index, const QCarlaString& text);
    void loadSettings();

    // ----------------------------------------------------------------------------------------------------------------
    // private slots

private slots:
    void slot_commandChanged(const QString& command);
    void slot_sessionManagerChanged(int);
    void slot_saveSettings();
};

// --------------------------------------------------------------------------------------------------------------------
