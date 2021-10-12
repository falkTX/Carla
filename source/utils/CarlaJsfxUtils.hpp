/*
 * Carla JSFX utils
 * Copyright (C) 2021 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaUtils.hpp"

#include "water/files/File.h"
#include "water/xml/XmlElement.h"
#include "water/xml/XmlDocument.h"
#include "water/streams/MemoryOutputStream.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wignored-attributes"
#include "jsusfx.h"
#include "jsusfx_file.h"
#include "jsusfx_serialize.h"
#pragma GCC diagnostic pop

#include <memory>

class CarlaJsusFx : public JsusFx
{
public:
    explicit CarlaJsusFx(JsusFxPathLibrary &pathLibrary)
        : JsusFx(pathLibrary)
    {
    }

    void setQuiet(bool quiet)
    {
        fQuiet = quiet;
    }

    void displayMsg(const char* fmt, ...) override
    {
        if (!fQuiet)
        {
            char msgBuf[256];
            ::va_list args;
            ::va_start(args, fmt);
            std::vsnprintf(msgBuf, sizeof(msgBuf), fmt, args);
            msgBuf[255] = 0;
            ::va_end(args);
            carla_stdout("%s", msgBuf);
        }
    }

    void displayError(const char* fmt, ...) override
    {
        if (!fQuiet)
        {
            char msgBuf[256];
            ::va_list args;
            ::va_start(args, fmt);
            std::vsnprintf(msgBuf, sizeof(msgBuf), fmt, args);
            msgBuf[255] = 0;
            ::va_end(args);
            carla_stderr("%s", msgBuf);
        }
    }

private:
    bool fQuiet = false;
};

// -------------------------------------------------------------------------------------------------------------------

class CarlaJsusFxPathLibrary : public JsusFxPathLibrary_Basic
{
public:
    explicit CarlaJsusFxPathLibrary(const water::File &file)
        : JsusFxPathLibrary_Basic(determineDataRoot(file).c_str())
    {
    }

private:
    static std::string determineDataRoot(const water::File &file)
    {
        return file.getParentDirectory().getFullPathName().toRawUTF8();
    }
};

// -------------------------------------------------------------------------------------------------------------------

class CarlaJsusFxFileAPI : public JsusFxFileAPI_Basic
{
public:
};

// -------------------------------------------------------------------------------------------------------------------

class CarlaJsusFxSerializer : public JsusFxSerializer_Basic
{
public:
    explicit CarlaJsusFxSerializer(JsusFxSerializationData& data)
        : JsusFxSerializer_Basic(data)
    {
    }

    static water::String convertDataToString(const JsusFxSerializationData& data)
    {
        water::XmlElement root("JSFXState");

        std::size_t numSliders = data.sliders.size();
        for (std::size_t i = 0; i < numSliders; ++i)
        {
            water::XmlElement *slider = new water::XmlElement("Slider");
            slider->setAttribute("index", data.sliders[i].index);
            slider->setAttribute("value", data.sliders[i].value);
            root.addChildElement(slider);
        }

        std::size_t numVars = data.vars.size();
        for (std::size_t i = 0; i < numVars; ++i)
        {
            water::XmlElement *var = new water::XmlElement("Var");
            var->setAttribute("value", data.vars[i]);
            root.addChildElement(var);
        }

        water::MemoryOutputStream stream;
        root.writeToStream(stream, water::StringRef(), true);
        return stream.toUTF8();
    }

    static bool convertStringToData(const water::String& string, JsusFxSerializationData& data)
    {
        std::unique_ptr<water::XmlElement> root(water::XmlDocument::parse(string));

        CARLA_SAFE_ASSERT_RETURN(root != nullptr, false);
        CARLA_SAFE_ASSERT_RETURN(root->getTagName() == "JSFXState", false);

        data = JsusFxSerializationData();

        int numChildren = root->getNumChildElements();
        for (int i = 0; i < numChildren; ++i)
        {
            water::XmlElement* child = root->getChildElement(i);
            CARLA_SAFE_ASSERT_CONTINUE(child != nullptr);

            if (child->getTagName() == "Slider")
            {
                CARLA_SAFE_ASSERT_CONTINUE(child->hasAttribute("index"));
                CARLA_SAFE_ASSERT_CONTINUE(child->hasAttribute("value"));

                JsusFxSerializationData::Slider slider;
                slider.index = child->getIntAttribute("index");
                slider.value = child->getDoubleAttribute("value");
                data.sliders.push_back(slider);
            }
            else if (child->getTagName() == "Var")
            {
                CARLA_SAFE_ASSERT_CONTINUE(child->hasAttribute("value"));

                float value = child->getDoubleAttribute("value");
                data.vars.push_back(value);
            }
            else
            {
                CARLA_SAFE_ASSERT_CONTINUE(false);
            }
        }

        return true;
    }
};

// -------------------------------------------------------------------------------------------------------------------

#endif // CARLA_DSSI_UTILS_HPP_INCLUDED
