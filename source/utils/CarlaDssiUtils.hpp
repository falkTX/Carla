/*
 * Carla DSSI utils
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_DSSI_UTILS_HPP_INCLUDED
#define CARLA_DSSI_UTILS_HPP_INCLUDED

#include "CarlaLadspaUtils.hpp"
#include "dssi/dssi.h"

#include "juce_core.h"

// -----------------------------------------------------------------------

static inline
const char* find_dssi_ui(const char* const filename, const char* const label)
{
    CARLA_SAFE_ASSERT_RETURN(filename != nullptr, nullptr);
    CARLA_SAFE_ASSERT_RETURN(label    != nullptr, nullptr);
    carla_debug("find_dssi_ui(\"%s\", \"%s\")", filename, label);

    using namespace juce;

    File pluginFile(filename);
    File pluginDir(pluginFile.getParentDirectory().getFullPathName() + File::separatorString + pluginFile.getFileNameWithoutExtension());

    Array<File> results;

    if (pluginDir.findChildFiles(results, File::findFiles|File::ignoreHiddenFiles, false) == 0)
        return nullptr;

    StringArray guiFiles;

    for (int i=0, count=results.size(); i < count; ++i)
    {
        const File& file(results[i]);
        guiFiles.add(file.getFileName());
    }

    String pluginDirName(pluginDir.getFullPathName());
    String pluginShortName(pluginFile.getFileNameWithoutExtension());

    String checkLabel(label);
    String checkShort(pluginShortName);

    if (! checkLabel.endsWith("_")) checkLabel += "_";
    if (! checkShort.endsWith("_")) checkShort += "_";

    for (int i=0, count=guiFiles.size(); i < count; ++i)
    {
        const String& gui(guiFiles[i]);

        if (gui.startsWith(checkLabel) || gui.startsWith(checkShort))
            return carla_strdup(File(pluginDir).getChildFile(gui).getFullPathName().toRawUTF8());
    }

    return nullptr;
}

// -----------------------------------------------------------------------

#endif // CARLA_DSSI_UTILS_HPP_INCLUDED
