/*
 * Carla DSSI utils
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaDssiUtils.hpp"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStringList>

// -----------------------------------------------------------------------

const char* find_dssi_ui(const char* const filename, const char* const label)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', nullptr);
    CARLA_SAFE_ASSERT_RETURN(label    != nullptr && label[0]    != '\0', nullptr);
    carla_debug("find_dssi_ui(\"%s\", \"%s\")", filename, label);

    QString guiFilename;
    QString pluginDir(filename);
    pluginDir.resize(pluginDir.lastIndexOf("."));

    QString checkLabel(label);
    QString checkSName(QFileInfo(pluginDir).baseName());

    if (! checkLabel.endsWith("_")) checkLabel += "_";
    if (! checkSName.endsWith("_")) checkSName += "_";

    QStringList guiFiles(QDir(pluginDir).entryList());

    foreach (const QString& gui, guiFiles)
    {
        if (gui.startsWith(checkLabel) || gui.startsWith(checkSName))
        {
            QFileInfo finalname(pluginDir + QDir::separator() + gui);
            guiFilename = finalname.absoluteFilePath();
            break;
        }
    }

    if (guiFilename.isEmpty())
        return nullptr;

    return carla_strdup(guiFilename.toUtf8().constData());
}

// -----------------------------------------------------------------------
