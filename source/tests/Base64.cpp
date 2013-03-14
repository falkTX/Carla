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
 * For a full copy of the GNU General Public License see the GPL.txt file
 */

#include "CarlaBase64Utils.hpp"
//#include "CarlaString.hpp"

int main()
{
    // First check, cannot fail
    assert(std::strlen(kBase64) == 64);

    // Test regular C strings
    {
        const char strHelloWorld[] = "Hello World\n\0";
        const char b64HelloWorld[] = "SGVsbG8gV29ybGQK\0";

        // encode "Hello World" to base64
        size_t bufSize = std::strlen(strHelloWorld);
        char   bufEncoded[carla_base64_encoded_len(bufSize) + 1];
        carla_base64_encode((const uint8_t*)strHelloWorld, bufSize, bufEncoded);
        assert(std::strcmp(b64HelloWorld, bufEncoded) == 0);

        // decode base64 "SGVsbG8gV29ybGQK" back to "Hello World"
        uint8_t bufDecoded[carla_base64_decoded_max_len(b64HelloWorld)];
        size_t  bufDecSize = carla_base64_decode(b64HelloWorld, bufDecoded);
        char    strDecoded[bufDecSize+1];
        std::strncpy(strDecoded, (char*)bufDecoded, bufDecSize);
        strDecoded[bufDecSize] = '\0';
        assert(std::strcmp(strHelloWorld, strDecoded) == 0);
        assert(bufSize == bufDecSize);
    }

    struct Blob {
        char s[4];
        int i;
        double d;

        char padding[100];
        void* ptr;

        Blob()
            : s{'1', 's', 't', 0},
              i(228),
              d(3.33333333333),
              padding{0},
              ptr((void*)0x500) {}
    } blob;

#if 0
    // binary -> base64
    void* const test0 = &blob;
    size_t test0Len = sizeof(Blob);

    char buf0[carla_base64_encoded_len(test0Len) + 1];
    carla_base64_encode((const uint8_t*)test0, test0Len, buf0);

    printf("BINARY '%s'\n", buf0);

    char buf0dec[carla_base64_decoded_max_len(buf0)+1];
    carla_zeroMem(buf0dec, sizeof(buf0dec));
    carla_base64_decode(buf0, (uint8_t*)buf0dec);

    Blob blobTester;
    blobTester.s[0] = 0;
    blobTester.i = 0;
    blobTester.d = 9999.99999999999999;
    std::memcpy(&blobTester, buf0dec, sizeof(Blob));

    CARLA_ASSERT(std::strcmp(blobTester.s, "1st") == 0);
    CARLA_ASSERT(blobTester.i == 228);
    CARLA_ASSERT(blobTester.d == 3.33333333333);

    // string -> base64
    const char* const test1 = "Hello World!";
    size_t test1Len = std::strlen(test1);

    char buf1[carla_base64_encoded_len(test1Len) + 1];
    carla_base64_encode((const uint8_t*)test1, test1Len, buf1);

    printf("'%s' => '%s'\n", test1, buf1);

    // base64 -> string
    const char* const test2 = "SGVsbG8gV29ybGQh";

    char buf2[carla_base64_decoded_max_len(test2)+1];
    carla_zeroMem(buf2, sizeof(buf2));
    carla_base64_decode(test2, (uint8_t*)buf2);

    printf("'%s' => '%s'\n", test2, buf2);

    printf("'%s' == '%s'\n", buf1, test2);
    printf("'%s' == '%s'\n", buf2, test1);

    CARLA_ASSERT(std::strcmp(buf1, test2) == 0);
    CARLA_ASSERT(std::strcmp(buf2, test1) == 0);

    // -----------------------------------------------------------------

    blob.s[0] = 'X';
    blob.i = 92320;
    blob.d = 9999887.99999999999999;

    CarlaString string;
    string.importBinaryAsBase64<Blob>(&blob);

    Blob* blob3 = string.exportAsBase64Binary<Blob>();

    CARLA_ASSERT(std::strcmp(blob3->s, blob.s) == 0);
    CARLA_ASSERT(blob3->i == blob.i);
    CARLA_ASSERT(blob3->d == blob.d);
    CARLA_ASSERT(blob3->ptr == blob.ptr);

    delete blob3;
#endif

    // -----------------------------------------------------------------

    return 0;
}
