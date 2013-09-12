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

struct MyStruct {
    char pad[100];
    int i;
    double d;
    intptr_t ptr;
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
    assert(std::strcmp(getenv("THIS"), "THAT") == 0);

    // carla_strdup
    const char* const str1 = carla_strdup("string1");
    const char* const strF = carla_strdup_free(strdup("stringFree"));
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

    // math functions
    carla_min<int32_t>(0, -5, 8);
    carla_fixValue<float>(0.0f, 1.0f, 1.1f);

    {
        int v1 = 6;
        int v2 = 8;
        const int v3 = 9;
        assert(v1 == 6 && v2 == 8 && v3 == 9);
        carla_copy<int>(&v1, &v2, 1);
        assert(v1 == 8 && v2 == 8 && v3 == 9);
        carla_copy<int>(&v2, &v3, 1);
        assert(v1 == 8 && v2 == 9 && v3 == 9);
    }

    {
        float data1[500];
        float data2[500];
        float data0[500];
        float data3[500];
        carla_zeroFloat(data0, 500);
        carla_fill<float>(data1, 500, 6.41f);
        carla_copy<float>(data2, data1, 500);
        carla_copyFloat(data3, data2, 500);

        carla_zeroMem(data2, sizeof(float)*500);

        for (int i=0; i < 500; ++i)
        {
            assert(data0[i] == 0.0f);
            assert(data1[i] == 6.41f);
            assert(data2[i] == 0.0f);
            assert(data3[i] == 6.41f);
        }
    }

    {
        MyStruct a, b, c, d;
        carla_zeroStruct<MyStruct>(a);
        carla_zeroStruct<MyStruct>(b);
        carla_zeroStruct<MyStruct>(c);
        carla_zeroStruct<MyStruct>(d);
    }

    return 0;
}
