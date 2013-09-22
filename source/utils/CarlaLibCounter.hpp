/*
 * Carla library counter
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

#ifndef CARLA_LIB_COUNTER_HPP_INCLUDED
#define CARLA_LIB_COUNTER_HPP_INCLUDED

#include "CarlaLibUtils.hpp"
#include "CarlaMutex.hpp"
#include "RtList.hpp"

// -----------------------------------------------------------------------

class LibCounter
{
public:
    LibCounter() {}

    ~LibCounter()
    {
        CARLA_ASSERT(libs.isEmpty());
    }

    void* open(const char* const filename)
    {
        CARLA_SAFE_ASSERT_RETURN(filename != nullptr, nullptr);

        const CarlaMutex::ScopedLocker sl(mutex);

        for (NonRtList<Lib>::Itenerator it = libs.begin(); it.valid(); it.next())
        {
            Lib& lib(*it);
            CARLA_ASSERT(lib.count > 0);
            CARLA_SAFE_ASSERT_CONTINUE(lib.filename != nullptr);

            if (std::strcmp(lib.filename, filename) == 0)
            {
                ++lib.count;
                return lib.lib;
            }
        }

        void* const libPtr(lib_open(filename));

        if (libPtr == nullptr)
            return nullptr;

        Lib lib;
        lib.lib      = libPtr;
        lib.filename = carla_strdup(filename);
        lib.count    = 1;

        libs.append(lib);

        return libPtr;
    }

    bool close(void* const libPtr)
    {
        CARLA_SAFE_ASSERT_RETURN(libPtr != nullptr, false);

        const CarlaMutex::ScopedLocker sl(mutex);

        for (NonRtList<Lib>::Itenerator it = libs.begin(); it.valid(); it.next())
        {
            Lib& lib(*it);
            CARLA_ASSERT(lib.count > 0);
            CARLA_SAFE_ASSERT_CONTINUE(lib.lib != nullptr);

            if (lib.lib != libPtr)
                continue;

            if (--lib.count == 0)
            {
                if (lib.filename != nullptr)
                {
                    delete[] lib.filename;
                    lib.filename = nullptr;
                }

                lib_close(lib.lib);
                lib.lib = nullptr;

                libs.remove(it);
            }

            return true;
        }

        CARLA_ASSERT(false); // invalid pointer
        return false;
    }

private:
    struct Lib {
        void* lib;
        const char* filename;
        int count;
    };

    CarlaMutex mutex;
    NonRtList<Lib> libs;
};

// -----------------------------------------------------------------------

#endif // CARLA_LIB_COUNTER_HPP_INCLUDED
