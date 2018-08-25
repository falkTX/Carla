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
#include "CarlaMathUtils.hpp"

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

const char* str_buf_bool(const bool value)
{
    strBuf[0] = value ? '1' : '0';
    strBuf[1] = '\0';
    return strBuf;
}

// -------------------------------------------------------------------------------------------------------------------

const char* str_buf_float(const double value)
{
    std::snprintf(strBuf, kStrBufSize, "%f", value);
    strBuf[kStrBufSize] = '\0';
    return strBuf;
}

const char* str_buf_float_array(const double* const values, const char sep)
{
    std::size_t bytesRead = 0;
    char tmpBuf[32];

    for (int i=0; carla_isNotZero(values[i]) && bytesRead < kStrBufSize; ++i)
    {
        std::snprintf(tmpBuf, 31, "%f", values[i]);
        tmpBuf[31] = '\0';

        const std::size_t size = std::strlen(tmpBuf);

        if (bytesRead + size > kStrBufSize)
            break;

        std::strncpy(strBuf+bytesRead, tmpBuf, kStrBufSize - bytesRead);
        bytesRead += size;
        strBuf[bytesRead] = sep;
        bytesRead += 1;
    }

    strBuf[bytesRead > 0 ? bytesRead-1 : 0] = '\0';
    return strBuf;
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
    std::size_t bytesRead = 0;

    for (int i=0; array[i] != nullptr && bytesRead < kStrBufSize; ++i)
    {
        const std::size_t size = std::strlen(array[i]);

        if (bytesRead + size > kStrBufSize)
            break;

        std::strncpy(strBuf+bytesRead, array[i], kStrBufSize - bytesRead);
        bytesRead += size;
        strBuf[bytesRead] = '\n';
        bytesRead += 1;
    }

    strBuf[bytesRead > 0 ? bytesRead-1 : 0] = '\0';
    return strBuf;
}

const char* str_buf_string_quoted(const char* const string)
{
    const std::size_t size = std::strlen(string);
    char* strBufPtr = strBuf;

    *strBufPtr++ = '"';

    for (std::size_t i=0, bytesWritten=0; i < size && bytesWritten < kStrBufSize-1; ++i)
    {
        switch (string[i])
        {
        case '"':
        case '\\':
            *strBufPtr++ = '\\';
            ++bytesWritten;
            break;
        case '\n':
            *strBufPtr++ = '\\';
            *strBufPtr++ = 'n';;
            bytesWritten += 2;
            continue;
        case '\f':
            *strBufPtr++ = '\\';
            *strBufPtr++ = 'f';;
            bytesWritten += 2;
            continue;
        }

        *strBufPtr++ = string[i];
        ++bytesWritten;
    }

    *strBufPtr++ = '"';
    *strBufPtr++ = '\0';
    return strBuf;
}

// -------------------------------------------------------------------------------------------------------------------

const char* str_buf_int(const int value)
{
    std::snprintf(strBuf, kStrBufSize, "%i", value);
    strBuf[kStrBufSize] = '\0';
    return strBuf;
}

const char* str_buf_int64(const int64_t value)
{
    std::snprintf(strBuf, kStrBufSize, P_INT64, value);
    strBuf[kStrBufSize] = '\0';
    return strBuf;
}

const char* str_buf_uint(const uint value)
{
    std::snprintf(strBuf, kStrBufSize, "%u", value);
    strBuf[kStrBufSize] = '\0';
    return strBuf;
}

const char* str_buf_uint64(const uint64_t value)
{
    std::snprintf(strBuf, kStrBufSize, P_UINT64, value);
    strBuf[kStrBufSize] = '\0';
    return strBuf;
}

const char* str_buf_uint_array(const uint* const values, const char sep)
{
    std::size_t bytesRead = 0;
    char tmpBuf[32];

    for (int i=0; values[i] != 0 && bytesRead < kStrBufSize; ++i)
    {
        std::snprintf(tmpBuf, 31, "%u", values[i]);
        tmpBuf[31] = '\0';

        const std::size_t size = std::strlen(tmpBuf);

        if (bytesRead + size > kStrBufSize)
            break;

        std::strncpy(strBuf+bytesRead, tmpBuf, kStrBufSize - bytesRead);
        bytesRead += size;
        strBuf[bytesRead] = sep;
        bytesRead += 1;
    }

    strBuf[bytesRead > 0 ? bytesRead-1 : 0] = '\0';
    return strBuf;
}

// -------------------------------------------------------------------------------------------------------------------

char* json_buf_start()
{
    std::strcpy(jsonBuf, "{");
    return jsonBuf + 1;
}

char* json_buf_add(char* jsonBufPtr, const char* const key, const char* const valueBuf)
{
    if (jsonBufPtr != jsonBuf+1)
        *jsonBufPtr++ = ',';

    *jsonBufPtr++ = '"';

    std::strcpy(jsonBufPtr, key);
    jsonBufPtr += std::strlen(key);

    *jsonBufPtr++ = '"';
    *jsonBufPtr++ = ':';

    std::strcpy(jsonBufPtr, valueBuf);
    jsonBufPtr += std::strlen(valueBuf);

    return jsonBufPtr;
}

template <typename T, typename Fn>
char* json_buf_add_fn(char* jsonBufPtr, const char* const key, const T value, const Fn fn)
{
    return json_buf_add(jsonBufPtr, key, fn(value));
}

template <typename T, typename Fn>
char* json_buf_add_fn_array(char* jsonBufPtr, const char* const key, const T value, const Fn fn)
{
    return json_buf_add(jsonBufPtr, key, fn(value, ','));
}

// -------------------------------------------------------------------------------------------------------------------

char* json_buf_add_bool(char* jsonBufPtr, const char* const key, const bool value)
{
    static const char* const kTrue  = "true";
    static const char* const kFalse = "false";
    return json_buf_add_fn(jsonBufPtr, key, value ? kTrue : kFalse, str_buf_string);
}

char* json_buf_add_float(char* jsonBufPtr, const char* const key, const double value)
{
    return json_buf_add_fn(jsonBufPtr, key, value, str_buf_float);
}

char* json_buf_add_float_array(char* jsonBufPtr, const char* const key, const double* const values)
{
    return json_buf_add_fn_array(jsonBufPtr, key, values, str_buf_float_array);
}

char* json_buf_add_string(char* jsonBufPtr, const char* const key, const char* const value)
{
    return json_buf_add_fn(jsonBufPtr, key, value, str_buf_string_quoted);
}

char* json_buf_add_int(char* jsonBufPtr, const char* const key, const int value)
{
    return json_buf_add_fn(jsonBufPtr, key, value, str_buf_int);
}

char* json_buf_add_int64(char* jsonBufPtr, const char* const key, const int64_t value)
{
    return json_buf_add_fn(jsonBufPtr, key, value, str_buf_int64);
}

char* json_buf_add_uint(char* jsonBufPtr, const char* const key, const uint value)
{
    return json_buf_add_fn(jsonBufPtr, key, value, str_buf_uint);
}

char* json_buf_add_uint_array(char* jsonBufPtr, const char* const key, const uint* const values)
{
    return json_buf_add_fn_array(jsonBufPtr, key, values, str_buf_uint_array);
}

char* json_buf_add_uint64(char* jsonBufPtr, const char* const key, const uint64_t value)
{
    return json_buf_add_fn(jsonBufPtr, key, value, str_buf_uint64);
}

const char* json_buf_end(char* jsonBufPtr)
{
    *jsonBufPtr++ = '}';
    *jsonBufPtr++ = '\0';
    return jsonBuf;
}

// -------------------------------------------------------------------------------------------------------------------
