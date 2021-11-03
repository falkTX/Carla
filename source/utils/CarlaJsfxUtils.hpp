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

#include "CarlaDefines.h"
#include "CarlaUtils.hpp"
#include "CarlaString.hpp"
#include "CarlaBase64Utils.hpp"

#include "water/files/File.h"
#include "water/xml/XmlElement.h"
#include "water/xml/XmlDocument.h"
#include "water/streams/MemoryInputStream.h"
#include "water/streams/MemoryOutputStream.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wignored-attributes"
#include "jsusfx.h"
#include "jsusfx_file.h"
#include "jsusfx_gfx.h"
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

    void setMessagesQuiet(bool quiet)
    {
        fMessagesQuiet = quiet;
    }

    void setErrorsQuiet(bool quiet)
    {
        fErrorsQuiet = quiet;
    }

    void displayMsg(const char* fmt, ...) override
    {
        if (!fMessagesQuiet)
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
        if (!fErrorsQuiet)
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
    bool fMessagesQuiet = false;
    bool fErrorsQuiet = false;
};

// -------------------------------------------------------------------------------------------------------------------

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

// -------------------------------------------------------------------------------------------------------------------

class CarlaJsusFxPathLibrary : public JsusFxPathLibrary_Basic
{
public:
    explicit CarlaJsusFxPathLibrary(const CarlaJsfxUnit &unit)
        : JsusFxPathLibrary_Basic(unit.getRootPath().getFullPathName().toRawUTF8())
    {
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
        if (numVars > 0)
        {
            water::MemoryOutputStream blob;
            for (std::size_t i = 0; i < numVars; ++i)
            {
                blob.writeFloat(data.vars[i]);
            }
            const CarlaString base64 = CarlaString::asBase64(blob.getData(), blob.getDataSize());
            water::XmlElement *var = new water::XmlElement("Serialization");
            var->addTextElement(base64.buffer());
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
            else if (child->getTagName() == "Serialization")
            {
                std::vector<uint8_t> chunk = carla_getChunkFromBase64String(child->getAllSubText().toRawUTF8());
                water::MemoryInputStream blob(chunk.data(), chunk.size(), false);
                size_t numVars = chunk.size() / sizeof(float);
                data.vars.resize(numVars);
                for (std::size_t i = 0; i < numVars; ++i)
                {
                    data.vars[i] = blob.readFloat();
                }
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

#endif // CARLA_JSFX_UTILS_HPP_INCLUDED
