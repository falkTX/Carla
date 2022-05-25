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

#include "ysfx_utils.hpp"
#include "base64/Base64.hpp"
#include <system_error>
#include <algorithm>
#include <deque>
#include <clocale>
#include <cstring>
#include <cassert>
#if !defined(_WIN32)
#   include <sys/stat.h>
#   include <sys/types.h>
#   include <unistd.h>
#   include <dirent.h>
#   include <fcntl.h>
#else
#   include <windows.h>
#   include <io.h>
#endif

namespace ysfx {

#if !defined(_WIN32)
static_assert(sizeof(off_t) == 8, "64-bit large file support is not enabled");
#endif

FILE *fopen_utf8(const char *path, const char *mode)
{
#if defined(_WIN32)
    return _wfopen(widen(path).c_str(), widen(mode).c_str());
#else
    return fopen(path, mode);
#endif
}

int64_t fseek_lfs(FILE *stream, int64_t off, int whence)
{
#if defined(_WIN32)
    return _fseeki64(stream, off, whence);
#else
    return fseeko(stream, off, whence);
#endif
}

int64_t ftell_lfs(FILE *stream)
{
#if defined(_WIN32)
    return _ftelli64(stream);
#else
    return ftello(stream);
#endif
}

//------------------------------------------------------------------------------

namespace {

struct scoped_c_locale
{
    scoped_c_locale(int lc, const char *name);
    ~scoped_c_locale();
    c_locale_t m_loc{};

    scoped_c_locale(const scoped_c_locale &) = delete;
    scoped_c_locale &operator=(const scoped_c_locale &) = delete;
};

scoped_c_locale::scoped_c_locale(int lc, const char *name)
{
#if defined(_WIN32)
    m_loc = _create_locale(lc, name);
#else
    switch (lc) {
    case LC_ALL:
        m_loc = newlocale(LC_ALL_MASK, name, c_locale_t{});
        break;
    case LC_CTYPE:
        m_loc = newlocale(LC_CTYPE_MASK, name, c_locale_t{});
        break;
    case LC_COLLATE:
        m_loc = newlocale(LC_COLLATE_MASK, name, c_locale_t{});
        break;
    case LC_MONETARY:
        m_loc = newlocale(LC_MONETARY_MASK, name, c_locale_t{});
        break;
    case LC_NUMERIC:
        m_loc = newlocale(LC_NUMERIC_MASK, name, c_locale_t{});
        break;
    case LC_TIME:
        m_loc = newlocale(LC_TIME_MASK, name, c_locale_t{});
        break;
    case LC_MESSAGES:
        m_loc = newlocale(LC_MESSAGES_MASK, name, c_locale_t{});
        break;
    default:
        errno = EINVAL;
        break;
    }
#endif
    if (!m_loc)
        throw std::system_error(errno, std::generic_category());
}

scoped_c_locale::~scoped_c_locale()
{
    if (!m_loc) return;
#if !defined(_WIN32)
    freelocale(m_loc);
#else
    _free_locale(m_loc);
#endif
}

#if !defined(_WIN32)
struct scoped_posix_uselocale {
    explicit scoped_posix_uselocale(c_locale_t loc);
    ~scoped_posix_uselocale();

    c_locale_t m_loc{};
    c_locale_t m_old{};

    scoped_posix_uselocale(const scoped_posix_uselocale &) = delete;
    scoped_posix_uselocale &operator=(const scoped_posix_uselocale &) = delete;
};

scoped_posix_uselocale::scoped_posix_uselocale(c_locale_t loc)
{
    if (loc)
    {
        m_loc = loc;
        m_old = uselocale(loc);
    }
}

scoped_posix_uselocale::~scoped_posix_uselocale()
{
    if (m_loc)
        uselocale(m_old);
}
#endif

} // namespace

//------------------------------------------------------------------------------

c_locale_t c_numeric_locale()
{
    static scoped_c_locale loc(LC_NUMERIC, "C");
    return loc.m_loc;
}

//------------------------------------------------------------------------------

double c_atof(const char *text, c_locale_t loc)
{
#if defined(_WIN32)
    return _atof_l(text, loc);
#else
    scoped_posix_uselocale use(loc);
    return atof(text);
#endif
}

double c_strtod(const char *text, char **endp, c_locale_t loc)
{
#if defined(_WIN32)
    return _strtod_l(text, endp, loc);
#else
    scoped_posix_uselocale use(loc);
    return strtod(text, endp);
#endif
}

double dot_atof(const char *text)
{
    return c_atof(text, c_numeric_locale());
}

double dot_strtod(const char *text, char **endp)
{
    return c_strtod(text, endp, c_numeric_locale());
}

bool ascii_isspace(char c)
{
    switch (c) {
    case ' ': case '\f': case '\n': case '\r': case '\t': case '\v':
        return true;
    default:
        return false;
    }
}

bool ascii_isalpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

char ascii_tolower(char c)
{
    return (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
}

char ascii_toupper(char c)
{
    return (c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c;
}

int ascii_casecmp(const char *a, const char *b)
{
    for (char ca, cb; (ca = *a++) | (cb = *b++); ) {
        ca = ascii_tolower(ca);
        cb = ascii_tolower(cb);
        if (ca < cb) return -1;
        if (ca > cb) return +1;
    }
    return 0;
}

uint32_t latin1_toupper(uint32_t c)
{
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 'A';
    if ((c >= 0xe0 && c <= 0xf6) || (c >= 0xf8 && c <= 0xfe))
        return c - 0x20;
    return c;
}

uint32_t latin1_tolower(uint32_t c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 'a';
    if ((c >= 0xc0 && c <= 0xd6) || (c >= 0xd8 && c <= 0xde))
        return c + 0x20;
    return c;
}

char *strdup_using_new(const char *src)
{
    size_t len = strlen(src);
    char *dst = new char[len + 1];
    memcpy(dst, src, len + 1);
    return dst;
}

string_list split_strings_noempty(const char *input, bool(*pred)(char))
{
    string_list list;

    if (input) {
        std::string acc;
        acc.reserve(256);

        for (char c; (c = *input++) != '\0'; ) {
            if (!pred(c))
                acc.push_back(c);
            else {
                if (!acc.empty()) {
                    list.push_back(acc);
                    acc.clear();
                }
            }
        }

        if (!acc.empty())
            list.push_back(acc);
    }

    return list;
}

std::string trim(const char *input, bool(*pred)(char))
{
    const char *start = input;
    while (*start != '\0' && pred(*start))
        ++start;

    const char *end = start + strlen(start);
    while (end > start && pred(*(end - 1)))
        --end;

    return std::string(start, end);
}

//------------------------------------------------------------------------------

void pack_u32le(uint32_t value, uint8_t data[4])
{
    data[0] = value & 0xff;
    data[1] = (value >> 8) & 0xff;
    data[2] = (value >> 16) & 0xff;
    data[3] = value >> 24;
}

void pack_f32le(float value, uint8_t data[4])
{
    uint32_t u;
    memcpy(&u, &value, 4);
    pack_u32le(u, data);
}

uint32_t unpack_u32le(const uint8_t data[4])
{
    return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

float unpack_f32le(const uint8_t data[4])
{
    float value;
    uint32_t u = unpack_u32le(data);
    memcpy(&value, &u, 4);
    return value;
}

//------------------------------------------------------------------------------

std::vector<uint8_t> decode_base64(const char *text, size_t len)
{
    return d_getChunkFromBase64String(text, len);
}

//------------------------------------------------------------------------------

bool get_file_uid(const char *path, file_uid &uid)
{
#ifdef _WIN32
    HANDLE handle = CreateFileW(widen(path).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
        return false;
    bool success = get_handle_file_uid((void *)handle, uid);
    CloseHandle(handle);
    return success;
#else
    int fd = open(path, O_RDONLY);
    if (fd == -1)
        return false;
    bool success = get_descriptor_file_uid(fd, uid);
    close(fd);
    return success;
#endif
}

bool get_stream_file_uid(FILE *stream, file_uid &uid)
{
#if !defined(_WIN32)
    int fd = fileno(stream);
    if (fd == -1)
        return false;
#else
    int fd = _fileno(stream);
    if (fd == -1)
        return false;
#endif
    return get_descriptor_file_uid(fd, uid);
}

bool get_descriptor_file_uid(int fd, file_uid &uid)
{
#if !defined(_WIN32)
    struct stat st;
    if (fstat(fd, &st) != 0)
        return false;
    uid.first = (uint64_t)st.st_dev;
    uid.second = (uint64_t)st.st_ino;
    return true;
#else
    HANDLE handle = (HANDLE)_get_osfhandle(fd);
    if (handle == INVALID_HANDLE_VALUE)
        return false;
    return get_handle_file_uid((void *)handle, uid);
#endif
}

#if defined(_WIN32)
bool get_handle_file_uid(void *handle, file_uid &uid)
{
    BY_HANDLE_FILE_INFORMATION info;
    if (!GetFileInformationByHandle((HANDLE)handle, &info))
        return false;
    uid.first = info.dwVolumeSerialNumber;
    uid.second = (uint64_t)info.nFileIndexLow | ((uint64_t)info.nFileIndexHigh << 32);
    return true;
}
#endif

//------------------------------------------------------------------------------

bool is_path_separator(char ch)
{
#if !defined(_WIN32)
    return ch == '/';
#else
    return ch == '/' || ch == '\\';
#endif
}

split_path_t split_path(const char *path)
{
    split_path_t sp;
#if !defined(_WIN32)
    size_t npos = ~(size_t)0;
    size_t pos = npos;
    for (size_t i = 0; path[i] != '\0'; ++i) {
        if (is_path_separator(path[i]))
            pos = i;
    }
    if (pos == npos)
        sp.file.assign(path);
    else {
        sp.dir.assign(path, pos + 1);
        sp.file.assign(path + pos + 1);
    }
#else
    std::wstring wpath = widen(path);
    std::unique_ptr<wchar_t[]> drive{new wchar_t[wpath.size() + 1]{}};
    std::unique_ptr<wchar_t[]> dir{new wchar_t[wpath.size() + 1]{}};
    std::unique_ptr<wchar_t[]> fname{new wchar_t[wpath.size() + 1]{}};
    std::unique_ptr<wchar_t[]> ext{new wchar_t[wpath.size() + 1]{}};
    _wsplitpath(wpath.c_str(), drive.get(), dir.get(), fname.get(), ext.get());
    sp.drive = narrow(drive.get());
    if (!drive[0] || dir[0] == L'/' || dir[0] == L'\\')
        sp.dir = narrow(dir.get());
    else
        sp.dir = narrow(L'\\' + std::wstring(dir.get()));
    sp.file = narrow(std::wstring(fname.get()) + std::wstring(ext.get()));
#endif
    return sp;
}

std::string path_file_name(const char *path)
{
    return split_path(path).file;
}

std::string path_directory(const char *path)
{
    split_path_t sp = split_path(path);
    return sp.dir.empty() ? std::string("./") : (sp.drive + sp.dir);
}

std::string path_ensure_final_separator(const char *path)
{
    std::string result(path);

    if (!result.empty() && !is_path_separator(result.back()))
        result.push_back('/');

    return result;
}

bool path_has_suffix(const char *path, const char *suffix)
{
    if (*suffix == '.')
        ++suffix;

    size_t plen = strlen(path);
    size_t slen = strlen(suffix);
    if (plen < slen + 2)
        return false;

    return path[plen - slen - 1] == '.' &&
        ascii_casecmp(suffix, &path[plen - slen]) == 0;
}

bool path_is_relative(const char *path)
{
#if !defined(_WIN32)
    return !is_path_separator(path[0]);
#else
    return !is_path_separator(split_path(path).dir.c_str()[0]);
#endif
}

//------------------------------------------------------------------------------

#if !defined(_WIN32)
bool exists(const char *path)
{
    return access(path, F_OK) == 0;
}

string_list list_directory(const char *path)
{
    string_list list;

    DIR *dir = opendir(path);
    if (!dir)
        return list;
    auto dir_cleanup = defer([dir]() { closedir(dir); });

    list.reserve(256);

    std::string pathbuf;
    pathbuf.reserve(1024);

    while (dirent *ent = readdir(dir)) {
        const char *name = ent->d_name;
        if (!strcmp(name, ".") || !strcmp(name, ".."))
            continue;

        pathbuf.assign(name);
        if (ent->d_type == DT_DIR)
            pathbuf.push_back('/');

        list.push_back(pathbuf);
    }

    std::sort(list.begin(), list.end());
    return list;
}

// void visit_directories(const char *rootpath, bool (*visit)(const std::string &, void *), void *data);
// NOTE: implemented in separate file `ysfx_utils_fts.cpp`
#else
bool exists(const char *path)
{
    return _waccess(widen(path).c_str(), 0) == 0;
}

string_list list_directory(const char *path)
{
    string_list list;

    std::wstring pattern = widen(path) + L"\\*";

    WIN32_FIND_DATAW fd;
    HANDLE handle = FindFirstFileW(pattern.c_str(), &fd);
    if (handle == INVALID_HANDLE_VALUE)
        return list;
    auto handle_cleanup = defer([handle]() { FindClose(handle); });

    list.reserve(256);

    do {
        const wchar_t *name = fd.cFileName;
        if (!wcscmp(name, L".") || !wcscmp(name, L".."))
            continue;

        std::string entry = narrow(name);
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !(fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
            entry.push_back('/');

        list.push_back(std::move(entry));
    } while (FindNextFileW(handle, &fd));

    std::sort(list.begin(), list.end());
    return list;
}

void visit_directories(const char *rootpath, bool (*visit)(const std::string &, void *), void *data)
{
    std::deque<std::wstring> dirs;
    dirs.push_back(widen(path_ensure_final_separator(rootpath)));

    std::wstring pathbuf;
    pathbuf.reserve(1024);

    std::vector<std::wstring> entries;
    entries.reserve(256);

    while (!dirs.empty()) {
        std::wstring dir = std::move(dirs.front());
        dirs.pop_front();

        if (!visit(narrow(dir), data))
            return;

        pathbuf.assign(dir);
        pathbuf.append(L"\\*");

        WIN32_FIND_DATAW fd;
        HANDLE handle = FindFirstFileW(pathbuf.c_str(), &fd);
        if (handle == INVALID_HANDLE_VALUE)
            continue;
        auto handle_cleanup = defer([handle]() { FindClose(handle); });

        entries.clear();
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
                continue;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                const wchar_t *name = fd.cFileName;
                if (!wcscmp(name, L".") || !wcscmp(name, L".."))
                    continue;
                pathbuf.assign(dir);
                pathbuf.append(name);
                pathbuf.push_back(L'\\');
                entries.push_back(pathbuf);
            }
        } while (FindNextFileW(handle, &fd));

        std::sort(entries.begin(), entries.end());
        for (size_t n = entries.size(); n-- > 0; )
            dirs.push_front(std::move(entries[n]));
    }
}
#endif

int case_resolve(const char *root_, const char *fragment, std::string &result)
{
    if (fragment[0] == '\0')
        return 0;

    std::string root = path_ensure_final_separator(root_);

    std::string pathbuf;
    pathbuf.reserve(1024);

    pathbuf.assign(root);
    pathbuf.append(fragment);
    if (exists(pathbuf.c_str())) {
        result = std::move(pathbuf);
        return 1;
    }

    struct Item {
        std::string root;
        string_list components;
    };

    std::deque<Item> worklist;

    {
        Item item;
        item.root = root;
        item.components = split_strings_noempty(fragment, &is_path_separator);
        if (item.components.empty())
            return 0;
        for (size_t i = 0; i + 1 < item.components.size(); ++i)
            item.components[i].push_back('/');
        if (is_path_separator(fragment[strlen(fragment) - 1]))
            item.components.back().push_back('/');
        worklist.push_back(std::move(item));
    }

    while (!worklist.empty()) {
        Item item = std::move(worklist.front());
        worklist.pop_front();

        for (const std::string &entry : list_directory(item.root.c_str())) {
            if (ascii_casecmp(entry.c_str(), item.components[0].c_str()) != 0)
                continue;

            if (item.components.size() == 1) {
                pathbuf.assign(item.root);
                pathbuf.append(entry);
                if (exists(pathbuf.c_str())) {
                    result = std::move(pathbuf);
                    return 2;
                }
            }
            else {
                assert(item.components.size() > 1);
                Item newitem;
                newitem.root = item.root + entry;
                newitem.components.assign(item.components.begin() + 1, item.components.end());
                worklist.push_front(std::move(newitem));
            }
        }
    }

    return 0;
}

//------------------------------------------------------------------------------

#if defined(_WIN32)
std::wstring widen(const std::string &u8str)
{
    return widen(u8str.data(), u8str.size());
}

std::wstring widen(const char *u8data, size_t u8len)
{
    if (u8len == ~(size_t)0)
        u8len = strlen(u8data);
    std::wstring wstr;
    int wch = MultiByteToWideChar(CP_UTF8, 0, u8data, (int)u8len, nullptr, 0);
    if (wch != 0) {
        wstr.resize((size_t)wch);
        MultiByteToWideChar(CP_UTF8, 0, u8data, (int)u8len, &wstr[0], wch);
    }
    return wstr;
}

std::string narrow(const std::wstring &wstr)
{
    return narrow(wstr.data(), wstr.size());
}

std::string narrow(const wchar_t *wdata, size_t wlen)
{
    if (wlen == ~(size_t)0)
        wlen = wcslen(wdata);
    std::string u8str;
    int u8ch = WideCharToMultiByte(CP_UTF8, 0, wdata, (int)wlen, nullptr, 0, nullptr, nullptr);
    if (u8ch != 0) {
        u8str.resize((size_t)u8ch);
        WideCharToMultiByte(CP_UTF8, 0, wdata, (int)wlen, &u8str[0], u8ch, nullptr, nullptr);
    }
    return u8str;
}
#endif

} // namespace ysfx

//------------------------------------------------------------------------------
// WDL helpers

// our replacement `atof` for WDL, which is unaffected by current locale
extern "C" double ysfx_wdl_atof(const char *text)
{
    return ysfx::dot_atof(text);
}
