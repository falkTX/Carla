// SPDX-FileCopyrightText: 2021 Jean Pierre Cimalando
// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CARLA_JSFX_UTILS_HPP_INCLUDED
#define CARLA_JSFX_UTILS_HPP_INCLUDED

#include "CarlaBackend.h"

#include "extra/String.hpp"
#include "water/files/File.h"

#ifdef YSFX_API
# error YSFX_API is not private
#endif
#include "ysfx/include/ysfx.h"

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
    static PluginCategory getFromEffect(ysfx_t* const effect)
    {
        PluginCategory category = PLUGIN_CATEGORY_OTHER;

        if (const uint32_t tagCount = ysfx_get_tags(effect, nullptr, 0))
        {
            std::vector<const char*> tags;
            tags.resize(tagCount);
            ysfx_get_tags(effect, tags.data(), tagCount);

            for (uint32_t i=0; i<tagCount && category == PLUGIN_CATEGORY_OTHER; ++i)
            {
                PluginCategory current = getFromTag(tags[i]);

                if (current != PLUGIN_CATEGORY_NONE)
                    category = current;
            }
        }

        return category;
    }

    static PluginCategory getFromTag(const char* const tag)
    {
        if (carla_strcasecmp(tag, "synthesis") == 0)
            return PLUGIN_CATEGORY_SYNTH;

        if (carla_strcasecmp(tag, "delay") == 0)
            return PLUGIN_CATEGORY_DELAY;

        if (carla_strcasecmp(tag, "equalizer") == 0)
            return PLUGIN_CATEGORY_EQ;

        if (carla_strcasecmp(tag, "filter") == 0)
            return PLUGIN_CATEGORY_FILTER;

        if (carla_strcasecmp(tag, "distortion") == 0)
            return PLUGIN_CATEGORY_DISTORTION;

        if (carla_strcasecmp(tag, "dynamics") == 0)
            return PLUGIN_CATEGORY_DYNAMICS;

        if (carla_strcasecmp(tag, "modulation") == 0)
            return PLUGIN_CATEGORY_MODULATOR;

        if (carla_strcasecmp(tag, "utility") == 0)
            return PLUGIN_CATEGORY_UTILITY;

        return PLUGIN_CATEGORY_NONE;
    }
};

// --------------------------------------------------------------------------------------------------------------------

class CarlaJsfxUnit
{
    static String createFileId(const water::File& rootPath, const water::File& filePath)
    {
        String fileId(filePath.getRelativePathFrom(rootPath).toRawUTF8());
       #ifdef CARLA_OS_WIN
        fileId.replace('\\', '/');
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
          fFilePath(rootPath.getChildFile(fFileId).getFullPathName().toRawUTF8()),
          fRootPath(rootPath.getFullPathName().toRawUTF8()) {}

    explicit operator bool() const noexcept
    {
        return fFileId.isNotEmpty();
    }

    const String& getFileId() const noexcept
    {
        return fFileId;
    }

    const String& getFilePath() const noexcept
    {
        return fFilePath;
    }

    const String& getRootPath() const noexcept
    {
        return fRootPath;
    }

private:
    String fFileId;
    String fFilePath;
    String fRootPath;
};

// --------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_JSFX_UTILS_HPP_INCLUDED
