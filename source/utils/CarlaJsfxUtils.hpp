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
#include "CarlaString.hpp"
#include "CarlaBase64Utils.hpp"
#include "CarlaJuceUtils.hpp"

#include "water/files/File.h"
#include "water/files/FileInputStream.h"
#include "water/xml/XmlElement.h"
#include "water/xml/XmlDocument.h"
#include "water/streams/MemoryInputStream.h"
#include "water/streams/MemoryOutputStream.h"

#ifdef YSFX_API
# error YSFX_API is not private
#endif
#include "ysfx/include/ysfx.h"

#include <memory>

CARLA_BACKEND_START_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

class CarlaJsfxLogging
{
public:
    static void logAll(intptr_t, ysfx_log_level level, const char *message)
    {
        switch (level)
        {
        case ysfx_log_info:
            carla_stdout("%s: %s", ysfx_log_level_string(level), message);
            break;
        case ysfx_log_warning:
        case ysfx_log_error:
            carla_stderr("%s: %s", ysfx_log_level_string(level), message);
            break;
        }
    };

    static void logErrorsOnly(intptr_t, ysfx_log_level level, const char *message)
    {
        switch (level)
        {
        case ysfx_log_info:
            break;
        case ysfx_log_warning:
        case ysfx_log_error:
            carla_stderr("%s: %s", ysfx_log_level_string(level), message);
            break;
        }
    };
};

// --------------------------------------------------------------------------------------------------------------------

class CarlaJsfxCategories
{
public:
    static PluginCategory getFromEffect(ysfx_t* effect)
    {
        PluginCategory category = PLUGIN_CATEGORY_OTHER;

        water::Array<const char*> tags;
        int tagCount = (int)ysfx_get_tags(effect, nullptr, 0);
        tags.resize(tagCount);
        ysfx_get_tags(effect, tags.getRawDataPointer(), (uint32_t)tagCount);

        for (int i = 0; i < tagCount && category == PLUGIN_CATEGORY_OTHER; ++i)
        {
            water::CharPointer_UTF8 tag(tags[i]);
            PluginCategory current = getFromTag(tag);
            if (current != PLUGIN_CATEGORY_NONE)
                category = current;
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

// -------------------------------------------------------------------------------------------------------------------

class CarlaJsfxState
{
public:
    static water::String convertToString(ysfx_state_t &state)
    {
        water::XmlElement root("JSFXState");

        for (uint32_t i = 0; i < state.slider_count; ++i)
        {
            water::XmlElement *slider = new water::XmlElement("Slider");
            slider->setAttribute("index", (int32_t)state.sliders[i].index);
            slider->setAttribute("value", state.sliders[i].value);
            root.addChildElement(slider);
        }

        {
            const CarlaString base64 = CarlaString::asBase64(state.data, state.data_size);
            water::XmlElement *var = new water::XmlElement("Serialization");
            var->addTextElement(base64.buffer());
            root.addChildElement(var);
        }

        water::MemoryOutputStream stream;
        root.writeToStream(stream, water::StringRef(), true);
        return stream.toUTF8();
    }

    static ysfx_state_u convertFromString(const water::String& string)
    {
        std::unique_ptr<water::XmlElement> root(water::XmlDocument::parse(string));

        CARLA_SAFE_ASSERT_RETURN(root != nullptr, nullptr);
        CARLA_SAFE_ASSERT_RETURN(root->getTagName() == "JSFXState", nullptr);

        std::vector<ysfx_state_slider_t> sliders;
        std::vector<uint8_t> serialization;

        sliders.reserve(ysfx_max_sliders);

        int numChildren = root->getNumChildElements();
        for (int i = 0; i < numChildren; ++i)
        {
            water::XmlElement* child = root->getChildElement(i);
            CARLA_SAFE_ASSERT_CONTINUE(child != nullptr);

            if (child->getTagName() == "Slider")
            {
                CARLA_SAFE_ASSERT_CONTINUE(child->hasAttribute("index"));
                CARLA_SAFE_ASSERT_CONTINUE(child->hasAttribute("value"));

                ysfx_state_slider_t item;
                int32_t index = child->getIntAttribute("index");
                CARLA_SAFE_ASSERT_CONTINUE(index >= 0 && index < ysfx_max_sliders);

                item.index = (uint32_t)index;
                item.value = child->getDoubleAttribute("value");
                sliders.push_back(item);
            }
            else if (child->getTagName() == "Serialization")
            {
                serialization = carla_getChunkFromBase64String(child->getAllSubText().toRawUTF8());
            }
            else
            {
                CARLA_SAFE_ASSERT_CONTINUE(false);
            }
        }

        ysfx_state_t state;
        state.sliders = sliders.data();
        state.slider_count = (uint32_t)sliders.size();
        state.data = serialization.data();
        state.data_size = (uint32_t)serialization.size();

        return ysfx_state_u(ysfx_state_dup(&state));
    }
};

// --------------------------------------------------------------------------------------------------------------------

class CarlaJsfxUnit
{
public:
    CarlaJsfxUnit() = default;

    CarlaJsfxUnit(const water::File& rootPath, const water::File& filePath)
        : fRootPath(rootPath), fFileId(filePath.getRelativePathFrom(rootPath))
    {
#ifdef CARLA_OS_WIN
        fFileId.replaceCharacter('\\', '/');
#endif
    }

    explicit operator bool() const
    {
        return fFileId.isNotEmpty();
    }

    const water::File& getRootPath() const
    {
        return fRootPath;
    }

    const water::String& getFileId() const
    {
        return fFileId;
    }

    water::File getFilePath() const
    {
        return fRootPath.getChildFile(fFileId);
    }

private:
    water::File fRootPath;
    water::String fFileId;
};

// --------------------------------------------------------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE

#endif // CARLA_JSFX_UTILS_HPP_INCLUDED
