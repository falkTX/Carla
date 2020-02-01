/*
 * Carla Native Plugin API (C++)
 * Copyright (C) 2012-2019 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_NATIVE_PROGRAMS_HPP_INCLUDED
#define CARLA_NATIVE_PROGRAMS_HPP_INCLUDED

#include "CarlaNative.hpp"

#include "CarlaMathUtils.hpp"
#include "CarlaMutex.hpp"

#include "water/files/File.h"
#include "water/memory/SharedResourcePointer.h"
#include "water/text/StringArray.h"

using water::Array;
using water::File;
using water::SharedResourcePointer;
using water::String;
using water::StringArray;

/*!
 * @defgroup CarlaNativeAPI Carla Native API
 * @{
 */

// -----------------------------------------------------------------------
// ...

enum FileType {
    FileNone,
    FileAudio,
    FileMIDI,
};

template <FileType fileType>
struct NativePluginPresetManager {
    StringArray filenames;

    NativePluginPresetManager(const char* const paths, const char* const wildcard)
        : filenames()
    {
        CARLA_SAFE_ASSERT_RETURN(wildcard != nullptr,);

        if (paths == nullptr || paths[0] == '\0' || wildcard[0] == '\0')
            return;

        const StringArray splitPaths(StringArray::fromTokens(paths, CARLA_OS_SPLIT_STR, ""));

        for (String *it = splitPaths.begin(), *end = splitPaths.end(); it != end; ++it)
        {
            Array<File> results;

            if (File(*it).findChildFiles(results, File::findFiles|File::ignoreHiddenFiles, true, wildcard) > 0)
            {
                for (File *it2 = results.begin(), *end2 = results.end(); it2 != end2; ++it2)
                    filenames.add(it2->getFullPathName());
            }
        }

        filenames.sort(true);
    }
};

// -----------------------------------------------------------------------
// Native Plugin with MIDI programs class

template <FileType fileType>
class NativePluginWithMidiPrograms : public NativePluginClass
{
public:
    typedef NativePluginPresetManager<fileType> NativePluginPresetManagerType;
    typedef SharedResourcePointer<NativePluginPresetManagerType> NativeMidiPrograms;

    NativePluginWithMidiPrograms(const NativeHostDescriptor* const host,
                                 const NativeMidiPrograms& programs,
                                 const uint32_t numOutputs)
        : NativePluginClass(host),
          fRetMidiProgram(),
          fRetMidiProgramName(),
          fNextFilename(nullptr),
          fProgramChangeMutex(),
          kPrograms(programs),
          kNumOutputs(numOutputs) {}

protected:
    // -------------------------------------------------------------------
    // New Plugin program calls

    virtual void setStateFromFile(const char* filename) = 0;
    virtual void process2(const float* const* inBuffer, float** outBuffer, uint32_t frames,
                          const NativeMidiEvent* midiEvents, uint32_t midiEventCount) = 0;

    void invalidateNextFilename() noexcept
    {
        const CarlaMutexLocker cml(fProgramChangeMutex);
        fNextFilename = nullptr;
    }

    // -------------------------------------------------------------------
    // Plugin midi-program calls

    uint32_t getMidiProgramCount() const override
    {
        const NativePluginPresetManagerType& pm(kPrograms.get());
        return static_cast<uint32_t>(pm.filenames.size());
    }

    const NativeMidiProgram* getMidiProgramInfo(const uint32_t uindex) const override
    {
        const int index = static_cast<int>(uindex);

        const NativePluginPresetManagerType& pm(kPrograms.get());
        CARLA_SAFE_ASSERT_RETURN(index < pm.filenames.size(), nullptr);

        fRetMidiProgramName = File(pm.filenames.strings.getUnchecked(index)).getFileNameWithoutExtension();

        fRetMidiProgram.bank = 0;
        fRetMidiProgram.program = uindex;
        fRetMidiProgram.name = fRetMidiProgramName.toRawUTF8();

        return &fRetMidiProgram;
    }

    // -------------------------------------------------------------------
    // Plugin state calls

    void setMidiProgram(const uint8_t, const uint32_t, const uint32_t program) override
    {
        const int iprogram = static_cast<int>(program);

        const NativePluginPresetManagerType& pm(kPrograms.get());
        CARLA_SAFE_ASSERT_RETURN(iprogram < pm.filenames.size(),);

        const char* const filename(pm.filenames.strings.getUnchecked(iprogram).toRawUTF8());

        const CarlaMutexLocker cml(fProgramChangeMutex);

        if (isOffline())
        {
            setStateFromFile(filename);
        }
        else
        {
            fNextFilename = filename;
            hostRequestIdle();
        }
    }

    // -------------------------------------------------------------------
    // Plugin process calls

    void process(const float* const* const inBuffer, float** const outBuffer, uint32_t frames,
                 const NativeMidiEvent* const midiEvents, const uint32_t midiEventCount) override
    {
        const CarlaMutexTryLocker cmtl(fProgramChangeMutex, isOffline());

        if (cmtl.wasLocked())
        {
            process2(inBuffer, outBuffer, frames, midiEvents, midiEventCount);
        }
        else
        {
            for (uint32_t i=0; i<kNumOutputs; ++i)
                carla_zeroFloats(outBuffer[i], frames);
        }
    }

    // -------------------------------------------------------------------
    // Plugin dispatcher calls

    void idle() override
    {
        if (const char* const filename = fNextFilename)
        {
            const CarlaMutexLocker cml(fProgramChangeMutex);

            fNextFilename = nullptr;
            setStateFromFile(filename);
        }
    }

private:
    mutable NativeMidiProgram fRetMidiProgram;
    mutable String fRetMidiProgramName;
    const char* fNextFilename;
    CarlaMutex fProgramChangeMutex;
    const NativeMidiPrograms& kPrograms;
    const uint32_t kNumOutputs;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativePluginWithMidiPrograms)
};

/**@}*/

// -----------------------------------------------------------------------

#endif // CARLA_NATIVE_PROGRAMS_HPP_INCLUDED
