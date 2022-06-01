/*
 * Carla JSFX utils
 * Copyright (C) 2021 Jean Pierre Cimalando
 * Copyright (C) 2021-2022 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_JSFX_UTILS_HPP_INCLUDED
#define CARLA_JSFX_UTILS_HPP_INCLUDED

#include "CarlaDefines.h"
#include "CarlaBackend.h"
#include "CarlaUtils.hpp"
#include "CarlaJuceUtils.hpp"

#include "water/files/File.h"
#include "water/text/String.h"

#ifdef YSFX_API
# error YSFX_API is not private
#endif
#include "ysfx/include/ysfx.h"

#include <memory>

CARLA_BACKEND_START_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

struct CarlaJsfxLogging
{
    static void logAll(intptr_t, const ysfx_log_level level, const char* const message)
    {
        switch (level)
        {
        case ysfx_log_info:
            carla_stdout("%s: %s", ysfx_log_level_string(level), message);
            break;
        case ysfx_log_warning:
            carla_stderr("%s: %s", ysfx_log_level_string(level), message);
            break;
        case ysfx_log_error:
            carla_stderr2("%s: %s", ysfx_log_level_string(level), message);
            break;
        }
    };

    static void logErrorsOnly(intptr_t, const ysfx_log_level level, const char* const message)
    {
        switch (level)
        {
        case ysfx_log_info:
            break;
        case ysfx_log_warning:
            carla_stderr("%s: %s", ysfx_log_level_string(level), message);
            break;
        case ysfx_log_error:
            carla_stderr2("%s: %s", ysfx_log_level_string(level), message);
            break;
        }
    };
};

// --------------------------------------------------------------------------------------------------------------------

struct CarlaJsfxCategories
{
    static PluginCategory getFromEffect(ysfx_t* effect)
    {
        PluginCategory category = PLUGIN_CATEGORY_OTHER;

        if (const uint32_t tagCount = ysfx_get_tags(effect, nullptr, 0))
        {
            std::vector<const char*> tags;
            tags.resize(tagCount);
            ysfx_get_tags(effect, tags.data(), tagCount);

            for (uint32_t i=0; i<tagCount && category == PLUGIN_CATEGORY_OTHER; ++i)
            {
                water::CharPointer_UTF8 tag(tags[i]);
                PluginCategory current = getFromTag(tag);
                if (current != PLUGIN_CATEGORY_NONE)
                    category = current;
            }
        }

        return category;
    }

    static PluginCategory getFromTag(const water::CharPointer_UTF8 tag)
    {
        if (tag.compareIgnoreCase(water::CharPointer_UTF8("synthesis")) == 0)
            return PLUGIN_CATEGORY_SYNTH;

        if (tag.compareIgnoreCase(water::CharPointer_UTF8("delay")) == 0)
            return PLUGIN_CATEGORY_DELAY;

        if (tag.compareIgnoreCase(water::CharPointer_UTF8("equalizer")) == 0)
            return PLUGIN_CATEGORY_EQ;

        if (tag.compareIgnoreCase(water::CharPointer_UTF8("filter")) == 0)
            return PLUGIN_CATEGORY_FILTER;

        if (tag.compareIgnoreCase(water::CharPointer_UTF8("distortion")) == 0)
            return PLUGIN_CATEGORY_DISTORTION;

        if (tag.compareIgnoreCase(water::CharPointer_UTF8("dynamics")) == 0)
            return PLUGIN_CATEGORY_DYNAMICS;

        if (tag.compareIgnoreCase(water::CharPointer_UTF8("modulation")) == 0)
            return PLUGIN_CATEGORY_MODULATOR;

        if (tag.compareIgnoreCase(water::CharPointer_UTF8("utility")) == 0)
            return PLUGIN_CATEGORY_UTILITY;

        return PLUGIN_CATEGORY_NONE;
    }
};

// --------------------------------------------------------------------------------------------------------------------

class CarlaJsfxUnit
{
    static water::String createFileId(const water::File& rootPath, const water::File& filePath)
    {
        water::String fileId(filePath.getRelativePathFrom(rootPath));
#ifdef CARLA_OS_WIN
        fileId.replaceCharacter('\\', '/');
#endif
        return fileId;
    }

public:
    CarlaJsfxUnit() noexcept
        : fFileId(),
          fFilePath(),
          fRootPath() {}

    CarlaJsfxUnit(const water::File& rootPath, const water::File& filePath)
        : fFileId(createFileId(rootPath, filePath)),
          fFilePath(rootPath.getChildFile(fFileId).getFullPathName()),
          fRootPath(rootPath.getFullPathName()) {}

    explicit operator bool() const noexcept
    {
        return fFileId.isNotEmpty();
    }

    const water::String& getFileId() const noexcept
    {
        return fFileId;
    }

    const water::String& getFilePath() const noexcept
    {
        return fFilePath;
    }

    const water::String& getRootPath() const noexcept
    {
        return fRootPath;
    }

private:
    water::String fFileId;
    water::String fFilePath;
    water::String fRootPath;
};

// --------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_JSFX_UTILS_HPP_INCLUDED
