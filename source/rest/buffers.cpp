/*
 * Carla REST API Server
 * Copyright (C) 2018 Filipe Coelho <falktx@falktx.com>
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

#include "buffers.hpp"

#include <cstdio>
#include <cstring>

// -------------------------------------------------------------------------------------------------------------------

enum {
    kJsonBufSize = 4095,
    kStrBufSize = 1023,
    kSizeBufSize = 31,
};

// static buffer to return json
// NOTE size is never checked for json, the buffer is big enough in order to assume it all always fits
static char jsonBuf[kJsonBufSize+1];

// static buffer to return size
static char sizeBuf[kSizeBufSize+1];

// static buffer to return regular strings
static char strBuf[kStrBufSize+1];

// -------------------------------------------------------------------------------------------------------------------

const char* size_buf(const char* const buf)
{
    const std::size_t size = std::strlen(buf);
    std::snprintf(sizeBuf, kSizeBufSize, P_SIZE, size);
    sizeBuf[kSizeBufSize] = '\0';
    return sizeBuf;
}

// -------------------------------------------------------------------------------------------------------------------

const char* str_buf_string(const char* const string)
{
    std::strncpy(strBuf, string, kStrBufSize);
    strBuf[kStrBufSize] = '\0';
    return strBuf;
}

const char* str_buf_string_array(const char* const* const array)
{
    size_t bytesRead = 0;

    for (int i=0; array[i] != nullptr && bytesRead < kStrBufSize; ++i)
    {
        const std::size_t size = std::strlen(array[i]);

        if (bytesRead + size > kStrBufSize)
            break;

        std::strncpy(strBuf+bytesRead, array[i], kStrBufSize);
        bytesRead += size;
        strBuf[bytesRead] = '\n';
        bytesRead += 1;
    }

    strBuf[bytesRead] = '\0';
    return strBuf;
}

const char* str_buf_uint(const uint value)
{
    std::snprintf(strBuf, kStrBufSize, "%u", value);
    strBuf[kStrBufSize] = '\0';
    return strBuf;
}

// -------------------------------------------------------------------------------------------------------------------

char* json_buf_start()
{
    std::strcpy(jsonBuf, "{");
    return jsonBuf + 1;
}

template <typename T, typename Fn>
char* json_buf_add(char* jsonBufPtr, const char* const key, const T value, const Fn fn)
{
    fn(value);

    if (jsonBufPtr != jsonBuf+1)
    {
        *jsonBufPtr = ',';
        jsonBufPtr += 1;
    }

    *jsonBufPtr++ = '"';

    std::strcpy(jsonBufPtr, key);
    jsonBufPtr += std::strlen(key);

    *jsonBufPtr++ = '"';
    *jsonBufPtr++ = ':';

    std::strcpy(jsonBufPtr, strBuf);
    jsonBufPtr += std::strlen(strBuf);

    return jsonBufPtr;
}

char* json_buf_add_string(char* jsonBufPtr, const char* const key, const char* const value)
{
    return json_buf_add(jsonBufPtr, key, value, str_buf_string);
}

char* json_buf_add_uint(char* jsonBufPtr, const char* const key, const uint value)
{
    return json_buf_add(jsonBufPtr, key, value, str_buf_uint);
}

const char* json_buf_end(char* jsonBufPtr)
{
    *jsonBufPtr++ = '}';
    *jsonBufPtr++ = '\0';
    return jsonBuf;
}

// -------------------------------------------------------------------------------------------------------------------
