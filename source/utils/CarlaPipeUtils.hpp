/*
 * Carla Pipe utils
 * Copyright (C) 2013-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef CARLA_PIPE_UTILS_HPP_INCLUDED
#define CARLA_PIPE_UTILS_HPP_INCLUDED

#include "CarlaJuceUtils.hpp"
#include "CarlaMutex.hpp"

// -----------------------------------------------------------------------

class CarlaPipeCommon
{
protected:
    CarlaPipeCommon() noexcept;
    virtual ~CarlaPipeCommon() noexcept;

    // returns true if msg handled
    virtual bool msgReceived(const char* const msg) noexcept = 0;

    // to possibly send errors somewhere
    virtual void fail(const char* const error) noexcept
    {
        carla_stderr2(error);
    }

public:
    void idle() noexcept;
    bool isRunning() const noexcept;

    // -------------------------------------------------------------------
    // write lock

    void lock() const noexcept;
    bool tryLock() const noexcept;
    void unlock() const noexcept;

    CarlaMutex& getLock() noexcept;

    // -------------------------------------------------------------------

    bool readNextLineAsBool(bool& value) noexcept;
    bool readNextLineAsInt(int32_t& value) noexcept;
    bool readNextLineAsUInt(uint32_t& value) noexcept;
    bool readNextLineAsLong(int64_t& value) noexcept;
    bool readNextLineAsULong(uint64_t& value) noexcept;
    bool readNextLineAsFloat(float& value) noexcept;
    bool readNextLineAsDouble(double& value) noexcept;
    bool readNextLineAsString(const char*& value) noexcept;

    // -------------------------------------------------------------------
    // must be locked before calling

    bool writeMsg(const char* const msg) const noexcept;
    bool writeMsg(const char* const msg, std::size_t size) const noexcept;
    bool writeAndFixMsg(const char* const msg) const noexcept;
    bool flush() const noexcept;

    // -------------------------------------------------------------------

private:
    struct PrivateData;
    PrivateData* const pData;

    // -------------------------------------------------------------------

    friend class CarlaPipeClient;
    friend class CarlaPipeServer;

    // internal
    const char* readline() noexcept;
    const char* readlineblock(const uint32_t timeOutMilliseconds = 50) noexcept;
    bool writeMsgBuffer(const char* const msg, const std::size_t size) const noexcept;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPipeCommon)
};

// -----------------------------------------------------------------------

class CarlaPipeServer : public CarlaPipeCommon
{
public:
    CarlaPipeServer() noexcept;
    ~CarlaPipeServer() noexcept override;

    bool start(const char* const filename, const char* const arg1, const char* const arg2) noexcept;
    void stop(const uint32_t timeOutMilliseconds) noexcept;
    void close() noexcept;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPipeServer)
};

// -----------------------------------------------------------------------

class CarlaPipeClient : public CarlaPipeCommon
{
public:
    CarlaPipeClient() noexcept;
    ~CarlaPipeClient() noexcept override;

    bool init(char* argv[]) noexcept;
    void close() noexcept;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPipeClient)
};

// -----------------------------------------------------------------------

class ScopedLocale {
public:
    ScopedLocale() noexcept;
    ~ScopedLocale() noexcept;

private:
    const char* const fLocale;

    CARLA_DECLARE_NON_COPY_CLASS(ScopedLocale)
    CARLA_PREVENT_HEAP_ALLOCATION
};

// -----------------------------------------------------------------------

#endif // CARLA_PIPE_UTILS_HPP_INCLUDED
