/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2017-2022 Filipe Coelho <falktx@falktx.com>

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

#ifndef WATER_FILEINPUTSTREAM_H_INCLUDED
#define WATER_FILEINPUTSTREAM_H_INCLUDED

#include "File.h"
#include "../streams/InputStream.h"

namespace water {

//==============================================================================
/**
    An input stream that reads from a local file.

    @see InputStream, FileOutputStream, File::createInputStream
*/
class FileInputStream  : public InputStream
{
public:
    //==============================================================================
    /** Creates a FileInputStream to read from the given file.

        After creating a FileInputStream, you should use openedOk() or failedToOpen()
        to make sure that it's OK before trying to read from it! If it failed, you
        can call getStatus() to get more error information.
    */
    explicit FileInputStream (const File& fileToRead);

    /** Destructor. */
    ~FileInputStream();

    //==============================================================================
    /** Returns the file that this stream is reading from. */
    const File& getFile() const noexcept                { return file; }

    /** Returns the status of the file stream.
        The result will be ok if the file opened successfully. If an error occurs while
        opening or reading from the file, this will contain an error message.
    */
    const Result& getStatus() const noexcept            { return status; }

    /** Returns true if the stream couldn't be opened for some reason.
        @see getResult()
    */
    bool failedToOpen() const noexcept                  { return status.failed(); }

    /** Returns true if the stream opened without problems.
        @see getResult()
    */
    bool openedOk() const noexcept                      { return status.wasOk(); }


    //==============================================================================
    int64 getTotalLength() override;
    int read (void*, int) override;
    bool isExhausted() override;
    int64 getPosition() override;
    bool setPosition (int64) override;

private:
    //==============================================================================
    const File file;
    void* fileHandle;
    int64 currentPosition;
    Result status;

    void openHandle();
    size_t readInternal (void*, size_t);

    CARLA_DECLARE_NON_COPYABLE (FileInputStream)
};

}

#endif // WATER_FILEINPUTSTREAM_H_INCLUDED
