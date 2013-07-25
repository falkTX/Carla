/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DISTRHO_UTILS_HPP_INCLUDED
#define DISTRHO_UTILS_HPP_INCLUDED

#include "src/DistrhoDefines.h"

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef PROPER_CPP11_SUPPORT
# include <cstdint>
#else
# include <stdint.h>
#endif

#ifdef DISTRHO_OS_WINDOWS
# include <windows.h>
#else
# include <unistd.h>
#endif

#if defined(DISTRHO_OS_MAC) && ! defined(CARLA_OS_MAC)
namespace std {
inline float
  fmin(float __x, float __y)
  { return __builtin_fminf(__x, __y); }
inline float
  fmax(float __x, float __y)
  { return __builtin_fmaxf(__x, __y); }
inline float
  rint(float __x)
  { return __builtin_rintf(__x); }
}
#endif

// -----------------------------------------------------------------------
// misc functions

static inline
long d_cconst(int a, int b, int c, int d) noexcept
{
    return (a << 24) | (b << 16) | (c << 8) | (d << 0);
}

// -----------------------------------------------------------------------
// string print functions

#ifndef DEBUG
# define d_debug(...)
#else
static inline
void d_debug(const char* const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::fprintf(stdout, "\x1b[30;1m");
    std::vfprintf(stdout, fmt, args);
    std::fprintf(stdout, "\x1b[0m\n");
    va_end(args);
}
#endif

static inline
void d_stdout(const char* const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::vfprintf(stdout, fmt, args);
    std::fprintf(stdout, "\n");
    va_end(args);
}

static inline
void d_stderr(const char* const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::vfprintf(stderr, fmt, args);
    std::fprintf(stderr, "\n");
    va_end(args);
}

static inline
void d_stderr2(const char* const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::fprintf(stderr, "\x1b[31m");
    std::vfprintf(stderr, fmt, args);
    std::fprintf(stderr, "\x1b[0m\n");
    va_end(args);
}

// -----------------------------------------------------------------------
// d_*sleep

static inline
void d_sleep(unsigned int secs)
{
#ifdef DISTRHO_OS_WINDOWS
    Sleep(secs * 1000);
#else
    sleep(secs);
#endif
}

static inline
void d_msleep(unsigned int msecs)
{
#ifdef DISTRHO_OS_WINDOWS
    Sleep(msecs);
#else
    usleep(msecs * 1000);
#endif
}

// -----------------------------------------------------------------------
// d_string class

class d_string
{
public:
    // -------------------------------------------------------------------
    // constructors (no explicit conversions allowed)

    explicit d_string()
    {
        _init();
        _dup(nullptr);
    }

    explicit d_string(char* const strBuf)
    {
        _init();
        _dup(strBuf);
    }

    explicit d_string(const char* const strBuf)
    {
        _init();
        _dup(strBuf);
    }

    explicit d_string(const int value)
    {
        char strBuf[0xff+1];
        std::memset(strBuf, 0, (0xff+1)*sizeof(char));
        std::snprintf(strBuf, 0xff, "%d", value);

        _init();
        _dup(strBuf);
    }

    explicit d_string(const unsigned int value, const bool hexadecimal = false)
    {
        char strBuf[0xff+1];
        std::memset(strBuf, 0, (0xff+1)*sizeof(char));
        std::snprintf(strBuf, 0xff, hexadecimal ? "0x%x" : "%u", value);

        _init();
        _dup(strBuf);
    }

    explicit d_string(const long int value)
    {
        char strBuf[0xff+1];
        std::memset(strBuf, 0, (0xff+1)*sizeof(char));
        std::snprintf(strBuf, 0xff, "%ld", value);

        _init();
        _dup(strBuf);
    }

    explicit d_string(const unsigned long int value, const bool hexadecimal = false)
    {
        char strBuf[0xff+1];
        std::memset(strBuf, 0, (0xff+1)*sizeof(char));
        std::snprintf(strBuf, 0xff, hexadecimal ? "0x%lx" : "%lu", value);

        _init();
        _dup(strBuf);
    }

    explicit d_string(const float value)
    {
        char strBuf[0xff+1];
        std::memset(strBuf, 0, (0xff+1)*sizeof(char));
        std::snprintf(strBuf, 0xff, "%f", value);

        _init();
        _dup(strBuf);
    }

    explicit d_string(const double value)
    {
        char strBuf[0xff+1];
        std::memset(strBuf, 0, (0xff+1)*sizeof(char));
        std::snprintf(strBuf, 0xff, "%g", value);

        _init();
        _dup(strBuf);
    }

    // -------------------------------------------------------------------
    // non-explicit constructor

    d_string(const d_string& str)
    {
        _init();
        _dup(str.fBuffer);
    }

    // -------------------------------------------------------------------
    // destructor

    ~d_string()
    {
        assert(fBuffer != nullptr);

        delete[] fBuffer;
        fBuffer = nullptr;
    }

    // -------------------------------------------------------------------
    // public methods

    size_t length() const noexcept
    {
        return fBufferLen;
    }

    bool isEmpty() const noexcept
    {
        return (fBufferLen == 0);
    }

    bool isNotEmpty() const noexcept
    {
        return (fBufferLen != 0);
    }

#ifdef __USE_GNU
    bool contains(const char* const strBuf, const bool ignoreCase = false) const
    {
        if (strBuf == nullptr)
            return false;

        if (ignoreCase)
            return (strcasestr(fBuffer, strBuf) != nullptr);
        else
            return (std::strstr(fBuffer, strBuf) != nullptr);
    }

    bool contains(const d_string& str, const bool ignoreCase = false) const
    {
        return contains(str.fBuffer, ignoreCase);
    }
#else
    bool contains(const char* const strBuf) const
    {
        if (strBuf == nullptr)
            return false;

        return (std::strstr(fBuffer, strBuf) != nullptr);
    }

    bool contains(const d_string& str) const
    {
        return contains(str.fBuffer);
    }
#endif

    bool isDigit(const size_t pos) const noexcept
    {
        if (pos >= fBufferLen)
            return false;

        return (fBuffer[pos] >= '0' && fBuffer[pos] <= '9');
    }

    bool startsWith(const char* const prefix) const
    {
        if (prefix == nullptr)
            return false;

        const size_t prefixLen(std::strlen(prefix));

        if (fBufferLen < prefixLen)
            return false;

        return (std::strncmp(fBuffer + (fBufferLen-prefixLen), prefix, prefixLen) == 0);
    }

    bool endsWith(const char* const suffix) const
    {
        if (suffix == nullptr)
            return false;

        const size_t suffixLen(std::strlen(suffix));

        if (fBufferLen < suffixLen)
            return false;

        return (std::strncmp(fBuffer + (fBufferLen-suffixLen), suffix, suffixLen) == 0);
    }

    void clear() noexcept
    {
        truncate(0);
    }

    size_t find(const char c) const noexcept
    {
        for (size_t i=0; i < fBufferLen; ++i)
        {
            if (fBuffer[i] == c)
                return i;
        }

        return 0;
    }

    size_t rfind(const char c) const noexcept
    {
        for (size_t i=fBufferLen; i > 0; --i)
        {
            if (fBuffer[i-1] == c)
                return i-1;
        }

        return 0;
    }

    size_t rfind(const char* const strBuf) const
    {
        if (strBuf == nullptr || strBuf[0] == '\0')
            return fBufferLen;

        size_t ret = fBufferLen+1;
        const char* tmpBuf = fBuffer;

        for (size_t i=0; i < fBufferLen; ++i)
        {
            if (std::strstr(tmpBuf, strBuf) == nullptr)
                break;

            --ret;
            ++tmpBuf;
        }

        return (ret > fBufferLen) ? fBufferLen : fBufferLen-ret;
    }

    void replace(const char before, const char after) noexcept
    {
        if (after == '\0')
            return;

        for (size_t i=0; i < fBufferLen; ++i)
        {
            if (fBuffer[i] == before)
                fBuffer[i] = after;
            else if (fBuffer[i] == '\0')
                break;
        }
    }

    void truncate(const size_t n) noexcept
    {
        if (n >= fBufferLen)
            return;

        for (size_t i=n; i < fBufferLen; ++i)
            fBuffer[i] = '\0';

        fBufferLen = n;
    }

    void toBasic() noexcept
    {
        for (size_t i=0; i < fBufferLen; ++i)
        {
            if (fBuffer[i] >= '0' && fBuffer[i] <= '9')
                continue;
            if (fBuffer[i] >= 'A' && fBuffer[i] <= 'Z')
                continue;
            if (fBuffer[i] >= 'a' && fBuffer[i] <= 'z')
                continue;
            if (fBuffer[i] == '_')
                continue;

            fBuffer[i] = '_';
        }
    }

    void toLower() noexcept
    {
        static const char kCharDiff('a' - 'A');

        for (size_t i=0; i < fBufferLen; ++i)
        {
            if (fBuffer[i] >= 'A' && fBuffer[i] <= 'Z')
                fBuffer[i] += kCharDiff;
        }
    }

    void toUpper() noexcept
    {
        static const char kCharDiff('a' - 'A');

        for (size_t i=0; i < fBufferLen; ++i)
        {
            if (fBuffer[i] >= 'a' && fBuffer[i] <= 'z')
                fBuffer[i] -= kCharDiff;
        }
    }

    // -------------------------------------------------------------------
    // public operators

    operator const char*() const noexcept
    {
        return fBuffer;
    }

    char& operator[](const size_t pos) const noexcept
    {
        return fBuffer[pos];
    }

    bool operator==(const char* const strBuf) const
    {
        return (strBuf != nullptr && std::strcmp(fBuffer, strBuf) == 0);
    }

    bool operator==(const d_string& str) const
    {
        return operator==(str.fBuffer);
    }

    bool operator!=(const char* const strBuf) const
    {
        return !operator==(strBuf);
    }

    bool operator!=(const d_string& str) const
    {
        return !operator==(str.fBuffer);
    }

    d_string& operator=(const char* const strBuf)
    {
        _dup(strBuf);

        return *this;
    }

    d_string& operator=(const d_string& str)
    {
        return operator=(str.fBuffer);
    }

    d_string& operator+=(const char* const strBuf)
    {
        const size_t newBufSize = fBufferLen + ((strBuf != nullptr) ? std::strlen(strBuf) : 0) + 1;
        char         newBuf[newBufSize];

        std::strcpy(newBuf, fBuffer);
        std::strcat(newBuf, strBuf);

        _dup(newBuf, newBufSize-1);

        return *this;
    }

    d_string& operator+=(const d_string& str)
    {
        return operator+=(str.fBuffer);
    }

    d_string operator+(const char* const strBuf)
    {
        const size_t newBufSize = fBufferLen + ((strBuf != nullptr) ? std::strlen(strBuf) : 0) + 1;
        char         newBuf[newBufSize];

        std::strcpy(newBuf, fBuffer);
        std::strcat(newBuf, strBuf);

        return d_string(newBuf);
    }

    d_string operator+(const d_string& str)
    {
        return operator+(str.fBuffer);
    }

    // -------------------------------------------------------------------

private:
    char*  fBuffer;
    size_t fBufferLen;
    bool   fFirstInit;

    void _init() noexcept
    {
        fBuffer    = nullptr;
        fBufferLen = 0;
        fFirstInit = true;
    }

    // allocate string strBuf if not null
    // size > 0 only if strBuf is valid
    void _dup(const char* const strBuf, const size_t size = 0)
    {
        if (strBuf != nullptr)
        {
            // don't recreate string if contents match
            if (fFirstInit || std::strcmp(fBuffer, strBuf) != 0)
            {
                if (! fFirstInit)
                {
                    assert(fBuffer != nullptr);
                    delete[] fBuffer;
                }

                fBufferLen = (size > 0) ? size : std::strlen(strBuf);
                fBuffer    = new char[fBufferLen+1];

                std::strcpy(fBuffer, strBuf);

                fBuffer[fBufferLen] = '\0';

                fFirstInit = false;
            }
        }
        else
        {
            assert(size == 0);

            // don't recreate null string
            if (fFirstInit || fBufferLen != 0)
            {
                if (! fFirstInit)
                {
                    assert(fBuffer != nullptr);
                    delete[] fBuffer;
                }

                fBufferLen = 0;
                fBuffer    = new char[1];
                fBuffer[0] = '\0';

                fFirstInit = false;
            }
        }
    }
};

// -----------------------------------------------------------------------

static inline
d_string operator+(const d_string& strBefore, const char* const strBufAfter)
{
    const char* const strBufBefore = (const char*)strBefore;
    const      size_t newBufSize   = strBefore.length() + ((strBufAfter != nullptr) ? std::strlen(strBufAfter) : 0) + 1;
    char newBuf[newBufSize];

    std::strcpy(newBuf, strBufBefore);
    std::strcat(newBuf, strBufAfter);

    return d_string(newBuf);
}

static inline
d_string operator+(const char* const strBufBefore, const d_string& strAfter)
{
    const char* const strBufAfter = (const char*)strAfter;
    const size_t      newBufSize  = ((strBufBefore != nullptr) ? std::strlen(strBufBefore) : 0) + strAfter.length() + 1;
    char newBuf[newBufSize];

    std::strcpy(newBuf, strBufBefore);
    std::strcat(newBuf, strBufAfter);

    return d_string(newBuf);
}

// -----------------------------------------------------------------------

#endif // DISTRHO_UTILS_HPP_INCLUDED
