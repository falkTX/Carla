// Copyright 2021 Jean Pierre Cimalando
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once
#include "ysfx.h"
#include <string>
#include <vector>
#include <mutex>
#include <cstdio>
#include <cstdint>
#include <clocale>
#if defined(__APPLE__)
#   include <xlocale.h>
#endif
#if !defined(_WIN32)
#   include <alloca.h>
#else
#   include <malloc.h>
#endif

#if defined(YSFX_NO_STANDARD_MUTEX)
#   include "WDL/mutex.h"
#endif

namespace ysfx {

YSFX_DEFINE_AUTO_PTR(FILE_u, FILE, fclose);

FILE *fopen_utf8(const char *path, const char *mode);
int64_t fseek_lfs(FILE *stream, int64_t off, int whence);
int64_t ftell_lfs(FILE *stream);

//------------------------------------------------------------------------------

#if !defined(_WIN32)
using c_locale_t = locale_t;
#else
using c_locale_t = _locale_t;
#endif

c_locale_t c_numeric_locale();

//------------------------------------------------------------------------------

#if !defined(_WIN32)
#   define ysfx_alloca(n) alloca((n))
#else
#   define ysfx_alloca(n) _malloca((n))
#endif

//------------------------------------------------------------------------------

#if !defined(YSFX_NO_STANDARD_MUTEX)
using mutex = std::mutex;
#else
class mutex
{
public:
    void lock() { m_mutex.Enter(); }
    void unlock() { m_mutex.Leave(); }

private:
    WDL_Mutex m_mutex;
};
#endif

//------------------------------------------------------------------------------

using string_list = std::vector<std::string>;

double c_atof(const char *text, c_locale_t loc);
double c_strtod(const char *text, char **endp, c_locale_t loc);
double dot_atof(const char *text);
double dot_strtod(const char *text, char **endp);
bool ascii_isspace(char c);
bool ascii_isalpha(char c);
char ascii_tolower(char c);
char ascii_toupper(char c);
int ascii_casecmp(const char *a, const char *b);
uint32_t latin1_toupper(uint32_t c);
uint32_t latin1_tolower(uint32_t c);
char *strdup_using_new(const char *src);
string_list split_strings_noempty(const char *input, bool(*pred)(char));
std::string trim(const char *input, bool(*pred)(char));

//------------------------------------------------------------------------------

void pack_u32le(uint32_t value, uint8_t data[4]);
void pack_f32le(float value, uint8_t data[4]);
uint32_t unpack_u32le(const uint8_t data[4]);
float unpack_f32le(const uint8_t data[4]);

//------------------------------------------------------------------------------

std::vector<uint8_t> decode_base64(const char *text, size_t len = ~(size_t)0);
std::string encode_base64(const uint8_t *data, size_t len);

//------------------------------------------------------------------------------

using file_uid = std::pair<uint64_t, uint64_t>;
bool get_file_uid(const char *path, file_uid &uid);
bool get_stream_file_uid(FILE *stream, file_uid &uid);
bool get_descriptor_file_uid(int fd, file_uid &uid);
#if defined(_WIN32)
bool get_handle_file_uid(void *handle, file_uid &uid);
#endif

//------------------------------------------------------------------------------

struct split_path_t {
    std::string drive;
    std::string dir;
    std::string file;
};

// check if the character is a path separator
bool is_path_separator(char ch);
// break down a path into individual components
split_path_t split_path(const char *path);
// get the file name part (all after the final '/' separator)
std::string path_file_name(const char *path);
// get the directory part (all up to the '/' separator, inclusive)
std::string path_directory(const char *path);
// add the final '/' separator if absent; if empty, does nothing
std::string path_ensure_final_separator(const char *path);
// compare the tail of the path with the suffix, case-insensitively
bool path_has_suffix(const char *path, const char *suffix);
// check whether the path is relative
bool path_is_relative(const char *path);

//------------------------------------------------------------------------------

// check whether a file exists on disk
bool exists(const char *path);
// list the elements of a directory; directories are distinguished with a final '/'
string_list list_directory(const char *path);
// visit the root and subdirectories in depth-first order
void visit_directories(const char *rootpath, bool (*visit)(const std::string &, void *), void *data);
// resolve a path which matches root/fragment, where fragment is case-insensitive (0=failed, 1=exact, 2=inexact)
int case_resolve(const char *root, const char *fragment, std::string &result);

//------------------------------------------------------------------------------

#if defined(_WIN32)
std::wstring widen(const std::string &u8str);
std::wstring widen(const char *u8data, size_t u8len = ~(size_t)0);
std::string narrow(const std::wstring &wstr);
std::string narrow(const wchar_t *wdata, size_t wlen = ~(size_t)0);
#endif

//------------------------------------------------------------------------------

template <class F>
class scope_guard {
public:
    explicit scope_guard(F &&f) : f(std::forward<F>(f)), a(true) {}
    scope_guard(scope_guard &&o) : f(std::move(o.f)), a(o.a) { o.a = false; }
    ~scope_guard() { if (a) f(); }
    void disarm() { a = false; }
private:
    F f;
    bool a;
    scope_guard(const scope_guard &) = delete;
    scope_guard &operator=(const scope_guard &) = delete;
};

template <class F> scope_guard<F> defer(F &&f)
{
    return scope_guard<F>(std::forward<F>(f));
}

} // namespace ysfx
