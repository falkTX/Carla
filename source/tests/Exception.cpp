/*
 * Carla Tests
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

#include "CarlaDefines.h"
#include "CarlaUtils.hpp"

struct Struct1 {
    Struct1() noexcept {}
    ~Struct1() noexcept {}

    void throwHere()
    {
        throw 1;
    }
};

struct Struct2 {
    Struct2() { throw 2; }
    ~Struct2() noexcept {}

    void throwHere()
    {
        throw 3;
    }
};

int main()
{
    carla_safe_assert("test here", __FILE__, __LINE__);

    Struct1 a;
    Struct2* b = nullptr;

    try {
        a.throwHere();
    } CARLA_SAFE_EXCEPTION("Struct1 throw", carla_stdout("after text1 here");)

    try {
        b = new Struct2;
    } CARLA_SAFE_EXCEPTION("Struct2 throw", carla_stdout("after text2 here");)

    if (b != nullptr)
    {
        delete b;
        b = nullptr;
    }

    return 0;
}
