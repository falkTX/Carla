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

// size buf
const char* size_buf(const char* const buf);

// base types
const char* str_buf_string(const char* const string);
const char* str_buf_string_array(const char* const* const array);
const char* str_buf_uint(const uint value);

// json
char* json_buf_start();
char* json_buf_add_string(char* jsonBufPtr, const char* const key, const char* const value);
char* json_buf_add_uint(char* jsonBufPtr, const char* const key, const uint value);
const char* json_buf_end(char* jsonBufPtr);

#endif // REST_BUFFERS_HPP_INCLUDED
