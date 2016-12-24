/*
 * Carla DSSI utils
 * Copyright (C) 2013-2016 Filipe Coelho <falktx@falktx.com>
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

#include "juce_core/juce_core.h"

// --------------------------------------------------------------------------------------------------------------------

const char* find_dssi_ui(const char* const filename, const char* const label) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', nullptr);
    CARLA_SAFE_ASSERT_RETURN(label    != nullptr && label[0]    != '\0', nullptr);
    carla_debug("find_dssi_ui(\"%s\", \"%s\")", filename, label);

    try {
        using namespace juce;

        String guiFilename;
        String pluginDir(String(filename).upToLastOccurrenceOf(".", false, false));

        String checkLabel(label);
        String checkSName(File(pluginDir).getFileName());

        if (! checkLabel.endsWith("_")) checkLabel += "_";
        if (! checkSName.endsWith("_")) checkSName += "_";

        Array<File> results;

        for (int i=File(pluginDir).findChildFiles(results, File::findFiles|File::ignoreHiddenFiles, false); --i >= 0;)
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

    } CARLA_SAFE_EXCEPTION_RETURN("find_dssi_ui", nullptr);
}

// --------------------------------------------------------------------------------------------------------------------
