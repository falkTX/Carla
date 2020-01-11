/*
 * Carla Exception Tests
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
#include "CarlaJuceUtils.hpp"

// -----------------------------------------------------------------------

struct Struct1 {
    Struct1() noexcept : leakDetector_Struct1() {}
    ~Struct1() noexcept {}

    void throwHere()
    {
        throw 1;
    }

    CARLA_LEAK_DETECTOR(Struct1)
};

struct Struct2 {
    Struct2() : leakDetector_Struct2() { throw 2; }
    ~Struct2() noexcept {}

    CARLA_LEAK_DETECTOR(Struct2)
};

// -----------------------------------------------------------------------

int main()
{
    carla_safe_assert("test here", __FILE__, __LINE__);

    Struct1 a1;
    Struct1* b1;
    Struct2* c2 = nullptr;

    try {
        a1.throwHere();
    } CARLA_SAFE_EXCEPTION("Struct1 a throw");

    try {
        b1 = new Struct1;
    } CARLA_SAFE_EXCEPTION("NOT POSSIBLE! Struct1 b throw constructor");

    assert(b1 != nullptr);

    try {
        b1->throwHere();
    } CARLA_SAFE_EXCEPTION("Struct1 b throw runtime");

    delete b1;
    b1 = nullptr;

    try {
        c2 = new Struct2;
    } CARLA_SAFE_EXCEPTION("Struct2 c throw");

    assert(c2 == nullptr);

    return 0;
}

// -----------------------------------------------------------------------
