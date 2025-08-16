/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2017-2024 Filipe Coelho <falktx@falktx.com>

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

#include "DirectoryIterator.h"

namespace water {

DirectoryIterator::DirectoryIterator (const File& directory, bool recursive,
                                      const String& pattern, const int type)
  : wildCards (parseWildcards (pattern)),
    fileFinder (directory, (recursive || wildCards.size() > 1) ? "*" : pattern),
    wildCard (pattern),
    path (File::addTrailingSeparator (directory.getFullPathName())),
    index (-1),
    totalNumFiles (-1),
    whatToLookFor (type),
    isRecursive (recursive),
    hasBeenAdvanced (false)
{
    // you have to specify the type of files you're looking for!
    wassert ((type & (File::findFiles | File::findDirectories)) != 0);
    wassert (type > 0 && type <= 7);
}

DirectoryIterator::~DirectoryIterator()
{
}

StringArray DirectoryIterator::parseWildcards (const String& pattern)
{
    StringArray s;
    s.addTokens (pattern, ";,", "\"'");
    s.trim();
    s.removeEmptyStrings();
    return s;
}

bool DirectoryIterator::fileMatches (const StringArray& wildCards, const String& filename)
{
    for (int i = 0; i < wildCards.size(); ++i)
        if (filename.matchesWildcard (wildCards[i], ! File::areFileNamesCaseSensitive()))
            return true;

    return false;
}

bool DirectoryIterator::next()
{
    return next (nullptr, nullptr, nullptr);
}

bool DirectoryIterator::next (bool* const isDirResult, int64* const fileSize, bool* const isReadOnly)
{
    for (;;)
    {
        hasBeenAdvanced = true;

        if (subIterator != nullptr)
        {
            if (subIterator->next (isDirResult, fileSize, isReadOnly))
                return true;

            subIterator = nullptr;
        }

        String filename;
        bool isDirectory, shouldContinue = false;

        while (fileFinder.next (filename, &isDirectory, fileSize, isReadOnly))
        {
            ++index;

            if (! filename.containsOnly ("."))
            {
                bool matches = false;

                if (isDirectory)
                {
                    if (isRecursive)
                        subIterator.reset(new DirectoryIterator (File::createFileWithoutCheckingPath (path + filename),
                                                                 true, wildCard, whatToLookFor));

                    matches = (whatToLookFor & File::findDirectories) != 0;
                }
                else
                {
                    matches = (whatToLookFor & File::findFiles) != 0;
                }

                // if we're not relying on the OS iterator to do the wildcard match, do it now..
                if (matches && (isRecursive || wildCards.size() > 1))
                    matches = fileMatches (wildCards, filename);

                if (matches)
                {
                    currentFile = File::createFileWithoutCheckingPath (path + filename);
                    if (isDirResult != nullptr)        *isDirResult = isDirectory;

                    return true;
                }

                if (subIterator != nullptr)
                {
                    shouldContinue = true;
                    break;
                }
            }
        }

        if (! shouldContinue)
            return false;
    }
}

const File& DirectoryIterator::getFile() const
{
    if (subIterator != nullptr && subIterator->hasBeenAdvanced)
        return subIterator->getFile();

    // You need to call DirectoryIterator::next() before asking it for the file that it found!
    wassert (hasBeenAdvanced);

    return currentFile;
}

}
