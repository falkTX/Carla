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

#ifndef REST_BUFFERS_HPP_INCLUDED
#define REST_BUFFERS_HPP_INCLUDED

#include "CarlaDefines.h"

#ifdef CARLA_PROPER_CPP11_SUPPORT
# include <cstdint>
#else
# include <stdint.h>
#endif

// size buf
const char* size_buf(const char* const buf);

// base types
const char* str_buf_bool(const bool value);
const char* str_buf_float(const double value);
const char* str_buf_float_array(const double* const values, const char sep = '\n');
const char* str_buf_string(const char* const string);
const char* str_buf_string_array(const char* const* const array);
const char* str_buf_string_quoted(const char* const string);
const char* str_buf_int(const int value);
const char* str_buf_int64(const int64_t value);
const char* str_buf_uint(const uint value);
const char* str_buf_uint64(const uint64_t value);
const char* str_buf_uint_array(const uint* const values, const char sep = '\n');

// json
char* json_buf_start();
char* json_buf_add_bool(char* jsonBufPtr, const char* const key, const bool value);
char* json_buf_add_float(char* jsonBufPtr, const char* const key, const double value);
char* json_buf_add_float_array(char* jsonBufPtr, const char* const key, const double* const values);
char* json_buf_add_string(char* jsonBufPtr, const char* const key, const char* const value);
char* json_buf_add_int(char* jsonBufPtr, const char* const key, const int value);
char* json_buf_add_int64(char* jsonBufPtr, const char* const key, const int64_t value);
char* json_buf_add_uint(char* jsonBufPtr, const char* const key, const uint value);
char* json_buf_add_uint64(char* jsonBufPtr, const char* const key, const uint64_t value);
char* json_buf_add_uint_array(char* jsonBufPtr, const char* const key, const uint* const values);
const char* json_buf_end(char* jsonBufPtr);

#endif // REST_BUFFERS_HPP_INCLUDED
