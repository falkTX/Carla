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

#include "jackappdialog.hpp"

#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated-copy-with-user-provided-copy"
# pragma clang diagnostic ignored "-Wdeprecated-register"
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wclass-memaccess"
# pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif

#include "jackappdialog_ui.hpp"
#include <QtCore/QFileInfo>
#include <QtCore/QVector>
#include <QtWidgets/QPushButton>

#ifdef __clang__
# pragma clang diagnostic pop
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic pop
#endif

#include "qsafesettings.hpp"

#include "CarlaLibJackHints.h"
#include "CarlaString.hpp"

// --------------------------------------------------------------------------------------------------------------------
// Jack Application Dialog

enum {
    UI_SESSION_NONE   = 0,
    UI_SESSION_LADISH = 1,
    UI_SESSION_NSM    = 2,
};

struct JackAppDialog::Self {
    Ui_JackAppDialog ui;
    const QString fProjectFilename;

    Self(const char* const projectFilename)
        : fProjectFilename(projectFilename) {}

    static Self& create(const char* const projectFilename)
    {
        Self* const self = new Self(projectFilename);
        return *self;
    }
};

JackAppDialog::JackAppDialog(QWidget* const parent, const char* const projectFilename)
    : QDialog(parent),
      self(Self::create(projectFilename))
{
    self.ui.setupUi(this);

    // -------------------------------------------------------------------------------------------------------------
    // UI setup

    self.ui.group_error->setVisible(false);

    adjustSize();
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

   #ifdef CARLA_OS_MAC
    if (parent != nullptr)
        setWindowModality(Qt::WindowModal);
   #endif

    // -------------------------------------------------------------------------------------------------------------
    // Load settings

    loadSettings();

    // -------------------------------------------------------------------------------------------------------------
    // Set-up connections

    connect(this, &QDialog::finished, 
            this, &JackAppDialog::slot_saveSettings);
    connect(self.ui.cb_session_mgr, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &JackAppDialog::slot_sessionManagerChanged);
    connect(self.ui.le_command, &QLineEdit::textChanged,
            this, &JackAppDialog::slot_commandChanged);
}

JackAppDialog::~JackAppDialog()
{
    delete &self;
}

// -----------------------------------------------------------------------------------------------------------------
// public methods

JackAppDialog::CommandAndFlags JackAppDialog::getCommandAndFlags() const
{
    const QString command = self.ui.le_command->text();
    QString name          = self.ui.le_name->text();

    if (name.isEmpty())
    {
        name = QFileInfo(command.split(' ').first()).baseName();
        name[0] = name[0].toTitleCase();
    }

    SessionManager smgr;
    switch (self.ui.cb_session_mgr->currentIndex())
    {
    case UI_SESSION_LADISH:
        smgr = LIBJACK_SESSION_MANAGER_LADISH;
        break;
    case UI_SESSION_NSM:
        smgr = LIBJACK_SESSION_MANAGER_NSM;
        break;
    default:
        smgr = LIBJACK_SESSION_MANAGER_NONE;
        break;
    }

    uint flags = 0x0;
    if (self.ui.cb_manage_window->isChecked())
        flags |= LIBJACK_FLAG_CONTROL_WINDOW;
    if (self.ui.cb_capture_first_window->isChecked())
        flags |= LIBJACK_FLAG_CAPTURE_FIRST_WINDOW;
    if (self.ui.cb_buffers_addition_mode->isChecked())
        flags |= LIBJACK_FLAG_AUDIO_BUFFERS_ADDITION;
    if (self.ui.cb_out_midi_mixdown->isChecked())
        flags |= LIBJACK_FLAG_MIDI_OUTPUT_CHANNEL_MIXDOWN;
    if (self.ui.cb_external_start->isChecked())
        flags |= LIBJACK_FLAG_EXTERNAL_START;

    const QString labelSetup(QString("%1%2%3%4%5%6").arg(QChar('0' + self.ui.sb_audio_ins->value()))
                                                        .arg(QChar('0' + self.ui.sb_audio_outs->value()))
                                                        .arg(QChar('0' + self.ui.sb_midi_ins->value()))
                                                        .arg(QChar('0' + self.ui.sb_midi_outs->value()))
                                                        .arg(QChar('0' + smgr))
                                                        .arg(QChar('0' + flags)));

    return {command, name, labelSetup};
}

// -----------------------------------------------------------------------------------------------------------------
// private methods

void JackAppDialog::checkIfButtonBoxShouldBeEnabled(const int index, const QCarlaString& command)
{
    bool enabled = command.isNotEmpty();
    QCarlaString showErr;

    // NSM applications must not be abstract or absolute paths, and must not contain arguments
    if (enabled and index == UI_SESSION_NSM)
    {
        if (QVector<QChar>{'.', '/'}.contains(command[0]))
            showErr = tr("NSM applications cannot use abstract or absolute paths");
        else if (command.contains(' ') or command.contains(';') or command.contains('&'))
            showErr = tr("NSM applications cannot use CLI arguments");
        else if (self.fProjectFilename.isEmpty())
            showErr = tr("You need to save the current Carla project before NSM can be used");
    }

    if (showErr.isNotEmpty())
    {
        enabled = false;
        self.ui.l_error->setText(showErr);
        self.ui.group_error->setVisible(true);
    }
    else
    {
        self.ui.group_error->setVisible(false);
    }

    if (QPushButton* const button = self.ui.buttonBox->button(QDialogButtonBox::Ok))
        button->setEnabled(enabled);
}

void JackAppDialog::loadSettings()
{
    const QSafeSettings settings("falkTX", "CarlaAddJackApp");

    const QString smName = settings.valueString("SessionManager", "");

    if (smName == "LADISH (SIGUSR1)")
        self.ui.cb_session_mgr->setCurrentIndex(UI_SESSION_LADISH);
    else if (smName == "NSM")
        self.ui.cb_session_mgr->setCurrentIndex(UI_SESSION_NSM);
    else
        self.ui.cb_session_mgr->setCurrentIndex(UI_SESSION_NONE);

    self.ui.le_command->setText(settings.valueString("Command", ""));
    self.ui.le_name->setText(settings.valueString("Name", ""));
    self.ui.sb_audio_ins->setValue(settings.valueIntPositive("NumAudioIns", 2));
    self.ui.sb_audio_ins->setValue(settings.valueIntPositive("NumAudioIns", 2));
    self.ui.sb_audio_outs->setValue(settings.valueIntPositive("NumAudioOuts", 2));
    self.ui.sb_midi_ins->setValue(settings.valueIntPositive("NumMidiIns", 0));
    self.ui.sb_midi_outs->setValue(settings.valueIntPositive("NumMidiOuts", 0));
    self.ui.cb_manage_window->setChecked(settings.valueBool("ManageWindow", true));
    self.ui.cb_capture_first_window->setChecked(settings.valueBool("CaptureFirstWindow", false));
    self.ui.cb_out_midi_mixdown->setChecked(settings.valueBool("MidiOutMixdown", false));

    checkIfButtonBoxShouldBeEnabled(self.ui.cb_session_mgr->currentIndex(),
                                    self.ui.le_command->text());
}

// -----------------------------------------------------------------------------------------------------------------
// private slots

void JackAppDialog::slot_commandChanged(const QString& command)
{
    checkIfButtonBoxShouldBeEnabled(self.ui.cb_session_mgr->currentIndex(), command);
}

void JackAppDialog::slot_sessionManagerChanged(const int index)
{
    checkIfButtonBoxShouldBeEnabled(index, self.ui.le_command->text());
}

void JackAppDialog::slot_saveSettings()
{
    QSafeSettings settings("falkTX", "CarlaAddJackApp");
    settings.setValue("Command", self.ui.le_command->text());
    settings.setValue("Name", self.ui.le_name->text());
    settings.setValue("SessionManager", self.ui.cb_session_mgr->currentText());
    settings.setValue("NumAudioIns", self.ui.sb_audio_ins->value());
    settings.setValue("NumAudioOuts", self.ui.sb_audio_outs->value());
    settings.setValue("NumMidiIns", self.ui.sb_midi_ins->value());
    settings.setValue("NumMidiOuts", self.ui.sb_midi_outs->value());
    settings.setValue("ManageWindow", self.ui.cb_manage_window->isChecked());
    settings.setValue("CaptureFirstWindow", self.ui.cb_capture_first_window->isChecked());
    settings.setValue("MidiOutMixdown", self.ui.cb_out_midi_mixdown->isChecked());
}

// --------------------------------------------------------------------------------------------------------------------

JackAppDialogResults* carla_frontend_createAndExecJackAppDialog(void* const parent, const char* const projectFilename)
{
    JackAppDialog gui(reinterpret_cast<QWidget*>(parent), projectFilename);

    if (gui.exec())
    {
        static JackAppDialogResults ret = {};
        static CarlaString retCommand;
        static CarlaString retName;
        static CarlaString retLabelSetup;

        const JackAppDialog::CommandAndFlags cafs = gui.getCommandAndFlags();
        retCommand = cafs.command.toUtf8().constData();
        retName = cafs.name.toUtf8().constData();
        retLabelSetup = cafs.labelSetup.toUtf8().constData();

        ret.command = retCommand;
        ret.name = retName;
        ret.labelSetup = retLabelSetup;

        return &ret;
    }

    return nullptr;
}

#if 0
// --------------------------------------------------------------------------------------------------------------------
// Testing

#include "../utils/qsafesettings.cpp"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    if (JackAppDialogResults* const res = carla_frontend_createAndExecJackAppDialog(nullptr, ""))
    {
        printf("Results:\n");
        printf("\tCommand:    %s\n", res->command);
        printf("\tName:       %s\n", res->name);
        printf("\tLabelSetup: %s\n", res->labelSetup);
    }

    return 0;
}
#endif

// --------------------------------------------------------------------------------------------------------------------
