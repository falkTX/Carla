/*
 * Carla Utility Tests
 * Copyright (C) 2013-2015 Filipe Coelho <falktx@falktx.com>
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

// #include "CarlaBackendUtils.hpp"
// #include "CarlaBridgeUtils.hpp"
// #include "CarlaEngineUtils.hpp"

// --------------------------------------------------------------------------------------------------------------------

static void test_CarlaUtils()
{
    // ----------------------------------------------------------------------------------------------------------------
    // misc functions

    {
        bool2str(false);
        bool2str(true);
        pass();

        char strBuf[2];
        nullStrBuf(strBuf);

        char* const strBuf2(strBuf+1);
        nullStrBuf(strBuf2);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // string print functions

    {
        carla_debug("DEBUG");
        carla_stdout("STDOUT %s", bool2str(true));
        carla_stderr("STDERR %s", bool2str(false));
        carla_stderr2("STDERR2 " P_UINT64, 0xffffffff); // 4294967295
    }

    // ----------------------------------------------------------------------------------------------------------------
    // carla_*sleep

    {
        carla_sleep(1);
        carla_msleep(1);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // carla_setenv

    {
        carla_setenv("THIS", "THAT");
        assert(std::strcmp(std::getenv("THIS"), "THAT") == 0);

        carla_unsetenv("THIS");
        assert(std::getenv("THIS") == nullptr);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // carla_strdup

    {
        // with variables
        const char* const str1(carla_strdup("stringN"));
        const char* const strF(carla_strdup_free(strdup("stringF")));
        const char* const str2(carla_strdup_safe("stringS"));
        delete[] str1;
        delete[] str2;
        delete[] strF;

        // without variables
        delete[] carla_strdup("string_normal");
        delete[] carla_strdup_free(strdup("string_free"));
        delete[] carla_strdup_safe("string_safe");
    }

    {
        // common use case in Carla code
        struct TestStruct {
            const char* strNull;
            const char* strNormal;
            const char* strFree;
            const char* strSafe;

            TestStruct()
                : strNull(nullptr),
                  strNormal(carla_strdup("strNormal")),
                  strFree(carla_strdup_free(strdup("strFree"))),
                  strSafe(carla_strdup_safe("strSafe")) {}

            ~TestStruct() noexcept
            {
                if (strNull != nullptr)
                {
                    delete[] strNull;
                    strNull = nullptr;
                }

                if (strNormal != nullptr)
                {
                    delete[] strNormal;
                    strNormal = nullptr;
                }

                if (strFree != nullptr)
                {
                    delete[] strFree;
                    strFree = nullptr;
                }

                if (strSafe != nullptr)
                {
                    delete[] strSafe;
                    strSafe = nullptr;
                }
            }

            CARLA_DECLARE_NON_COPY_STRUCT(TestStruct)
        };

        TestStruct a, b, c;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // memory functions

    {
        int a1[] = { 4, 3, 2, 1 };
        int a2[] = { 4, 3, 2, 1 };
        int b1[] = { 1, 2, 3, 4 };
        int b2[] = { 1, 2, 3, 4 };

        carla_add(a1, b1, 4);
        assert(a1[0] == 5);
        assert(a1[1] == 5);
        assert(a1[2] == 5);
        assert(a1[3] == 5);

        carla_add(b1, a2, 4);
        assert(b1[0] == 5);
        assert(b1[1] == 5);
        assert(b1[2] == 5);
        assert(b1[3] == 5);
        assert(a1[0] == b1[0]);
        assert(a1[1] == b1[1]);
        assert(a1[2] == b1[2]);
        assert(a1[3] == b1[3]);

        carla_copy(a1, b2, 4);
        assert(a1[0] != b1[0]);
        assert(a1[1] != b1[1]);
        assert(a1[2] != b1[2]);
        assert(a1[3] != b1[3]);
        assert(a1[0] == b2[0]);
        assert(a1[1] == b2[1]);
        assert(a1[2] == b2[2]);
        assert(a1[3] == b2[3]);

        carla_copy(a1, b1, 4);
        assert(a1[0] == b1[0]);
        assert(a1[1] == b1[1]);
        assert(a1[2] == b1[2]);
        assert(a1[3] == b1[3]);

        carla_copy(a1, b2, 2);
        assert(a1[0] != b1[0]);
        assert(a1[1] != b1[1]);
        assert(a1[2] == b1[2]);
        assert(a1[3] == b1[3]);

        carla_copy(a1+2, b2+2, 2);
        assert(a1[0] != b1[0]);
        assert(a1[1] != b1[1]);
        assert(a1[2] != b1[2]);
        assert(a1[3] != b1[3]);

        carla_copy(a1, b1, 2);
        assert(a1[0] == b1[0]);
        assert(a1[1] == b1[1]);
        assert(a1[2] != b1[2]);
        assert(a1[3] != b1[3]);

        carla_copy(a1+2, b1+2, 2);
        assert(a1[0] == b1[0]);
        assert(a1[1] == b1[1]);
        assert(a1[2] == b1[2]);
        assert(a1[3] == b1[3]);

        carla_fill(a1, 0, 4);
        assert(a1[0] == 0);
        assert(a1[1] == 0);
        assert(a1[2] == 0);
        assert(a1[3] == 0);

        carla_fill(a1, -11, 4);
        assert(a1[0] == -11);
        assert(a1[1] == -11);
        assert(a1[2] == -11);
        assert(a1[3] == -11);

        carla_fill(a1, 1791, 2);
        assert(a1[0] == 1791);
        assert(a1[1] == 1791);
        assert(a1[2] == -11);
        assert(a1[3] == -11);

        carla_fill(a1+2, 1791, 2);
        assert(a1[0] == 1791);
        assert(a1[1] == 1791);
        assert(a1[2] == 1791);
        assert(a1[3] == 1791);

        int16_t d = 1527, e = 0;

        carla_add(&d, &d, 1);
        assert(d == 1527*2);

        carla_add(&d, &d, 1);
        assert(d == 1527*4);

        carla_add(&d, &e, 1);
        assert(d == 1527*4);
        assert(e == 0);

        carla_add(&e, &d, 1);
        assert(d == e);

        carla_add(&e, &d, 1);
        assert(e == d*2);

        d = -e;
        carla_add(&d, &e, 1);
        assert(d == 0);
    }

    {
        bool x;
        const bool f = false, t = true;

        carla_copy(&x, &t, 1);
        assert(x);

        carla_copy(&x, &f, 1);
        assert(! x);

        carla_fill(&x, true, 1);
        assert(x);

        carla_fill(&x, false, 1);
        assert(! x);
    }

    {
        uint8_t a[] = { 3, 2, 1, 0 };
        carla_zeroBytes(a, 1);
        assert(a[0] == 0);
        assert(a[1] == 2);
        assert(a[2] == 1);
        assert(a[3] == 0);

        carla_zeroBytes(a+1, 2);
        assert(a[0] == 0);
        assert(a[1] == 0);
        assert(a[2] == 0);
        assert(a[3] == 0);
    }

    {
        char a[501];

        for (int i=500; --i>=0;)
            a[i] = 'a';

        carla_zeroChars(a, 501);

        for (int i=501; --i>=0;)
            assert(a[i] == '\0');

        for (int i=500; --i>=0;)
            a[i] = 'a';

        assert(std::strlen(a) == 500);

        carla_fill(a+200, '\0', 1);
        assert(std::strlen(a) == 200);
    }

    {
        void* a[33];
        carla_zeroPointers(a, 33);

        for (int i=33; --i>=0;)
            assert(a[i] == nullptr);
    }

    {
        struct Thing {
            char c;
            int i;
            int64_t h;

            bool operator==(const Thing& t) const noexcept
            {
                return (t.c == c && t.i == i && t.h == h);
            }

            bool operator!=(const Thing& t) const noexcept
            {
                return !operator==(t);
            }
        };

        Thing a, b, c;

        a.c = 0;
        a.i = 0;
        a.h = 0;

        b.c = 64;
        b.i = 64;
        b.h = 64;

        c = a;

        carla_copyStruct(a, b);
        assert(a == b);

        carla_zeroStruct(a);
        assert(a == c);

        carla_copyStruct(c, b);
        assert(a != c);

        // make it non-zero
        a.c = 1;

        Thing d[3];

        carla_zeroStructs(d, 3);
        assert(d[0] != a);
        assert(d[1] != b);
        assert(d[2] != c);

        carla_copyStructs(d, &a, 1);
        assert(d[0] == a);
        assert(d[1] != b);
        assert(d[2] != c);

        carla_copyStructs(&c, d+2, 1);
        assert(d[0] == a);
        assert(d[1] != b);
        assert(d[2] == c);
    }
}

#if 0
// --------------------------------------------------------------------------------------------------------------------

static void test_CarlaMathUtils() noexcept
{
    // ----------------------------------------------------------------------------------------------------------------
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
    assert(carla_fixedValue(0, 1, -1) == 0);
    assert(carla_fixedValue(0, 1,  0) == 0);
    assert(carla_fixedValue(0, 1,  1) == 1);
    assert(carla_fixedValue(0, 1,  2) == 1);

    assert(carla_fixedValue(0.0, 1.0, -1.0) == 0.0);
    assert(carla_fixedValue(0.0, 1.0,  0.0) == 0.0);
    assert(carla_fixedValue(0.0, 1.0,  1.0) == 1.0);
    assert(carla_fixedValue(0.0, 1.0,  2.0) == 1.0);

    assert(carla_fixedValue(0.0, 1.0, -0.1) == 0.0);
    assert(carla_fixedValue(0.0, 1.0,  1.1) == 1.0);

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

    // ----------------------------------------------------------------------------------------------------------------
    // math functions (extended)

    // carla_addFloat, carla_copyFloat & carla_zeroFloat tests skipped
    // mostly unused due to juce::FloatVectorOperations
}
#endif

#if 0
// --------------------------------------------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------------------------------------------

static void test_CarlaBridgeUtils() noexcept
{
    carla_stdout(PluginBridgeOscInfoType2str(kPluginBridgeOscPong));
    carla_stdout(PluginBridgeRtOpcode2str(kPluginBridgeRtNull));
    carla_stdout(PluginBridgeNonRtOpcode2str(kPluginBridgeNonRtNull));
}

// --------------------------------------------------------------------------------------------------------------------

static void test_CarlaEngineUtils() noexcept
{
    CARLA_BACKEND_USE_NAMESPACE
    carla_stdout(EngineType2Str(kEngineTypeNull));
    carla_stdout(EnginePortType2Str(kEnginePortTypeNull));
    carla_stdout(EngineEventType2Str(kEngineEventTypeNull));
    carla_stdout(EngineControlEventType2Str(kEngineControlEventTypeNull));
}
#endif

// --------------------------------------------------------------------------------------------------------------------
// main

int main()
{
    test_CarlaUtils();
    //test_CarlaMathUtils();

    //test_CarlaBackendUtils();
    //test_CarlaBridgeUtils();
    //test_CarlaEngineUtils();

    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
