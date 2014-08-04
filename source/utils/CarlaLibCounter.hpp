/*
 * Carla library counter
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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
#include "LinkedList.hpp"

// -----------------------------------------------------------------------

class LibCounter
{
public:
    LibCounter() noexcept
        : fMutex(),
          fLibs() {}

    ~LibCounter() noexcept
    {
        // might have some leftovers
        for (LinkedList<Lib>::Itenerator it = fLibs.begin(); it.valid(); it.next())
        {
            Lib& lib(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(lib.count > 0);
            CARLA_SAFE_ASSERT_CONTINUE(lib.lib != nullptr);

            // all libs should be closed by now except those explicitly marked non-delete
            CARLA_SAFE_ASSERT(! lib.canDelete);

            if (! lib_close(lib.lib))
                carla_stderr("LibCounter cleanup failed, reason:\n%s", lib_error(lib.filename));

            lib.lib = nullptr;

            if (lib.filename != nullptr)
            {
                delete[] lib.filename;
                lib.filename = nullptr;
            }
        }

        fLibs.clear();
    }

    void* open(const char* const filename, const bool canDelete = true) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', nullptr);

        // try duplicating filename first, it can throw
        const char* dfilename = nullptr;

        try {
            dfilename = carla_strdup(filename);
        } CARLA_SAFE_EXCEPTION_RETURN("LibCounter::open", nullptr);

        const CarlaMutexLocker cml(fMutex);

        for (LinkedList<Lib>::Itenerator it = fLibs.begin(); it.valid(); it.next())
        {
            Lib& lib(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(lib.count > 0);
            CARLA_SAFE_ASSERT_CONTINUE(lib.filename != nullptr);

            if (std::strcmp(lib.filename, filename) == 0)
            {
                // will not be needed
                delete[] dfilename;

                ++lib.count;
                return lib.lib;
            }
        }

        void* const libPtr(lib_open(filename));

        if (libPtr == nullptr)
        {
            delete[] dfilename;
            return nullptr;
        }

        Lib lib;
        lib.lib       = libPtr;
        lib.filename  = dfilename;
        lib.count     = 1;
        lib.canDelete = canDelete;

        if (fLibs.append(lib))
            return libPtr;

        delete[] dfilename;
        return nullptr;
    }

    bool close(void* const libPtr) noexcept
    {
        CARLA_SAFE_ASSERT_RETURN(libPtr != nullptr, false);

        const CarlaMutexLocker cml(fMutex);

        for (LinkedList<Lib>::Itenerator it = fLibs.begin(); it.valid(); it.next())
        {
            Lib& lib(it.getValue());
            CARLA_SAFE_ASSERT_CONTINUE(lib.count > 0);
            CARLA_SAFE_ASSERT_CONTINUE(lib.lib != nullptr);

            if (lib.lib != libPtr)
                continue;

            if (lib.count == 1 && ! lib.canDelete)
                return true;

            if (--lib.count == 0)
            {
                if (! lib_close(lib.lib))
                    carla_stderr("LibCounter::close() failed, reason:\n%s", lib_error(lib.filename));

                lib.lib = nullptr;

                if (lib.filename != nullptr)
                {
                    delete[] lib.filename;
                    lib.filename = nullptr;
                }

                fLibs.remove(it);
            }

            return true;
        }

        carla_safe_assert("invalid lib pointer", __FILE__, __LINE__);
        return false;
    }

private:
    struct Lib {
        void* lib;
        const char* filename;
        int count;
        bool canDelete;
    };

    CarlaMutex fMutex;
    LinkedList<Lib> fLibs;
};

// -----------------------------------------------------------------------

#endif // CARLA_LIB_COUNTER_HPP_INCLUDED
