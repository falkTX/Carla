// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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

#include <QtCore/QFileInfo>
#include <QtWidgets/QPushButton>

#ifdef __clang__
# pragma clang diagnostic pop
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic pop
#endif

#include "qsafesettings.hpp"

#include "CarlaLibJackHints.h"

// --------------------------------------------------------------------------------------------------------------------
// Jack Application Dialog

struct JackAppDialog::PrivateData {
    const QString fProjectFilename;

    PrivateData(const char* const projectFilename)
        : fProjectFilename(projectFilename) {}
};

JackAppDialog::JackAppDialog(QWidget* const parent, const char* const projectFilename)
    : QDialog(parent),
      p(new PrivateData(projectFilename))
{
    ui.setupUi(this);

    // ----------------------------------------------------------------------------------------------------------------
    // UI setup

    ui.group_error->setVisible(false);

    adjustSize();
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

   #ifdef CARLA_OS_MAC
    if (parent != nullptr)
        setWindowModality(Qt::WindowModal);
   #endif

    // ----------------------------------------------------------------------------------------------------------------
    // Load settings

    loadSettings();

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up connections

    connect(this, &QDialog::finished,
            this, &JackAppDialog::slot_saveSettings);
    connect(ui.cb_session_mgr, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &JackAppDialog::slot_sessionManagerChanged);
    connect(ui.le_command, &QLineEdit::textChanged,
            this, &JackAppDialog::slot_commandChanged);
}

JackAppDialog::~JackAppDialog()
{
    delete p;
}

// --------------------------------------------------------------------------------------------------------------------
// public methods

JackAppDialog::CommandAndFlags JackAppDialog::getCommandAndFlags() const
{
    const QString command = ui.le_command->text();
    QString name          = ui.le_name->text();

    if (name.isEmpty())
    {
        name = QFileInfo(command.split(' ').first()).baseName();
        name[0] = name[0].toTitleCase();
    }

    SessionManager smgr;
    switch (ui.cb_session_mgr->currentIndex())
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
    if (ui.cb_manage_window->isChecked())
        flags |= LIBJACK_FLAG_CONTROL_WINDOW;
    if (ui.cb_capture_first_window->isChecked())
        flags |= LIBJACK_FLAG_CAPTURE_FIRST_WINDOW;
    if (ui.cb_buffers_addition_mode->isChecked())
        flags |= LIBJACK_FLAG_AUDIO_BUFFERS_ADDITION;
    if (ui.cb_out_midi_mixdown->isChecked())
        flags |= LIBJACK_FLAG_MIDI_OUTPUT_CHANNEL_MIXDOWN;
    if (ui.cb_external_start->isChecked())
        flags |= LIBJACK_FLAG_EXTERNAL_START;

    const QString labelSetup(QString("%1%2%3%4%5%6").arg(QChar('0' + ui.sb_audio_ins->value()))
                                                        .arg(QChar('0' + ui.sb_audio_outs->value()))
                                                        .arg(QChar('0' + ui.sb_midi_ins->value()))
                                                        .arg(QChar('0' + ui.sb_midi_outs->value()))
                                                        .arg(QChar('0' + smgr))
                                                        .arg(QChar('0' + flags)));

    return {command, name, labelSetup};
}

// --------------------------------------------------------------------------------------------------------------------
// private methods

void JackAppDialog::checkIfButtonBoxShouldBeEnabled(const int index, const QCarlaString& command)
{
    bool enabled = command.isNotEmpty();
    QCarlaString showErr;

    // NSM applications must not be abstract or absolute paths, and must not contain arguments
    if (enabled && index == UI_SESSION_NSM)
    {
        if (command[0] == '.' || command[0] == '/')
            showErr = tr("NSM applications cannot use abstract or absolute paths");
        else if (command.contains(' ') or command.contains(';') or command.contains('&'))
            showErr = tr("NSM applications cannot use CLI arguments");
        else if (p->fProjectFilename.isEmpty())
            showErr = tr("You need to save the current Carla project before NSM can be used");
    }

    if (showErr.isNotEmpty())
    {
        enabled = false;
        ui.l_error->setText(showErr);
        ui.group_error->setVisible(true);
    }
    else
    {
        ui.group_error->setVisible(false);
    }

    if (QPushButton* const button = ui.buttonBox->button(QDialogButtonBox::Ok))
        button->setEnabled(enabled);
}

void JackAppDialog::loadSettings()
{
    const QSafeSettings settings("falkTX", "CarlaAddJackApp");

    const QString smName = settings.valueString("SessionManager", "");

    if (smName == "LADISH (SIGUSR1)")
        ui.cb_session_mgr->setCurrentIndex(UI_SESSION_LADISH);
    else if (smName == "NSM")
        ui.cb_session_mgr->setCurrentIndex(UI_SESSION_NSM);
    else
        ui.cb_session_mgr->setCurrentIndex(UI_SESSION_NONE);

    ui.le_command->setText(settings.valueString("Command", ""));
    ui.le_name->setText(settings.valueString("Name", ""));
    ui.sb_audio_ins->setValue(settings.valueIntPositive("NumAudioIns", 2));
    ui.sb_audio_ins->setValue(settings.valueIntPositive("NumAudioIns", 2));
    ui.sb_audio_outs->setValue(settings.valueIntPositive("NumAudioOuts", 2));
    ui.sb_midi_ins->setValue(settings.valueIntPositive("NumMidiIns", 0));
    ui.sb_midi_outs->setValue(settings.valueIntPositive("NumMidiOuts", 0));
    ui.cb_manage_window->setChecked(settings.valueBool("ManageWindow", true));
    ui.cb_capture_first_window->setChecked(settings.valueBool("CaptureFirstWindow", false));
    ui.cb_out_midi_mixdown->setChecked(settings.valueBool("MidiOutMixdown", false));

    checkIfButtonBoxShouldBeEnabled(ui.cb_session_mgr->currentIndex(),
                                    ui.le_command->text());
}

// --------------------------------------------------------------------------------------------------------------------
// private slots

void JackAppDialog::slot_commandChanged(const QString& command)
{
    checkIfButtonBoxShouldBeEnabled(ui.cb_session_mgr->currentIndex(), command);
}

void JackAppDialog::slot_sessionManagerChanged(const int index)
{
    checkIfButtonBoxShouldBeEnabled(index, ui.le_command->text());
}

void JackAppDialog::slot_saveSettings()
{
    QSafeSettings settings("falkTX", "CarlaAddJackApp");
    settings.setValue("Command", ui.le_command->text());
    settings.setValue("Name", ui.le_name->text());
    settings.setValue("SessionManager", ui.cb_session_mgr->currentText());
    settings.setValue("NumAudioIns", ui.sb_audio_ins->value());
    settings.setValue("NumAudioOuts", ui.sb_audio_outs->value());
    settings.setValue("NumMidiIns", ui.sb_midi_ins->value());
    settings.setValue("NumMidiOuts", ui.sb_midi_outs->value());
    settings.setValue("ManageWindow", ui.cb_manage_window->isChecked());
    settings.setValue("CaptureFirstWindow", ui.cb_capture_first_window->isChecked());
    settings.setValue("MidiOutMixdown", ui.cb_out_midi_mixdown->isChecked());
}

// --------------------------------------------------------------------------------------------------------------------
