/*
 * Carla Tests
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

#include "CarlaUtils.hpp"

#include <cassert>
#include <cstdlib>

struct MyStruct {
    char pad[100];
    int i;
    double d;
    void* ptr;
};

int main()
{
    // misc functions
    bool2str(false);
    bool2str(true);
    pass();

    // string print functions, handled in Print

    // carla_*sleep
    carla_sleep(1);
    carla_msleep(1);

    // carla_setenv
    carla_setenv("THIS", "THAT");
    assert(std::strcmp(std::getenv("THIS"), "THAT") == 0);

    // carla_strdup
    const char* const str1(carla_strdup("string1"));
    const char* const strF(carla_strdup_free(strdup("stringFree")));
    delete[] str1;
    delete[] strF;

    {
        struct TestStruct {
            const char* str1;
            const char* str2;
            const char* str3;
            const char* str4;

            TestStruct()
                : str1(carla_strdup("str1")),
                  str2(carla_strdup("str2")),
                  str3(nullptr),
                  str4(carla_strdup("str4")) {}

            ~TestStruct()
            {
                if (str1 != nullptr)
                {
                    delete[] str1;
                    str1 = nullptr;
                }

                if (str2 != nullptr)
                {
                    delete[] str2;
                    str2 = nullptr;
                }

                if (str3 != nullptr)
                {
                    delete[] str3;
                    str3 = nullptr;
                }

                if (str4 != nullptr)
                {
                    delete[] str4;
                    str4 = nullptr;
                }
            }
        };

        TestStruct a, b, c;
    }

    // math/memory functions
    {
        carla_min<int32_t>(0, -5, 8);
        carla_max<int32_t>(0, -5, 8);
        carla_fixValue<float>(0.0f, 1.0f, 1.1f);

        float fl[5];
        carla_fill(fl, 5, 1.11f);
        assert(fl[0] == 1.11f);
        assert(fl[1] == 1.11f);
        assert(fl[2] == 1.11f);
        assert(fl[3] == 1.11f);
        assert(fl[4] == 1.11f);

        carla_add(fl, fl, 5);
        assert(fl[0] == 1.11f*2);
        assert(fl[1] == 1.11f*2);
        assert(fl[2] == 1.11f*2);
        assert(fl[3] == 1.11f*2);
        assert(fl[4] == 1.11f*2);

        carla_add(fl, fl, 4);
        assert(fl[0] == 1.11f*4);
        assert(fl[1] == 1.11f*4);
        assert(fl[2] == 1.11f*4);
        assert(fl[3] == 1.11f*4);
        assert(fl[4] == 1.11f*2);

        carla_add(fl, fl, 3);
        assert(fl[0] == 1.11f*8);
        assert(fl[1] == 1.11f*8);
        assert(fl[2] == 1.11f*8);
        assert(fl[3] == 1.11f*4);
        assert(fl[4] == 1.11f*2);

        carla_add(fl, fl, 2);
        assert(fl[0] == 1.11f*16);
        assert(fl[1] == 1.11f*16);
        assert(fl[2] == 1.11f*8);
        assert(fl[3] == 1.11f*4);
        assert(fl[4] == 1.11f*2);

        carla_add(fl, fl, 1);
        assert(fl[0] == 1.11f*32);
        assert(fl[1] == 1.11f*16);
        assert(fl[2] == 1.11f*8);
        assert(fl[3] == 1.11f*4);
        assert(fl[4] == 1.11f*2);

        char ch[500];
        carla_zeroChar(ch, 500);
    }

    {
        MyStruct a, b, c[2], d[2];
        carla_zeroMem(&a, sizeof(MyStruct));
        carla_zeroStruct<MyStruct>(b);
        carla_zeroStruct<MyStruct>(c, 2);
        carla_copyStruct<MyStruct>(b, a);
        carla_copyStruct<MyStruct>(d, c, 2);
    }

    return 0;
}

#include "../utils/Utils.cpp"
