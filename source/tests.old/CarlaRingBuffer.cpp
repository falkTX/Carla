/*
 * CarlaRingBuffer Tests
 * Copyright (C) 2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaRingBuffer.hpp"

// -----------------------------------------------------------------------
// simple types

template <class BufferStruct>
static void test_CarlaRingBuffer1(CarlaRingBuffer<BufferStruct>& b) noexcept
{
    // start empty
    assert(b.isEmpty());

    // write all types
    b.writeBool(true);
    b.writeByte(123);
    b.writeShort(1234);
    b.writeInt(99999123);
    b.writeLong(0xffffffff);
    b.writeFloat(0.0123f);
    b.writeDouble(0.0123);

    // should be considered empty until a commitWrite
    assert(b.isEmpty());

    // commit
    assert(b.commitWrite());

    // should have data now
    assert(! b.isEmpty());

    // read all types back
    assert(b.readBool() == true);
    assert(b.readByte() == 123);
    assert(b.readShort() == 1234);
    assert(b.readInt() == 99999123);
    assert(b.readLong() == 0xffffffff);
    assert(b.readFloat() == 0.0123f);

    // still has data
    assert(! b.isEmpty());

    assert(b.readDouble() == 0.0123);

    // now empty
    assert(b.isEmpty());
}

// -----------------------------------------------------------------------
// custom type

struct BufferTestStruct {
    BufferTestStruct()
        : b(false), i(255), l(9999) {}

    bool b;
    int32_t i;
    char _pad[999];
    int64_t l;

    bool operator==(const BufferTestStruct& s) const noexcept
    {
        return (b == s.b && i == s.i && l == s.l);
    }
};

template <class BufferStruct>
static void test_CarlaRingBuffer2(CarlaRingBuffer<BufferStruct>& b) noexcept
{
    // start empty
    assert(b.isEmpty());

    // write unmodified
    BufferTestStruct t1, t2;
    assert(t1 == t2);
    b.writeCustomType(t1);
    assert(b.commitWrite());

    // test read
    b.readCustomType(t1);
    assert(t1 == t2);

    // modify t1
    b.writeCustomType(t1);
    assert(b.commitWrite());
    carla_zeroStruct(t1);

    // test read
    b.readCustomType(t1);
    assert(t1 == t2);

    // now empty
    assert(b.isEmpty());
}

// -----------------------------------------------------------------------
// custom data

template <class BufferStruct>
static void test_CarlaRingBuffer3(CarlaRingBuffer<BufferStruct>& b) noexcept
{
    static const char* const kLicense = ""
    "This program is free software; you can redistribute it and/or\n"
    "modify it under the terms of the GNU General Public License as\n"
    "published by the Free Software Foundation; either version 2 of\n"
    "the License, or any later version.\n"
    "\n"
    "This program is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
    "GNU General Public License for more details.\n"
    "\n"
    "For a full copy of the GNU General Public License see the doc/GPL.txt file.\n"
    "\n";

    static const std::size_t kLicenseSize = std::strlen(kLicense);

    // start empty
    assert(b.isEmpty());

    // write big chunk
    b.writeCustomData(kLicense, kLicenseSize);

    // still empty
    assert(b.isEmpty());

    // commit
    assert(b.commitWrite());

    // not empty
    assert(! b.isEmpty());

    // read data
    char license[kLicenseSize+1];
    carla_zeroChars(license, kLicenseSize+1);
    b.readCustomData(license, kLicenseSize);

    // now empty again
    assert(b.isEmpty());

    // test data integrity
    assert(std::strcmp(license, kLicense) == 0);
}

// -----------------------------------------------------------------------

int main()
{
    CarlaHeapRingBuffer heap;
    CarlaStackRingBuffer stack;

    // small test first
    heap.createBuffer(4096);
    heap.deleteBuffer();
    heap.createBuffer(1);
    heap.deleteBuffer();

    // ok
    heap.createBuffer(1024);

    test_CarlaRingBuffer1(heap);
    test_CarlaRingBuffer1(stack);
    test_CarlaRingBuffer2(heap);
    test_CarlaRingBuffer2(stack);

    // test the big chunk a couple of times to ensure circular writing
    for (int i=0; i<20; ++i)
    {
        test_CarlaRingBuffer3(heap);
        test_CarlaRingBuffer3(stack);
    }

    return 0;
}

// -----------------------------------------------------------------------
