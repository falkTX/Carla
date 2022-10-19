/*
 * Carla plugin host
 * Copyright (C) 2011-2022 Filipe Coelho <falktx@falktx.com>
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

#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated-copy-with-user-provided-copy"
# pragma clang diagnostic ignored "-Wdeprecated-register"
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wclass-memaccess"
# pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif

#include <QtWidgets/QDialog>

#ifdef __clang__
# pragma clang diagnostic pop
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic pop
#endif

#include "qcarlastring.hpp"

// --------------------------------------------------------------------------------------------------------------------
// Jack Application Dialog

class JackApplicationW : public QDialog
{
    struct Self;
    Self& self;

    // ----------------------------------------------------------------------------------------------------------------

public:
    explicit JackApplicationW(QWidget* parent, const char* projectFilename);
    ~JackApplicationW() override;

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

extern "C" {

struct JackApplicationDialogResults {
    const char* command;
    const char* name;
    const char* labelSetup;
};

CARLA_API JackApplicationDialogResults* carla_frontend_createAndExecJackApplicationW(void* parent, const char* projectFilename);

}

// --------------------------------------------------------------------------------------------------------------------
