// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "carla_frontend.h"

// -------------------------------------------------------------------------------------------------------------------
// common files

#include "utils/qsafesettings.cpp"

// --------------------------------------------------------------------------------------------------------------------
// aboutdialog

#include "dialogs/aboutdialog.hpp"

void carla_frontend_createAndExecAboutDialog(QWidget* const parent,
                                             const CarlaHostHandle hostHandle,
                                             const bool isControl,
                                             const bool isPlugin)
{
    AboutDialog(parent, hostHandle, isControl, isPlugin).exec();
}

// --------------------------------------------------------------------------------------------------------------------
// jackappdialog

#include "dialogs/jackappdialog.hpp"
#include "extra/String.hpp"

const JackAppDialogResults*
carla_frontend_createAndExecJackAppDialog(QWidget* const parent, const char* const projectFilename)
{
    JackAppDialog gui(parent, projectFilename);

    if (gui.exec())
    {
        static JackAppDialogResults ret = {};
        static String retCommand;
        static String retName;
        static String retLabelSetup;

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

// --------------------------------------------------------------------------------------------------------------------
// pluginlistdialog

#include "pluginlist/pluginlistdialog.hpp"
#include "CarlaUtils.h"

PluginListDialog*
carla_frontend_createPluginListDialog(QWidget* const parent, const HostSettings* const hostSettings)
{
    return new PluginListDialog(parent, hostSettings);
}

void
carla_frontend_destroyPluginListDialog(PluginListDialog* const dialog)
{
    dialog->close();
    delete dialog;
}

void
carla_frontend_setPluginListDialogPath(PluginListDialog* const dialog, const int ptype, const char* const path)
{
    dialog->setPluginPath(static_cast<PluginType>(ptype), path);
}

const PluginListDialogResults*
carla_frontend_execPluginListDialog(PluginListDialog* const dialog)
{
    if (dialog->exec())
    {
        static PluginListDialogResults ret;
        static String category;
        static String filename;
        static String name;
        static String label;
        static String maker;

        const PluginInfo& plugin(dialog->getSelectedPluginInfo());

        category = plugin.category.toUtf8();
        filename = plugin.filename.toUtf8();
        name = plugin.name.toUtf8();
        label = plugin.label.toUtf8();
        maker = plugin.maker.toUtf8();

        ret.build = plugin.build;
        ret.type = plugin.type;
        ret.hints = plugin.hints;
        ret.category = category;
        ret.filename = filename;
        ret.name = name;
        ret.label = label;
        ret.maker = maker;
        ret.uniqueId = plugin.uniqueId;
        ret.audioIns = plugin.audioIns;
        ret.audioOuts = plugin.audioOuts;
        ret.cvIns = plugin.cvIns;
        ret.cvOuts = plugin.cvOuts;
        ret.midiIns = plugin.midiIns;
        ret.midiOuts = plugin.midiOuts;
        ret.parameterIns = plugin.parameterIns;
        ret.parameterOuts = plugin.parameterOuts;

        return &ret;
    }

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------

// const PluginListDialogResults*
// carla_frontend_createAndExecPluginListDialog(void* const parent, const HostSettings* const hostSettings)
// {
//     PluginListDialog gui(reinterpret_cast<QWidget*>(parent), hostSettings);
//
//     return carla_frontend_execPluginListDialog(&gui);
// }

// --------------------------------------------------------------------------------------------------------------------
