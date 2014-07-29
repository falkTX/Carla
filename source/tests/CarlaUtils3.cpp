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

#include "CarlaJuceUtils.hpp"
#include "CarlaLibCounter.hpp"
#include "CarlaOscUtils.hpp"
#include "CarlaPatchbayUtils.hpp"
#include "CarlaShmUtils.hpp"

// -----------------------------------------------------------------------

class LeakTestClass
{
public:
    LeakTestClass() noexcept
        : i(0) {}

private:
    int i;
    CARLA_LEAK_DETECTOR(LeakTestClass)
};

static void test_CarlaJuceUtils()
{
    LeakTestClass a, b;
    LeakTestClass* e;
    LeakTestClass* f = nullptr;

    e = new LeakTestClass;
    f = new LeakTestClass;
    delete e; delete f;
    delete new LeakTestClass;

    int x = 1;

    {
        assert(x == 1);
        ScopedValueSetter<int> s(x, 2);
        assert(x == 2);
    }

    assert(x == 1);

    {
        assert(x == 1);
        ScopedValueSetter<int> s(x, 3, 4);
        assert(x == 3);
    }

    assert(x == 4);
}

// -----------------------------------------------------------------------

static void test_CarlaLibUtils() noexcept
{
    void* const libNot = lib_open("/libzzzzz...");
    assert(libNot == nullptr);
    carla_stdout("Force lib_open fail error results in: %s", lib_error("/libzzzzz..."));

    void* const lib = lib_open("/usr/lib/liblo.so");
    CARLA_SAFE_ASSERT_RETURN(lib != nullptr,);

    void* const libS = lib_symbol(lib, "lo_server_new");
    CARLA_SAFE_ASSERT(libS != nullptr);

    const bool closed = lib_close(lib);
    CARLA_SAFE_ASSERT(closed);

    LibCounter lc;
    void* const test1 = lc.open("/usr/lib/liblo.so");
    void* const test2 = lc.open("/usr/lib/liblo.so");
    void* const test3 = lc.open("/usr/lib/liblo.so");
    assert(test1 == test2);
    assert(test2 == test3);
    lc.close(test1); lc.close(test2); lc.close(test3);

    // test if the pointer changes after all closed
    void* const test1b = lc.open("/usr/lib/liblo.so");
    assert(test1 != test1b);
    lc.close(test1b);

    // test non-delete flag
    void* const test4 = lc.open("/usr/lib/liblrdf.so.0", false);
    lc.close(test4);
    void* const test5 = lc.open("/usr/lib/liblrdf.so.0");
    assert(test4 == test5);
    lc.close(test5);

    // open non-delete a few times, tests for cleanup on destruction
    lc.open("/usr/lib/liblrdf.so.0");
    lc.open("/usr/lib/liblrdf.so.0");
    lc.open("/usr/lib/liblrdf.so.0");
}

// -----------------------------------------------------------------------

static void test_CarlaOscUtils() noexcept
{
}

// -----------------------------------------------------------------------

static void test_CarlaPatchbayUtils() noexcept
{
    GroupNameToId g1, g2, g3;

    // only clear #1
    g1.clear();

    // set initial data
    g1.setData(1, "1");
    g2.setData(2, "2");
    g3.setData(3, "3");

    // should not match
    assert(g1 != g2);
    assert(g1 != g3);

    // set data equal to #1, test match
    g3.setData(1, "0");
    g3.rename("1");
    assert(g1 == g3);

    // set data back
    g3.setData(3, "3");

    PatchbayGroupList glist;
    glist.list.append(g1); ++glist.lastId;
    glist.list.append(g3); ++glist.lastId;
    glist.list.append(g2); ++glist.lastId;
    assert(glist.getGroupId("1") == 1);
    assert(glist.getGroupId("2") == 2);
    assert(glist.getGroupId("3") == 3);
    assert(std::strcmp(glist.getGroupName(1), "1") == 0);
    assert(std::strcmp(glist.getGroupName(2), "2") == 0);
    assert(std::strcmp(glist.getGroupName(3), "3") == 0);
    glist.clear();

    PortNameToId p11, p12, p21, p31;

    // only clear #11
    p11.clear();

    // set initial data
    p11.setData(1, 1, "1", "1:1");
    p12.setData(1, 2, "2", "1:2");
    p21.setData(2, 1, "1", "2:1");
    p31.setData(3, 1, "1", "3:1");

    // should not match
    assert(p11 != p12);
    assert(p11 != p21);
    assert(p11 != p31);

    // set data equal to #1, test match
    p31.setData(1, 2, "0", "0:0");
    p31.rename("2", "1:2");
    assert(p12 == p31);

    // set data back
    p31.setData(3, 1, "1", "3:1");

    PatchbayPortList plist;
    plist.list.append(p11); ++plist.lastId;
    plist.list.append(p12); ++plist.lastId;
    plist.list.append(p21); ++plist.lastId;
    plist.list.append(p31); ++plist.lastId;
    assert(std::strcmp(plist.getFullPortName(1, 1), "1:1") == 0);
    assert(std::strcmp(plist.getFullPortName(1, 2), "1:2") == 0);
    assert(std::strcmp(plist.getFullPortName(2, 1), "2:1") == 0);
    assert(std::strcmp(plist.getFullPortName(3, 1), "3:1") == 0);
    assert(p11 == plist.getPortNameToId("1:1"));
    assert(p12 == plist.getPortNameToId("1:2"));
    assert(p21 == plist.getPortNameToId("2:1"));
    assert(p31 == plist.getPortNameToId("3:1"));
    plist.clear();

    // no tests here, just usage
    ConnectionToId c1, c2;
    c1.clear();
    c2.setData(0, 0, 0, 0, 0);
    assert(c1 == c2);
    c2.setData(0, 0, 0, 0, 1);
    assert(c1 != c2);

    PatchbayConnectionList clist;
    clist.list.append(c1); ++clist.lastId;
    clist.list.append(c2); ++clist.lastId;
    clist.clear();
}

// -----------------------------------------------------------------------

struct ShmStruct {
    char stringStart[255];
    bool boolean;
    int integer;
    float floating;
    char stringEnd[255];
};

static void test_CarlaShmUtils() noexcept
{
    shm_t shm, shma;
    ShmStruct* shmStruct1;
    ShmStruct* shmStruct2;

    // base tests first
    carla_shm_init(shm);
    assert(! carla_is_shm_valid(shm));

    shm = carla_shm_create("/carla-shm-test1");
    carla_stdout("test %i", shm);
    assert(carla_is_shm_valid(shm));

    carla_shm_close(shm);
    assert(! carla_is_shm_valid(shm));

    shm = carla_shm_create("/carla-shm-test1");
    assert(carla_is_shm_valid(shm));

    shma = carla_shm_attach("/carla-shm-test1");
    assert(carla_is_shm_valid(shma));

    carla_shm_close(shm);
    carla_shm_close(shma);
    assert(! carla_is_shm_valid(shm));
    assert(! carla_is_shm_valid(shma));

    // test attach invalid
    shma = carla_shm_attach("/carla-shm-test-NOT");
    assert(! carla_is_shm_valid(shma));

    // test memory, start
    shm = carla_shm_create("/carla-shm-test1");
    assert(carla_is_shm_valid(shm));

    shma = carla_shm_attach("/carla-shm-test1");
    assert(carla_is_shm_valid(shma));

    // test memory, check valid
    shmStruct1 = carla_shm_map<ShmStruct>(shm);
    assert(shmStruct1 != nullptr);

    shmStruct2 = carla_shm_map<ShmStruct>(shma);
    assert(shmStruct2 != nullptr);

    carla_shm_unmap(shma, shmStruct2);
    assert(shmStruct2 == nullptr);

    carla_shm_unmap(shm, shmStruct1);
    assert(shmStruct1 == nullptr);

    // test memory, check if write data matches
    shmStruct1 = carla_shm_map<ShmStruct>(shm);
    assert(shmStruct1 != nullptr);

    shmStruct2 = carla_shm_map<ShmStruct>(shma);
    assert(shmStruct2 != nullptr);

    carla_zeroStruct(*shmStruct1);
    assert(shmStruct1->stringStart[0] == '\0');
    assert(shmStruct2->stringStart[0] == '\0');
    assert(shmStruct1->stringEnd[0] == '\0');
    assert(shmStruct2->stringEnd[0] == '\0');
    assert(! shmStruct1->boolean);
    assert(! shmStruct2->boolean);

    shmStruct1->boolean = true;
    shmStruct1->integer = 232312;
    assert(shmStruct1->boolean == shmStruct2->boolean);
    assert(shmStruct1->integer == shmStruct2->integer);

    shmStruct2->floating = 2342.231f;
    std::strcpy(shmStruct2->stringStart, "test1start");
    std::strcpy(shmStruct2->stringEnd, "test2end");
    assert(shmStruct1->floating == shmStruct2->floating);
    assert(std::strcmp(shmStruct1->stringStart, "test1start") == 0);
    assert(std::strcmp(shmStruct1->stringStart, shmStruct2->stringStart) == 0);
    assert(std::strcmp(shmStruct1->stringEnd, "test2end") == 0);
    assert(std::strcmp(shmStruct1->stringEnd, shmStruct2->stringEnd) == 0);

    carla_shm_unmap(shma, shmStruct2);
    assert(shmStruct2 == nullptr);

    carla_shm_unmap(shm, shmStruct1);
    assert(shmStruct1 == nullptr);

    // test memory, done
    carla_shm_close(shm);
    carla_shm_close(shma);
    assert(! carla_is_shm_valid(shm));
    assert(! carla_is_shm_valid(shma));
}

// -----------------------------------------------------------------------
// main

int main()
{
    test_CarlaJuceUtils();
    test_CarlaLibUtils();
    test_CarlaOscUtils();
    test_CarlaPatchbayUtils();
    test_CarlaShmUtils();

    return 0;
}

// -----------------------------------------------------------------------
