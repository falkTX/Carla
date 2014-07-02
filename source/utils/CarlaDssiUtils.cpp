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

#ifdef HAVE_JUCE
# include "juce_core.h"
#else
# include <QtCore/QDir>
# include <QtCore/QFileInfo>
# include <QtCore/QStringList>
#endif

// -----------------------------------------------------------------------

const char* find_dssi_ui(const char* const filename, const char* const label) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', nullptr);
    CARLA_SAFE_ASSERT_RETURN(label    != nullptr && label[0]    != '\0', nullptr);
    carla_debug("find_dssi_ui(\"%s\", \"%s\")", filename, label);

    try {
#ifdef HAVE_JUCE
        using namespace juce;

        String guiFilename;
        String pluginDir(String(filename).upToLastOccurrenceOf(".", false, false));

        String checkLabel(label);
        String checkSName(File(pluginDir).getFileName());

        if (! checkLabel.endsWith("_")) checkLabel += "_";
        if (! checkSName.endsWith("_")) checkSName += "_";

        Array<File> results;

        for (int i=0, count=File(pluginDir).findChildFiles(results, File::findFiles|File::ignoreHiddenFiles, false); i < count; ++i)
        {
            const File& gui(results[i]);
            const String& guiShortName(gui.getFileName());

            if (guiShortName.startsWith(checkLabel) || guiShortName.startsWith(checkSName))
            {
                guiFilename = gui.getFullPathName();
                break;
            }
        }

        if (guiFilename.isEmpty())
            return nullptr;

        return carla_strdup(guiFilename.toRawUTF8());
#else
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
#endif
    } CARLA_SAFE_EXCEPTION_RETURN("find_dssi_ui", nullptr);
}

// -----------------------------------------------------------------------
