/*
 * Carla Utility Tests
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

#ifdef NDEBUG
# error Build this file with debug ON please
#endif

#include "CarlaUtils.hpp"
#include "CarlaMathUtils.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaBridgeUtils.hpp"
#include "CarlaEngineUtils.hpp"

// -----------------------------------------------------------------------

static void test_CarlaUtils()
{
    // -------------------------------------------------------------------
    // misc functions

    {
        bool2str(false);
        bool2str(true);
        pass();

        char strBuf[1];
        nullStrBuf(strBuf);
    }

    // -------------------------------------------------------------------
    // string print functions

    {
        carla_debug("DEBUG");
        carla_stdout("STDOUT %s", bool2str(true));
        carla_stderr("STDERR %s", bool2str(false));
        carla_stderr2("STDERR2 " P_UINT64, 0xffffffff); // 4294967295
    }

    // -------------------------------------------------------------------
    // carla_*sleep

    {
        carla_sleep(1);
        carla_msleep(1);
    }

    // -------------------------------------------------------------------
    // carla_setenv

    {
        carla_setenv("THIS", "THAT");
        assert(std::strcmp(std::getenv("THIS"), "THAT") == 0);
    }

    // -------------------------------------------------------------------
    // carla_strdup

    {
        // with variables
        const char* const str1(carla_strdup("string1"));
        const char* const strF(carla_strdup_free(strdup("string2")));
        delete[] str1;
        delete[] strF;

        // without variables
        delete[] carla_strdup("string3");
        delete[] carla_strdup_free(strdup("string4"));

        // common use case in Carla code
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

            ~TestStruct() noexcept
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

            CARLA_DECLARE_NON_COPY_STRUCT(TestStruct)
        };

        TestStruct a, b, c;
    }

    // -------------------------------------------------------------------
    // memory functions

    {
        int a[] = { 4, 3, 2, 1 };
        int b[] = { 1, 2, 3, 4 };
        carla_add(a, b, 4);
        assert(a[0] == 5);
        assert(a[1] == 5);
        assert(a[2] == 5);
        assert(a[3] == 5);

        int c[] = { 4, 3, 2, 1 };
        carla_add(b, c, 4);
        assert(a[0] == b[0]);
        assert(a[1] == b[1]);
        assert(a[2] == b[2]);
        assert(a[3] == b[3]);

        carla_copy(a,   c,   2);
        carla_copy(b+2, c+2, 2);
        assert(a[0] == c[0]);
        assert(a[1] == c[1]);
        assert(b[2] == c[2]);
        assert(b[3] == c[3]);
        carla_copy(c, a, 4);
        assert(a[0] == c[0]);
        assert(a[1] == c[1]);
        assert(a[2] == c[2]);
        assert(a[3] == c[3]);

        carla_fill(a, 0, 4);
        assert(a[0] == 0);
        assert(a[1] == 0);
        assert(a[2] == 0);
        assert(a[3] == 0);

        carla_fill(a, -11, 4);
        assert(a[0] == -11);
        assert(a[1] == -11);
        assert(a[2] == -11);
        assert(a[3] == -11);

        carla_fill(a+0, 17, 2);
        carla_fill(a+2, 23, 2);
        assert(a[0] == 17);
        assert(a[1] == 17);
        assert(a[2] == 23);
        assert(a[3] == 23);

        carla_zeroBytes(a, sizeof(int)*4);
        assert(a[0] == 0);
        assert(a[1] == 0);
        assert(a[2] == 0);
        assert(a[3] == 0);

        char strBuf[501];
        strBuf[0] = strBuf[499] = strBuf[500] = '!';
        carla_zeroChar(strBuf, 501);

        for (int i=0; i<501; ++i)
            assert(strBuf[i] == '\0');

        int d = 1527, e = 0;
        carla_add(&d, &d, 1);
        carla_add(&d, &e, 1);
        carla_add(&e, &d, 1);
        assert(d == 1527*2);
        assert(d == e);

        e = -e;
        carla_add(&d, &e, 1);
        assert(d == 0);

        carla_copy(&d, &e, 1);
        assert(d == -1527*2);
        assert(d == e);

        carla_fill(&d, 9999, 1);
        carla_fill(&e, 123, 1);
        assert(d == 9999);
        assert(e == 123);

        carla_zeroStruct(d);
        assert(d == 0);

        carla_copyStruct(d, e);
        assert(d != 0);
        assert(d == e);

        carla_fill(a+0, 1, 1);
        carla_fill(a+1, 2, 1);
        carla_fill(a+2, 3, 1);
        carla_fill(a+3, 4, 1);
        assert(a[0] == 1);
        assert(a[1] == 2);
        assert(a[2] == 3);
        assert(a[3] == 4);

        carla_copyStruct(c, a, 4);
        assert(c[0] == a[0]);
        assert(c[1] == a[1]);
        assert(c[2] == a[2]);
        assert(c[3] == a[3]);

        carla_zeroStruct(c, 4);
        assert(c[0] == 0);
        assert(c[1] == 0);
        assert(c[2] == 0);
        assert(c[3] == 0);

        float fl[5];
        carla_fill(fl, 1.11f, 5);
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
    }
}

// -----------------------------------------------------------------------

static void test_CarlaMathUtils() noexcept
{
    // -------------------------------------------------------------------
    // math functions (base)

    // Return the lower of 2 values, with 'min' as the minimum possible value.
    assert(carla_min(0, 0, 0) == 0);
    assert(carla_min(0, 3, 0) == 0);
    assert(carla_min(3, 0, 0) == 0);

    assert(carla_min(0, 0, 3) == 3);
    assert(carla_min(0, 1, 3) == 3);
    assert(carla_min(0, 2, 3) == 3);
    assert(carla_min(0, 3, 3) == 3);
    assert(carla_min(0, 4, 3) == 3);

    assert(carla_min(5, 0, 3) == 3);
    assert(carla_min(5, 1, 3) == 3);
    assert(carla_min(5, 2, 3) == 3);
    assert(carla_min(5, 3, 3) == 3);
    assert(carla_min(5, 4, 3) == 4);
    assert(carla_min(5, 5, 3) == 5);
    assert(carla_min(5, 6, 3) == 5);

    assert(carla_min(2, -2, 0) == 0);
    assert(carla_min(2, -1, 0) == 0);
    assert(carla_min(2,  0, 0) == 0);
    assert(carla_min(2,  1, 0) == 1);
    assert(carla_min(2,  2, 0) == 2);

    assert(carla_min(5, -6, 3) == 3);
    assert(carla_min(5, -5, 3) == 3);
    assert(carla_min(5, -4, 3) == 3);
    assert(carla_min(5, -3, 3) == 3);
    assert(carla_min(5, -2, 3) == 3);
    assert(carla_min(5, -1, 3) == 3);
    assert(carla_min(5,  0, 3) == 3);
    assert(carla_min(5,  1, 3) == 3);
    assert(carla_min(5,  2, 3) == 3);
    assert(carla_min(5,  3, 3) == 3);
    assert(carla_min(5,  4, 3) == 4);
    assert(carla_min(5,  5, 3) == 5);
    assert(carla_min(5,  6, 3) == 5);

    assert(carla_min(5, -6, -3) == -3);
    assert(carla_min(5, -5, -3) == -3);
    assert(carla_min(5, -4, -3) == -3);
    assert(carla_min(5, -3, -3) == -3);
    assert(carla_min(5, -2, -3) == -2);
    assert(carla_min(5, -1, -3) == -1);
    assert(carla_min(5,  0, -3) == 0);
    assert(carla_min(5,  1, -3) == 1);
    assert(carla_min(5,  2, -3) == 2);
    assert(carla_min(5,  3, -3) == 3);
    assert(carla_min(5,  4, -3) == 4);
    assert(carla_min(5,  5, -3) == 5);
    assert(carla_min(5,  6, -3) == 5);

    // Return the lower positive of 2 values.
    assert(carla_minPositive(0, 0) == 0);
    assert(carla_minPositive(0, 1) == 0);
    assert(carla_minPositive(0, 2) == 0);
    assert(carla_minPositive(0, 3) == 0);

    assert(carla_minPositive(1, 0) == 0);
    assert(carla_minPositive(1, 1) == 1);
    assert(carla_minPositive(1, 2) == 1);
    assert(carla_minPositive(1, 3) == 1);

    assert(carla_minPositive(3, 0) == 0);
    assert(carla_minPositive(3, 1) == 1);
    assert(carla_minPositive(3, 2) == 2);
    assert(carla_minPositive(3, 3) == 3);

    assert(carla_minPositive(-1, 0) == 0);
    assert(carla_minPositive(-1, 1) == 1);
    assert(carla_minPositive(-1, 2) == 2);
    assert(carla_minPositive(-1, 3) == 3);

    // Return the higher of 2 values, with 'max' as the maximum possible value.
    assert(carla_max(0, 0, 0) == 0);
    assert(carla_max(0, 3, 0) == 0);
    assert(carla_max(3, 0, 0) == 0);

    assert(carla_max(0, 0, 3) == 0);
    assert(carla_max(0, 1, 3) == 1);
    assert(carla_max(0, 2, 3) == 2);
    assert(carla_max(0, 3, 3) == 3);
    assert(carla_max(0, 4, 3) == 3);

    assert(carla_max(5, 0, 3) == 3);
    assert(carla_max(5, 1, 3) == 3);
    assert(carla_max(5, 2, 3) == 3);
    assert(carla_max(5, 3, 3) == 3);
    assert(carla_max(5, 4, 3) == 3);
    assert(carla_max(5, 5, 3) == 3);
    assert(carla_max(5, 6, 3) == 3);

    // Fix bounds of 'value' between 'min' and 'max'.
    assert(carla_fixValue(0, 1, -1) == 0);
    assert(carla_fixValue(0, 1,  0) == 0);
    assert(carla_fixValue(0, 1,  1) == 1);
    assert(carla_fixValue(0, 1,  2) == 1);

    assert(carla_fixValue(0.0, 1.0, -1.0) == 0.0);
    assert(carla_fixValue(0.0, 1.0,  0.0) == 0.0);
    assert(carla_fixValue(0.0, 1.0,  1.0) == 1.0);
    assert(carla_fixValue(0.0, 1.0,  2.0) == 1.0);

    assert(carla_fixValue(0.0, 1.0, -0.1) == 0.0);
    assert(carla_fixValue(0.0, 1.0,  1.1) == 1.0);

    // Get next power of 2.
    assert(carla_nextPowerOf2(0) == 0);
    assert(carla_nextPowerOf2(1) == 1);
    assert(carla_nextPowerOf2(2) == 2);
    assert(carla_nextPowerOf2(4) == 4);
    assert(carla_nextPowerOf2(5) == 8);
    assert(carla_nextPowerOf2(6) == 8);
    assert(carla_nextPowerOf2(7) == 8);
    assert(carla_nextPowerOf2(8) == 8);
    assert(carla_nextPowerOf2(9) == 16);

    uint32_t power = 1;
    assert((power = carla_nextPowerOf2(power+1)) == 2);
    assert((power = carla_nextPowerOf2(power+1)) == 4);
    assert((power = carla_nextPowerOf2(power+1)) == 8);
    assert((power = carla_nextPowerOf2(power+1)) == 16);
    assert((power = carla_nextPowerOf2(power+1)) == 32);
    assert((power = carla_nextPowerOf2(power+1)) == 64);
    assert((power = carla_nextPowerOf2(power+1)) == 128);
    assert((power = carla_nextPowerOf2(power+1)) == 256);
    assert((power = carla_nextPowerOf2(power+1)) == 512);
    assert((power = carla_nextPowerOf2(power+1)) == 1024);
    assert((power = carla_nextPowerOf2(power+1)) == 2048);
    assert((power = carla_nextPowerOf2(power+1)) == 4096);
    assert((power = carla_nextPowerOf2(power+1)) == 8192);
    assert((power = carla_nextPowerOf2(power+1)) == 16384);
    assert((power = carla_nextPowerOf2(power+1)) == 32768);
    assert((power = carla_nextPowerOf2(power+1)) == 65536);
    assert((power = carla_nextPowerOf2(power+1)) == 131072);

    // -------------------------------------------------------------------
    // math functions (extended)

    // carla_addFloat, carla_copyFloat & carla_zeroFloat tests skipped
    // mostly unused due to juce::FloatVectorOperations
}

// -----------------------------------------------------------------------

static void test_CarlaBackendUtils() noexcept
{
    CARLA_BACKEND_USE_NAMESPACE
    carla_stdout(PluginOption2Str(PLUGIN_OPTION_FIXED_BUFFERS));
    carla_stdout(BinaryType2Str(BINARY_NONE));
    carla_stdout(PluginType2Str(PLUGIN_NONE));
    carla_stdout(PluginCategory2Str(PLUGIN_CATEGORY_NONE));
    carla_stdout(ParameterType2Str(PARAMETER_UNKNOWN));
    carla_stdout(InternalParameterIndex2Str(PARAMETER_NULL));
    carla_stdout(EngineCallbackOpcode2Str(ENGINE_CALLBACK_DEBUG));
    carla_stdout(EngineOption2Str(ENGINE_OPTION_DEBUG));
    carla_stdout(EngineProcessMode2Str(ENGINE_PROCESS_MODE_SINGLE_CLIENT));
    carla_stdout(EngineTransportMode2Str(ENGINE_TRANSPORT_MODE_INTERNAL));
    carla_stdout(FileCallbackOpcode2Str(FILE_CALLBACK_DEBUG));
    carla_stdout(getPluginTypeAsString(PLUGIN_INTERNAL));
    carla_stdout(PatchbayIcon2Str(PATCHBAY_ICON_APPLICATION));
    getPluginTypeFromString("none");
    getPluginCategoryFromName("cat");
}

// -----------------------------------------------------------------------

static void test_CarlaBridgeUtils() noexcept
{
    carla_stdout(PluginBridgeOscInfoType2str(kPluginBridgeOscPong));
    carla_stdout(PluginBridgeRtOpcode2str(kPluginBridgeRtNull));
    carla_stdout(PluginBridgeNonRtOpcode2str(kPluginBridgeNonRtNull));
}

// -----------------------------------------------------------------------

static void test_CarlaEngineUtils() noexcept
{
    CARLA_BACKEND_USE_NAMESPACE
    carla_stdout(EngineType2Str(kEngineTypeNull));
    carla_stdout(EnginePortType2Str(kEnginePortTypeNull));
    carla_stdout(EngineEventType2Str(kEngineEventTypeNull));
    carla_stdout(EngineControlEventType2Str(kEngineControlEventTypeNull));
}

// -----------------------------------------------------------------------
// main

int main()
{
    // already tested, skip for now
    test_CarlaUtils();
    test_CarlaMathUtils();

    test_CarlaBackendUtils();
    test_CarlaBridgeUtils();
    test_CarlaEngineUtils();

    return 0;
}

// -----------------------------------------------------------------------
