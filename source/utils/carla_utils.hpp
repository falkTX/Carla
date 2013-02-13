/*
 * Carla common utils
 * Copyright (C) 2011-2013 Filipe Coelho <falktx@falktx.com>
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

#ifndef __CARLA_UTILS_HPP__
#define __CARLA_UTILS_HPP__

#include "carla_juce_utils.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef CPP11_MUTEX
# include <mutex>
#else
# include <pthread.h>
#endif

#if defined(Q_OS_HAIKU)
# include <kernel/OS.h>
#elif defined(Q_OS_LINUX)
# include <sys/prctl.h>
# include <linux/prctl.h>
#endif

// -------------------------------------------------
// carla_assert*

static inline
void carla_assert(const char* const assertion, const char* const file, const int line)
{
    qCritical("Carla assertion failure: \"%s\" in file %s, line %i", assertion, file, line);
}

static inline
void carla_assert_int(const char* const assertion, const char* const file, const int line, const int value)
{
    qCritical("Carla assertion failure: \"%s\" in file %s, line %i, value %i", assertion, file, line, value);
}

static inline
void carla_assert_int2(const char* const assertion, const char* const file, const int line, const int v1, const int v2)
{
    qCritical("Carla assertion failure: \"%s\" in file %s, line %i, v1 %i, v2 %i", assertion, file, line, v1, v2);
}

// -------------------------------------------------
// carla_*sleep

static inline
void carla_sleep(const unsigned int secs)
{
    CARLA_ASSERT(secs > 0);

#ifdef Q_OS_WIN
    Sleep(secs * 1000);
#else
    sleep(secs);
#endif
}

static inline
void carla_msleep(const unsigned int msecs)
{
    CARLA_ASSERT(msecs > 0);

#ifdef Q_OS_WIN
    Sleep(msecs);
#else
    usleep(msecs * 1000);
#endif
}

static inline
void carla_usleep(const unsigned int usecs)
{
    CARLA_ASSERT(usecs > 0);

#ifdef Q_OS_WIN
    Sleep(usecs / 1000);
#else
    usleep(usecs);
#endif
}

// -------------------------------------------------
// carla_setenv

static inline
void carla_setenv(const char* const key, const char* const value)
{
    CARLA_ASSERT(key != nullptr);
    CARLA_ASSERT(value != nullptr);

#ifdef Q_OS_WIN
    SetEnvironmentVariableA(key, value);
#else
    setenv(key, value, 1);
#endif
}

// -------------------------------------------------
// carla_setprocname (not available on all platforms)

static inline
void carla_setprocname(const char* const name)
{
    CARLA_ASSERT(name != nullptr);

#if defined(Q_OS_HAIKU)
    if ((thread_id this_thread = find_thread(nullptr)) != B_NAME_NOT_FOUND)
        rename_thread(this_thread, name);
#elif defined(Q_OS_LINUX)
    prctl(PR_SET_NAME, name);
#else
    qWarning("carla_setprocname(\"%s\") - unsupported on this platform", name);
#endif
}

// -------------------------------------------------
// carla_strdup

static inline
const char* carla_strdup(const char* const strBuf)
{
    const size_t bufferLen = (strBuf != nullptr) ? std::strlen(strBuf) : 0;
    char* const  buffer    = new char [bufferLen+1];

    std::strcpy(buffer, strBuf);

    buffer[bufferLen] = '\0';

    return buffer;
}

static inline
const char* carla_strdup_free(const char* const strBuf)
{
    const char* const buffer = carla_strdup(strBuf);
    std::free((void*)strBuf);
    return buffer;
}

// -------------------------------------------------
// math functions

template<typename T>
static inline
const T& carla_min(const T& v1, const T& v2, const T& min)
{
    return ((v1 < min || v2 < min) ? min : (v1 < v2 ? v1 : v2));
}

template<typename T>
static inline
const T& carla_fixValue(const T& min, const T& max, const T& value)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

template<typename T>
static inline
void carla_fill(T* data, const unsigned int size, const T v)
{
    CARLA_ASSERT(data != nullptr);
    CARLA_ASSERT(size > 0);

    if (data == nullptr)
        return;

    for (unsigned int i=0; i < size; i++)
        *data++ = v;
}

static inline
void carla_zeroDouble(double* data, const unsigned size)
{
    carla_fill<double>(data, size, 0.0);
}

static inline
void carla_zeroFloat(float* data, const unsigned size)
{
    carla_fill<float>(data, size, 0.0f);
}

// -------------------------------------------------
// memory functions

static inline
void carla_zeroMem(void* const memory, const size_t numBytes)
{
    CARLA_ASSERT(memory != nullptr);

    if (memory == nullptr)
        return;

    std::memset(memory, 0, numBytes);
}

template <typename T>
static inline
void carla_zeroStruct(T& structure)
{
    std::memset(&structure, 0, sizeof(T));
}

// -------------------------------------------------
// other misc functions

static inline
const char* bool2str(const bool yesNo)
{
    return yesNo ? "true" : "false";
}

static inline
void pass() {}

// -------------------------------------------------
// CarlaMutex class

class CarlaMutex
{
public:
    CarlaMutex()
    {
#ifndef CPP11_MUTEX
        pthread_mutex_init(&pmutex, nullptr);
#endif
    }

    ~CarlaMutex()
    {
#ifndef CPP11_MUTEX
        pthread_mutex_destroy(&pmutex);
#endif
    }

    void lock()
    {
#ifdef CPP11_MUTEX
        cmutex.lock();
#else
        pthread_mutex_lock(&pmutex);
#endif
    }

    bool tryLock()
    {
#ifdef CPP11_MUTEX
        return cmutex.try_lock();
#else
        return (pthread_mutex_trylock(&pmutex) == 0);
#endif
    }

    void unlock()
    {
#ifdef CPP11_MUTEX
        cmutex.unlock();
#else
        pthread_mutex_unlock(&pmutex);
#endif
    }

    class ScopedLocker
    {
    public:
        ScopedLocker(CarlaMutex* const mutex)
            : fMutex(mutex)
        {
            fMutex->lock();
        }

        ~ScopedLocker()
        {
            fMutex->unlock();
        }

    private:
        CarlaMutex* const fMutex;

        CARLA_PREVENT_HEAP_ALLOCATION
        CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScopedLocker)
    };

private:
#ifdef CPP11_MUTEX
    std::mutex cmutex;
#else
    pthread_mutex_t pmutex;
#endif

    CARLA_PREVENT_HEAP_ALLOCATION
    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaMutex)
};

// -------------------------------------------------
// CarlaString class

class CarlaString
{
public:
    // ---------------------------------------------
    // constructors (no explicit conversions allowed)

    explicit CarlaString()
    {
        _init();
        _dup(nullptr);
    }

    explicit CarlaString(char* const strBuf)
    {
        _init();
        _dup(strBuf);
    }

    explicit CarlaString(const char* const strBuf)
    {
        _init();
        _dup(strBuf);
    }

    explicit CarlaString(const int value)
    {
        const size_t strBufSize = std::abs(value/10) + 3;
        char         strBuf[strBufSize];
        std::snprintf(strBuf, strBufSize, "%d", value);

        _init();
        _dup(strBuf, strBufSize);
    }

    explicit CarlaString(const unsigned int value, const bool hexadecimal = false)
    {
        const size_t strBufSize = value/10 + 2 + (hexadecimal ? 2 : 0);
        char         strBuf[strBufSize];
        std::snprintf(strBuf, strBufSize, hexadecimal ? "0x%x" : "%u", value);

        _init();
        _dup(strBuf, strBufSize);
    }

    explicit CarlaString(const long int value)
    {
        const size_t strBufSize = std::abs(value/10) + 3;
        char         strBuf[strBufSize];
        std::snprintf(strBuf, strBufSize, "%ld", value);

        _init();
        _dup(strBuf, strBufSize);
    }

    explicit CarlaString(const unsigned long int value, const bool hexadecimal = false)
    {
        const size_t strBufSize = value/10 + 2 + (hexadecimal ? 2 : 0);
        char         strBuf[strBufSize];
        std::snprintf(strBuf, strBufSize, hexadecimal ? "0x%lx" : "%lu", value);

        _init();
        _dup(strBuf, strBufSize);
    }

    explicit CarlaString(const float value)
    {
        char strBuf[0xff];
        std::snprintf(strBuf, 0xff, "%f", value);

        _init();
        _dup(strBuf);
    }

    explicit CarlaString(const double value)
    {
        char strBuf[0xff];
        std::snprintf(strBuf, 0xff, "%g", value);

        _init();
        _dup(strBuf);
    }

    // ---------------------------------------------
    // non-explicit constructor

    CarlaString(const CarlaString& str)
    {
        _init();
        _dup(str.buffer);
    }

    // ---------------------------------------------
    // deconstructor

    ~CarlaString()
    {
        CARLA_ASSERT(buffer);

        delete[] buffer;
    }

    // ---------------------------------------------
    // public methods

    size_t length() const
    {
        return bufferLen;
    }

    bool isEmpty() const
    {
        return (bufferLen == 0);
    }

    bool isNotEmpty() const
    {
        return (bufferLen != 0);
    }

#if __USE_GNU
    bool contains(const char* const strBuf, const bool ignoreCase = false) const
    {
        if (strBuf == nullptr)
            return false;

        if (ignoreCase)
            return (strcasestr(buffer, strBuf) != nullptr);
        else
            return (std::strstr(buffer, strBuf) != nullptr);
    }

    bool contains(const CarlaString& str, const bool ignoreCase = false) const
    {
        return contains(str.buffer, ignoreCase);
    }
#else
    bool contains(const char* const strBuf) const
    {
        if (strBuf == nullptr)
            return false;

        return (std::strstr(buffer, strBuf) != nullptr);
    }

    bool contains(const CarlaString& str) const
    {
        return contains(str.buffer);
    }
#endif

    bool isDigit(const size_t pos) const
    {
        if (pos >= bufferLen)
            return false;

        return (buffer[pos] >= '0' && buffer[pos] <= '9');
    }

    void clear()
    {
        truncate(0);
    }

    void replace(const char before, const char after)
    {
        if (after == '\0')
            return;

        for (size_t i=0; i < bufferLen; i++)
        {
            if (buffer[i] == before)
                buffer[i] = after;
            else if (buffer[i] == '\0')
                break;
        }
    }

    void truncate(const size_t n)
    {
        if (n >= bufferLen)
            return;

        for (size_t i=n; i < bufferLen; i++)
            buffer[i] = '\0';

        bufferLen = n;
    }

    void toBasic()
    {
        for (size_t i=0; i < bufferLen; i++)
        {
            if (buffer[i] >= '0' && buffer[i] <= '9')
                continue;
            if (buffer[i] >= 'A' && buffer[i] <= 'Z')
                continue;
            if (buffer[i] >= 'a' && buffer[i] <= 'z')
                continue;
            if (buffer[i] == '_')
                continue;

            buffer[i] = '_';
        }
    }

    void toLower()
    {
        static const char charDiff = 'a' - 'A';

        for (size_t i=0; i < bufferLen; i++)
        {
            if (buffer[i] >= 'A' && buffer[i] <= 'Z')
                buffer[i] += charDiff;
        }
    }

    void toUpper()
    {
        static const char charDiff = 'a' - 'A';

        for (size_t i=0; i < bufferLen; i++)
        {
            if (buffer[i] >= 'a' && buffer[i] <= 'z')
                buffer[i] -= charDiff;
        }
    }

    // ---------------------------------------------
    // public operators

    operator const char*() const
    {
        return buffer;
    }

    char& operator[](const size_t pos)
    {
        return buffer[pos];
    }

    bool operator==(const char* const strBuf) const
    {
        return (strBuf != nullptr && std::strcmp(buffer, strBuf) == 0);
    }

    bool operator==(const CarlaString& str) const
    {
        return operator==(str.buffer);
    }

    bool operator!=(const char* const strBuf) const
    {
        return !operator==(strBuf);
    }

    bool operator!=(const CarlaString& str) const
    {
        return !operator==(str.buffer);
    }

    CarlaString& operator=(const char* const strBuf)
    {
        _dup(strBuf);

        return *this;
    }

    CarlaString& operator=(const CarlaString& str)
    {
        return operator=(str.buffer);
    }

    CarlaString& operator+=(const char* const strBuf)
    {
        const size_t newBufSize = bufferLen + ((strBuf != nullptr) ? std::strlen(strBuf) : 0) + 1;
        char         newBuf[newBufSize];

        std::strcpy(newBuf, buffer);
        std::strcat(newBuf, strBuf);

        _dup(newBuf, newBufSize-1);

        return *this;
    }

    CarlaString& operator+=(const CarlaString& str)
    {
        return operator+=(str.buffer);
    }

    CarlaString operator+(const char* const strBuf)
    {
        const size_t newBufSize = bufferLen + ((strBuf != nullptr) ? std::strlen(strBuf) : 0) + 1;
        char         newBuf[newBufSize];

        std::strcpy(newBuf, buffer);
        std::strcat(newBuf, strBuf);

        return CarlaString(newBuf);
    }

    CarlaString operator+(const CarlaString& str)
    {
        return operator+(str.buffer);
    }

    // ---------------------------------------------

private:
    char*  buffer;
    size_t bufferLen;
    bool   firstInit;

    void _init()
    {
        buffer    = nullptr;
        bufferLen = 0;
        firstInit = true;
    }

    // allocate string strBuf if not null
    // size > 0 only if strBuf is valid
    void _dup(const char* const strBuf, const size_t size = 0)
    {
        if (strBuf != nullptr)
        {
            // don't recreate string if contents match
            if (firstInit || std::strcmp(buffer, strBuf) != 0)
            {
                if (! firstInit)
                {
                    CARLA_ASSERT(buffer);
                    delete[] buffer;
                }

                bufferLen = (size > 0) ? size : std::strlen(strBuf);
                buffer    = new char[bufferLen+1];

                std::strcpy(buffer, strBuf);

                buffer[bufferLen] = '\0';

                firstInit = false;
            }
        }
        else
        {
            CARLA_ASSERT(size == 0);

            // don't recreate null string
            if (firstInit || bufferLen != 0)
            {
                if (! firstInit)
                {
                    CARLA_ASSERT(buffer);
                    delete[] buffer;
                }

                bufferLen = 0;
                buffer    = new char[1];
                buffer[0] = '\0';

                firstInit = false;
            }
        }
    }

    CARLA_LEAK_DETECTOR(CarlaString)
    CARLA_PREVENT_HEAP_ALLOCATION
};

static inline
CarlaString operator+(const char* const strBufBefore, const CarlaString& strAfter)
{
    const char* const strBufAfter = (const char*)strAfter;
    const size_t newBufSize = ((strBufBefore != nullptr) ? std::strlen(strBufBefore) : 0) + std::strlen(strBufAfter) + 1;
    char         newBuf[newBufSize];

    std::strcpy(newBuf, strBufBefore);
    std::strcat(newBuf, strBufAfter);

    return CarlaString(newBuf);
}

// -------------------------------------------------

#endif // __CARLA_UTILS_HPP__
