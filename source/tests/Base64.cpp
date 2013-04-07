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
#include "CarlaString.hpp"

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
        void* ptr;
        char pad[17];
        int32_t i32;
        int64_t i64;
        bool b;
    };
    Blob blob;

    const size_t blobSize = sizeof(Blob);
    carla_zeroStruct<Blob>(blob);

    blob.s[0] = '1';
    blob.s[1] = 's';
    blob.s[2] = 't';
    blob.i    = 228;
    blob.d    = 3.33333333333;
    blob.ptr  = (void*)0x500;
    blob.i32  = 32;
    blob.i64  = 64;
    blob.b    = true;
    blob.pad[11] = 71;

    // binary -> base64
    char blobEnc[carla_base64_encoded_len(blobSize) + 1];
    carla_base64_encode((const uint8_t*)&blob, blobSize, blobEnc);
    std::printf("BINARY '%s', size:" P_SIZE "\n", blobEnc, blobSize);

    {
        // base64 -> binary
        uint8_t blobDec[carla_base64_decoded_max_len(blobEnc)];
        carla_base64_decode(blobEnc, blobDec);

        Blob blobTest;
        carla_zeroStruct<Blob>(blobTest);

        std::memcpy(&blobTest, blobDec, sizeof(Blob));
        assert(blobTest.s[0] == '1');
        assert(blobTest.s[1] == 's');
        assert(blobTest.s[2] == 't');
        assert(blobTest.i    == 228);
        assert(blobTest.d    == 3.33333333333);
        assert(blobTest.ptr  == (void*)0x500);
        assert(blobTest.i32  == 32);
        assert(blobTest.i64  == 64);
        assert(blobTest.b    == true);
        assert(blobTest.pad[11] == 71);
    }

    {
        // CarlaString usage
        CarlaString string;
        string.importBinaryAsBase64<Blob>(&blob);
        assert(std::strcmp(blobEnc, (const char*)string) == 0);
        assert(std::strlen(blobEnc) == string.length());

        Blob* blobNew = string.exportAsBase64Binary<Blob>();

        assert(blobNew->s[0] == '1');
        assert(blobNew->s[1] == 's');
        assert(blobNew->s[2] == 't');
        assert(blobNew->i    == 228);
        assert(blobNew->d    == 3.33333333333);
        assert(blobNew->ptr  == (void*)0x500);
        assert(blobNew->i32  == 32);
        assert(blobNew->i64  == 64);
        assert(blobNew->b    == true);
        assert(blobNew->pad[11] == 71);

        delete blobNew;
    }

    return 0;
}
