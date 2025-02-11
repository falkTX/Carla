// SPDX-FileCopyrightText: 2011-2024 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CarlaDssiUtils.hpp"

#include "water/files/File.h"

// --------------------------------------------------------------------------------------------------------------------

const char* find_dssi_ui(const char* const filename, const char* const label) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', nullptr);
    CARLA_SAFE_ASSERT_RETURN(label    != nullptr && label[0]    != '\0', nullptr);
    carla_debug("find_dssi_ui(\"%s\", \"%s\")", filename, label);

    try {
        water::String guiFilename;
        water::String pluginDir(water::String(filename).upToLastOccurrenceOf(".", false, false));

        water::String checkLabel(label);
        water::String checkSName(water::File(pluginDir.toRawUTF8()).getFileName());

        if (checkSName.endsWithIgnoreCase("dssi"))
        {
            checkSName = checkSName.dropLastCharacters(4);

            if (checkSName.endsWithChar('-'))
                checkSName = checkSName.dropLastCharacters(1);
        }

        if (! checkLabel.endsWithChar('_')) checkLabel += "_";
        if (! checkSName.endsWithChar('_')) checkSName += "_";

        std::vector<water::File> results;

        if (const uint count = water::File(pluginDir.toRawUTF8()).findChildFiles(
                results, water::File::findFiles|water::File::ignoreHiddenFiles, false))
        {
            for (uint i=0; i<count; ++i)
            {
                const water::File& gui(results[i]);
                const water::String& guiShortName(gui.getFileName());

                if (guiShortName.startsWith(checkLabel) || guiShortName.startsWith(checkSName))
                {
                    guiFilename = gui.getFullPathName();
                    break;
                }
            }
        }

        if (guiFilename.isEmpty())
            return nullptr;

        return carla_strdup(guiFilename.toRawUTF8());

    } CARLA_SAFE_EXCEPTION_RETURN("find_dssi_ui", nullptr);
}

// --------------------------------------------------------------------------------------------------------------------
