/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2017 Filipe Coelho <falktx@falktx.com>

   Permission is granted to use this software under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license/

   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH REGARD
   TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
   FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
   OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
   USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
   OF THIS SOFTWARE.

  ==============================================================================
*/

#include "FileInputStream.h"

namespace water {

static int64 water_fileSetPosition (void* handle, int64 pos);

//==============================================================================
FileInputStream::FileInputStream (const File& f)
    : file (f),
      fileHandle (nullptr),
      currentPosition (0),
      status (Result::ok())
{
    openHandle();
}

int64 FileInputStream::getTotalLength()
{
    // You should always check that a stream opened successfully before using it!
    wassert (openedOk());

    return file.getSize();
}

int FileInputStream::read (void* buffer, int bytesToRead)
{
    // You should always check that a stream opened successfully before using it!
    wassert (openedOk());

    // The buffer should never be null, and a negative size is probably a
    // sign that something is broken!
    wassert (buffer != nullptr && bytesToRead >= 0);

    const size_t num = readInternal (buffer, (size_t) bytesToRead);
    currentPosition += (int64) num;

    return (int) num;
}

bool FileInputStream::isExhausted()
{
    return currentPosition >= getTotalLength();
}

int64 FileInputStream::getPosition()
{
    return currentPosition;
}

bool FileInputStream::setPosition (int64 pos)
{
    // You should always check that a stream opened successfully before using it!
    wassert (openedOk());

    if (pos != currentPosition)
        currentPosition = water_fileSetPosition (fileHandle, pos);

    return currentPosition == pos;
}

#ifdef CARLA_OS_WIN
FileInputStream::~FileInputStream()
{
    CloseHandle ((HANDLE) fileHandle);
}

void FileInputStream::openHandle()
{
    HANDLE h = CreateFile (file.getFullPathName().toUTF8(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);

    if (h != INVALID_HANDLE_VALUE)
        fileHandle = (void*) h;
    else
        status = getResultForLastError();
}

size_t FileInputStream::readInternal (void* buffer, size_t numBytes)
{
    if (fileHandle != 0)
    {
        DWORD actualNum = 0;
        if (! ReadFile ((HANDLE) fileHandle, buffer, (DWORD) numBytes, &actualNum, 0))
            status = getResultForLastError();

        return (size_t) actualNum;
    }

    return 0;
}
#else
FileInputStream::~FileInputStream()
{
    if (fileHandle != 0)
        close (getFD (fileHandle));
}

void FileInputStream::openHandle()
{
    const int f = open (file.getFullPathName().toUTF8(), O_RDONLY, 00644);

    if (f != -1)
        fileHandle = fdToVoidPointer (f);
    else
        status = getResultForErrno();
}

size_t FileInputStream::readInternal (void* const buffer, const size_t numBytes)
{
    ssize_t result = 0;

    if (fileHandle != 0)
    {
        result = ::read (getFD (fileHandle), buffer, numBytes);

        if (result < 0)
        {
            status = getResultForErrno();
            result = 0;
        }
    }

    return (size_t) result;
}
#endif

}
