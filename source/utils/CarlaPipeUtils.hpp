/*
 * Carla Pipe utils
 * Copyright (C) 2013-2024 Filipe Coelho <falktx@falktx.com>
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

#ifdef BUILDING_CARLA
# include "lv2/atom.h"
#else
# include "lv2/atom/atom.h"
#endif

// -----------------------------------------------------------------------
// CarlaPipeCommon class

class CarlaPipeCommon
{
protected:
    /*!
     * Constructor.
     */
    CarlaPipeCommon() noexcept;

    /*!
     * Destructor.
     */
    virtual ~CarlaPipeCommon() /*noexcept*/;

    /*!
     * A message has been received (in the context of idlePipe()).
     * If extra data is required, use any of the readNextLineAs* functions.
     * Returning true means the message has been handled and should not propagate to subclasses.
     */
    virtual bool msgReceived(const char* msg) noexcept = 0;

    /*!
     * An error has occurred during the current requested operation.
     * Reimplementing this method allows to catch these errors as strings.
     * By default the error is simply printed to stderr.
     */
    virtual void fail(const char* error) noexcept
    {
        carla_stderr2(error);
    }

public:
    /*!
     * Check if the pipe is running.
     */
    bool isPipeRunning() const noexcept;

    /*!
     * Check the pipe for new messages and send them to msgReceived().
     */
    void idlePipe(bool onlyOnce = false) noexcept;

    // -------------------------------------------------------------------
    // write lock

    /*!
     * Lock the pipe write mutex.
     */
    void lockPipe() const noexcept;

    /*!
     * Try locking the pipe write mutex.
     * Returns true if successful.
     */
    bool tryLockPipe() const noexcept;

    /*!
     * Unlock the pipe write mutex.
     */
    void unlockPipe() const noexcept;

    /*!
     * Get the pipe write lock.
     */
    CarlaMutex& getPipeLock() const noexcept;

    // -------------------------------------------------------------------
    // read lines, must only be called in the context of msgReceived()

    /*!
     * Read the next line as a boolean.
     */
    bool readNextLineAsBool(bool& value) const noexcept;

    /*!
     * Read the next line as a byte.
     */
    bool readNextLineAsByte(uint8_t& value) const noexcept;

    /*!
     * Read the next line as an integer.
     */
    bool readNextLineAsInt(int32_t& value) const noexcept;

    /*!
     * Read the next line as an unsigned integer.
     */
    bool readNextLineAsUInt(uint32_t& value) const noexcept;

    /*!
     * Read the next line as a long integer.
     */
    bool readNextLineAsLong(int64_t& value) const noexcept;

    /*!
     * Read the next line as a long unsigned integer.
     */
    bool readNextLineAsULong(uint64_t& value) const noexcept;

    /*!
     * Read the next line as a floating point number (single precision).
     */
    bool readNextLineAsFloat(float& value) const noexcept;

    /*!
     * Read the next line as a floating point number (double precision).
     */
    bool readNextLineAsDouble(double& value) const noexcept;

    /*!
     * Read the next line as a string.
     * @note: @a value must be freed if valid and allocateString is true.
     */
    bool readNextLineAsString(const char*& value, bool allocateString, uint32_t size = 0) const noexcept;

    /*!
     * Read the next line as a string, returning an allocated copy that needs to be freed.
     */
    char* readNextLineAsString() const noexcept;

    // -------------------------------------------------------------------
    // write messages, must be locked before calling

    /*!
     * Write a valid message with unknown size.
     * A valid message has only one '\n' character and it's at the end.
     */
    bool writeMessage(const char* msg) const noexcept;

    /*!
     * Write a valid message with known size.
     * A valid message has only one '\n' character and it's at the end.
     */
    bool writeMessage(const char* msg, std::size_t size) const noexcept;

    /*!
     * Write and fix a message.
     */
    bool writeAndFixMessage(const char* msg) const noexcept;

    /*!
     * Write an empty message, which means a single '\n'.
     */
    bool writeEmptyMessage() const noexcept;

    /*!
     * Sync all messages currently in cache.
     * This call will forcely write any messages in cache to any relevant IO.
     */
    bool syncMessages() const noexcept;

    // -------------------------------------------------------------------
    // write prepared messages, no lock or flush needed (done internally)

    /*!
     * Write an "error" message.
     */
    bool writeErrorMessage(const char* error) const noexcept;

    /*!
     * Write a "control" message used for parameter/control changes.
     */
    bool writeControlMessage(uint32_t index, float value, bool withWriteLock = true) const noexcept;

    /*!
     * Write a "configure" message used for state changes.
     */
    bool writeConfigureMessage(const char* key, const char* value) const noexcept;

    /*!
     * Write a "program" message (using index).
     */
    bool writeProgramMessage(uint32_t index) const noexcept;

    /*!
     * Write a "program" message (using channel, bank and program).
     */
    bool writeProgramMessage(uint8_t channel, uint32_t bank, uint32_t program) const noexcept;

    /*!
     * Write a "midiprogram" message (using bank and program).
     */
    bool writeMidiProgramMessage(uint32_t bank, uint32_t program) const noexcept;

    /*!
     * Write a "reloadprograms" message.
     */
    bool writeReloadProgramsMessage(int32_t index) const noexcept;

    /*!
     * Write a MIDI "note" message.
     */
    bool writeMidiNoteMessage(bool onOff, uint8_t channel, uint8_t note, uint8_t velocity) const noexcept;

    /*!
     * Write an lv2 "atom" message.
     */
    bool writeLv2AtomMessage(uint32_t index, const LV2_Atom* atom) const noexcept;

    /*!
     * Write an lv2 "parameter" message.
     */
    bool writeLv2ParameterMessage(const char* uri, float value, bool withWriteLock = true) const noexcept;

    /*!
     * Write an lv2 "urid" message.
     */
    bool writeLv2UridMessage(uint32_t urid, const char* uri) const noexcept;

    // -------------------------------------------------------------------

protected:
    struct PrivateData;
    PrivateData* const pData;

    // -------------------------------------------------------------------

    /*! @internal */
    const char* _readline(bool allocReturn, uint16_t size, bool& readSucess) const noexcept;

    /*! @internal */
    const char* _readlineblock(bool allocReturn, uint16_t size = 0, uint32_t timeOutMilliseconds = 50) const noexcept;

    /*! @internal */
    bool _writeMsgBuffer(const char* msg, std::size_t size) const noexcept;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPipeCommon)
};

// -----------------------------------------------------------------------
// CarlaPipeServer class

class CarlaPipeServer : public CarlaPipeCommon
{
public:
    /*!
     * Constructor.
     */
    CarlaPipeServer() noexcept;

    /*!
     * Destructor.
     */
    ~CarlaPipeServer() /*noexcept*/ override;

    /*!
     * Get the process ID of this pipe's matching client.
     * Will return 0 if client is not running.
     * @note: Unsupported on Windows
     */
    uintptr_t getPID() const noexcept;

    /*!
     * Start the pipe server using @a filename with 2 arguments.
     * @see fail()
     */
    bool startPipeServer(const char* helperTool, const char* filename, const char* arg1, const char* arg2,
                         int size = -1, int timeOutMilliseconds = -1) noexcept;

    /*!
     * Start the pipe server using @a filename with 2 arguments.
     * @see fail()
     */
    bool startPipeServer(const char* filename, const char* arg1, const char* arg2,
                         int size = -1, int timeOutMilliseconds = -1) noexcept;

    /*!
     * Stop the pipe server.
     * This will send a quit message to the client, wait for it to close for @a timeOutMilliseconds, and close the pipes.
     */
    void stopPipeServer(uint32_t timeOutMilliseconds) noexcept;

    /*!
     * Close the pipes without waiting for the child process to terminate.
     */
    void closePipeServer() noexcept;

    // -------------------------------------------------------------------
    // write prepared messages, no lock or flush needed (done internally)

    /*!
     * Write a single "show" message.
     */
    void writeShowMessage() const noexcept;

    /*!
     * Write a single "focus" message.
     */
    void writeFocusMessage() const noexcept;

    /*!
     * Write a single "hide" message.
     */
    void writeHideMessage() const noexcept;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPipeServer)
};

// -----------------------------------------------------------------------
// CarlaPipeClient class

class CarlaPipeClient : public CarlaPipeCommon
{
public:
    /*!
     * Constructor.
     */
    CarlaPipeClient() noexcept;

    /*!
     * Destructor.
     */
    ~CarlaPipeClient() /*noexcept*/ override;

    /*!
     * Initialize the pipes used by a server.
     * @a argv must match the arguments set the by server.
     */
    bool initPipeClient(const char* argv[]) noexcept;

    /*!
     * Close the pipes.
     */
    void closePipeClient() noexcept;

    // -------------------------------------------------------------------
    // write prepared messages, no lock or flush needed (done internally)

    /*!
     * Write a single "exiting" message and wait for server to respond.
     */
    void writeExitingMessageAndWait() noexcept;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CarlaPipeClient)
};

// -----------------------------------------------------------------------

#endif // CARLA_PIPE_UTILS_HPP_INCLUDED
