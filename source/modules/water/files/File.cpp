/*
  ==============================================================================

   This file is part of the Water library.
   Copyright (c) 2016 ROLI Ltd.
   Copyright (C) 2017-2018 Filipe Coelho <falktx@falktx.com>

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

#include "File.h"
#include "DirectoryIterator.h"
#include "FileInputStream.h"
#include "FileOutputStream.h"
#include "TemporaryFile.h"
#include "../maths/Random.h"
#include "../misc/Time.h"
#include "../text/StringArray.h"

#include "CarlaJuceUtils.hpp"

#ifdef CARLA_OS_WIN
# include <shlobj.h>
#else
# include <dlfcn.h>
# include <fcntl.h>
# include <fnmatch.h>
# include <pwd.h>
# include <sys/stat.h>
# ifdef CARLA_OS_MAC
#  include <mach-o/dyld.h>
#  import <Foundation/NSFileManager.h>
#  import <Foundation/NSPathUtilities.h>
#  import <Foundation/NSString.h>
# else
#  include <dirent.h>
# endif
#endif

namespace water {

File::File (const String& fullPathName)
    : fullPath (parseAbsolutePath (fullPathName))
{
}

File File::createFileWithoutCheckingPath (const String& path) noexcept
{
    File f;
    f.fullPath = path;
    return f;
}

File::File (const File& other)
    : fullPath (other.fullPath)
{
}

File& File::operator= (const String& newPath)
{
    fullPath = parseAbsolutePath (newPath);
    return *this;
}

File& File::operator= (const File& other)
{
    fullPath = other.fullPath;
    return *this;
}

#if WATER_COMPILER_SUPPORTS_MOVE_SEMANTICS
File::File (File&& other) noexcept
    : fullPath (static_cast<String&&> (other.fullPath))
{
}

File& File::operator= (File&& other) noexcept
{
    fullPath = static_cast<String&&> (other.fullPath);
    return *this;
}
#endif

//==============================================================================
static String removeEllipsis (const String& path)
{
    // This will quickly find both /../ and /./ at the expense of a minor
    // false-positive performance hit when path elements end in a dot.
   #ifdef CARLA_OS_WIN
    if (path.contains (".\\"))
   #else
    if (path.contains ("./"))
   #endif
    {
        StringArray toks;
        toks.addTokens (path, File::separatorString, StringRef());
        bool anythingChanged = false;

        for (int i = 1; i < toks.size(); ++i)
        {
            const String& t = toks[i];

            if (t == ".." && toks[i - 1] != "..")
            {
                anythingChanged = true;
                toks.removeRange (i - 1, 2);
                i = jmax (0, i - 2);
            }
            else if (t == ".")
            {
                anythingChanged = true;
                toks.remove (i--);
            }
        }

        if (anythingChanged)
            return toks.joinIntoString (File::separatorString);
    }

    return path;
}

String File::parseAbsolutePath (const String& p)
{
    if (p.isEmpty())
        return String();

#ifdef CARLA_OS_WIN
    // Windows..
    String path (removeEllipsis (p.replaceCharacter ('/', '\\')));

    if (path.startsWithChar (separator))
    {
        if (path[1] != separator)
        {
            // Check if path is valid under Wine
            String testpath ("Z:" + path);

            if (File(testpath).exists())
            {
                path = testpath;
            }
            else
            {
                /*  When you supply a raw string to the File object constructor, it must be an absolute path.
                    If you're trying to parse a string that may be either a relative path or an absolute path,
                    you MUST provide a context against which the partial path can be evaluated - you can do
                    this by simply using File::getChildFile() instead of the File constructor. E.g. saying
                    "File::getCurrentWorkingDirectory().getChildFile (myUnknownPath)" would return an absolute
                    path if that's what was supplied, or would evaluate a partial path relative to the CWD.
                */
                carla_safe_assert(testpath.toRawUTF8(), __FILE__, __LINE__);

                path = File::getCurrentWorkingDirectory().getFullPathName().substring (0, 2) + path;
            }
        }
    }
    else if (! path.containsChar (':'))
    {
        /*  When you supply a raw string to the File object constructor, it must be an absolute path.
            If you're trying to parse a string that may be either a relative path or an absolute path,
            you MUST provide a context against which the partial path can be evaluated - you can do
            this by simply using File::getChildFile() instead of the File constructor. E.g. saying
            "File::getCurrentWorkingDirectory().getChildFile (myUnknownPath)" would return an absolute
            path if that's what was supplied, or would evaluate a partial path relative to the CWD.
        */
        carla_safe_assert(path.toRawUTF8(), __FILE__, __LINE__);

        return File::getCurrentWorkingDirectory().getChildFile (path).getFullPathName();
    }
#else
    // Mac or Linux..

    // Yes, I know it's legal for a unix pathname to contain a backslash, but this assertion is here
    // to catch anyone who's trying to run code that was written on Windows with hard-coded path names.
    // If that's why you've ended up here, use File::getChildFile() to build your paths instead.
    jassert ((! p.containsChar ('\\')) || (p.indexOfChar ('/') >= 0 && p.indexOfChar ('/') < p.indexOfChar ('\\')));

    String path (removeEllipsis (p));

    if (path.startsWithChar ('~'))
    {
        if (path[1] == separator || path[1] == 0)
        {
            // expand a name of the form "~/abc"
            path = File::getSpecialLocation (File::userHomeDirectory).getFullPathName()
                    + path.substring (1);
        }
        else
        {
            // expand a name of type "~dave/abc"
            const String userName (path.substring (1).upToFirstOccurrenceOf ("/", false, false));

            if (struct passwd* const pw = getpwnam (userName.toUTF8()))
                path = addTrailingSeparator (pw->pw_dir) + path.fromFirstOccurrenceOf ("/", false, false);
        }
    }
    else if (! path.startsWithChar (separator))
    {
        /*  When you supply a raw string to the File object constructor, it must be an absolute path.
            If you're trying to parse a string that may be either a relative path or an absolute path,
            you MUST provide a context against which the partial path can be evaluated - you can do
            this by simply using File::getChildFile() instead of the File constructor. E.g. saying
            "File::getCurrentWorkingDirectory().getChildFile (myUnknownPath)" would return an absolute
            path if that's what was supplied, or would evaluate a partial path relative to the CWD.
        */
        CARLA_SAFE_ASSERT_RETURN(path.startsWith ("./") || path.startsWith ("../"), String());

        return File::getCurrentWorkingDirectory().getChildFile (path).getFullPathName();
    }
#endif

    while (path.endsWithChar (separator) && path != separatorString) // careful not to turn a single "/" into an empty string.
        path = path.dropLastCharacters (1);

    return path;
}

String File::addTrailingSeparator (const String& path)
{
    return path.endsWithChar (separator) ? path
                                         : path + separator;
}

//==============================================================================
#if ! (defined(CARLA_OS_MAC) || defined(CARLA_OS_WIN))
 #define NAMES_ARE_CASE_SENSITIVE 1
#endif

bool File::areFileNamesCaseSensitive()
{
   #if NAMES_ARE_CASE_SENSITIVE
    return true;
   #else
    return false;
   #endif
}

static int compareFilenames (const String& name1, const String& name2) noexcept
{
   #if NAMES_ARE_CASE_SENSITIVE
    return name1.compare (name2);
   #else
    return name1.compareIgnoreCase (name2);
   #endif
}

bool File::operator== (const File& other) const     { return compareFilenames (fullPath, other.fullPath) == 0; }
bool File::operator!= (const File& other) const     { return compareFilenames (fullPath, other.fullPath) != 0; }
bool File::operator<  (const File& other) const     { return compareFilenames (fullPath, other.fullPath) <  0; }
bool File::operator>  (const File& other) const     { return compareFilenames (fullPath, other.fullPath) >  0; }

//==============================================================================
bool File::deleteRecursively() const
{
    bool worked = true;

    if (isDirectory())
    {
        Array<File> subFiles;
        findChildFiles (subFiles, File::findFilesAndDirectories, false);

        for (int i = subFiles.size(); --i >= 0;)
            worked = subFiles.getReference(i).deleteRecursively() && worked;
    }

    return deleteFile() && worked;
}

bool File::moveFileTo (const File& newFile) const
{
    if (newFile.fullPath == fullPath)
        return true;

    if (! exists())
        return false;

   #if ! NAMES_ARE_CASE_SENSITIVE
    if (*this != newFile)
   #endif
        if (! newFile.deleteFile())
            return false;

    return moveInternal (newFile);
}

bool File::copyFileTo (const File& newFile) const
{
    return (*this == newFile)
            || (exists() && newFile.deleteFile() && copyInternal (newFile));
}

bool File::replaceFileIn (const File& newFile) const
{
    if (newFile.fullPath == fullPath)
        return true;

    if (! newFile.exists())
        return moveFileTo (newFile);

    if (! replaceInternal (newFile))
        return false;

    deleteFile();
    return true;
}

bool File::copyDirectoryTo (const File& newDirectory) const
{
    if (isDirectory() && newDirectory.createDirectory())
    {
        Array<File> subFiles;
        findChildFiles (subFiles, File::findFiles, false);

        for (int i = 0; i < subFiles.size(); ++i)
            if (! subFiles.getReference(i).copyFileTo (newDirectory.getChildFile (subFiles.getReference(i).getFileName())))
                return false;

        subFiles.clear();
        findChildFiles (subFiles, File::findDirectories, false);

        for (int i = 0; i < subFiles.size(); ++i)
            if (! subFiles.getReference(i).copyDirectoryTo (newDirectory.getChildFile (subFiles.getReference(i).getFileName())))
                return false;

        return true;
    }

    return false;
}

//==============================================================================
String File::getPathUpToLastSlash() const
{
    const int lastSlash = fullPath.lastIndexOfChar (separator);

    if (lastSlash > 0)
        return fullPath.substring (0, lastSlash);

    if (lastSlash == 0)
        return separatorString;

    return fullPath;
}

File File::getParentDirectory() const
{
    File f;
    f.fullPath = getPathUpToLastSlash();
    return f;
}

//==============================================================================
String File::getFileName() const
{
    return fullPath.substring (fullPath.lastIndexOfChar (separator) + 1);
}

String File::getFileNameWithoutExtension() const
{
    const int lastSlash = fullPath.lastIndexOfChar (separator) + 1;
    const int lastDot   = fullPath.lastIndexOfChar ('.');

    if (lastDot > lastSlash)
        return fullPath.substring (lastSlash, lastDot);

    return fullPath.substring (lastSlash);
}

bool File::isAChildOf (const File& potentialParent) const
{
    if (potentialParent.fullPath.isEmpty())
        return false;

    const String ourPath (getPathUpToLastSlash());

    if (compareFilenames (potentialParent.fullPath, ourPath) == 0)
        return true;

    if (potentialParent.fullPath.length() >= ourPath.length())
        return false;

    return getParentDirectory().isAChildOf (potentialParent);
}

//==============================================================================
bool File::isAbsolutePath (StringRef path)
{
    const water_uchar firstChar = *(path.text);

    return firstChar == separator
           #ifdef CARLA_OS_WIN
            || (firstChar != 0 && path.text[1] == ':');
           #else
            || firstChar == '~';
           #endif
}

File File::getChildFile (StringRef relativePath) const
{
    String::CharPointerType r = relativePath.text;

    if (isAbsolutePath (r))
        return File (String (r));

   #ifdef CARLA_OS_WIN
    if (r.indexOf ((water_uchar) '/') >= 0)
        return getChildFile (String (r).replaceCharacter ('/', '\\'));
   #endif

    String path (fullPath);

    while (*r == '.')
    {
        String::CharPointerType lastPos = r;
        const water_uchar secondChar = *++r;

        if (secondChar == '.') // remove "../"
        {
            const water_uchar thirdChar = *++r;

            if (thirdChar == separator || thirdChar == 0)
            {
                const int lastSlash = path.lastIndexOfChar (separator);
                if (lastSlash >= 0)
                    path = path.substring (0, lastSlash);

                while (*r == separator) // ignore duplicate slashes
                    ++r;
            }
            else
            {
                r = lastPos;
                break;
            }
        }
        else if (secondChar == separator || secondChar == 0)  // remove "./"
        {
            while (*r == separator) // ignore duplicate slashes
                ++r;
        }
        else
        {
            r = lastPos;
            break;
        }
    }

    path = addTrailingSeparator (path);
    path.appendCharPointer (r);
    return File (path);
}

File File::getSiblingFile (StringRef fileName) const
{
    return getParentDirectory().getChildFile (fileName);
}

//==============================================================================
String File::descriptionOfSizeInBytes (const int64 bytes)
{
    const char* suffix;
    double divisor = 0;

    if (bytes == 1)                       { suffix = " byte"; }
    else if (bytes < 1024)                { suffix = " bytes"; }
    else if (bytes < 1024 * 1024)         { suffix = " KB"; divisor = 1024.0; }
    else if (bytes < 1024 * 1024 * 1024)  { suffix = " MB"; divisor = 1024.0 * 1024.0; }
    else                                  { suffix = " GB"; divisor = 1024.0 * 1024.0 * 1024.0; }

    return (divisor > 0 ? String (bytes / divisor, 1) : String (bytes)) + suffix;
}

//==============================================================================
Result File::create() const
{
    if (exists())
        return Result::ok();

    const File parentDir (getParentDirectory());

    if (parentDir == *this)
        return Result::fail ("Cannot create parent directory");

    Result r (parentDir.createDirectory());

    if (r.wasOk())
    {
        FileOutputStream fo (*this, 8);
        r = fo.getStatus();
    }

    return r;
}

Result File::createDirectory() const
{
    if (isDirectory())
        return Result::ok();

    const File parentDir (getParentDirectory());

    if (parentDir == *this)
        return Result::fail ("Cannot create parent directory");

    Result r (parentDir.createDirectory());

    if (r.wasOk())
        r = createDirectoryInternal (fullPath.trimCharactersAtEnd (separatorString));

    return r;
}

//==============================================================================
bool File::loadFileAsData (MemoryBlock& destBlock) const
{
    if (! existsAsFile())
        return false;

    FileInputStream in (*this);
    return in.openedOk() && getSize() == (int64) in.readIntoMemoryBlock (destBlock);
}

String File::loadFileAsString() const
{
    if (! existsAsFile())
        return String();

    FileInputStream in (*this);
    return in.openedOk() ? in.readEntireStreamAsString()
                         : String();
}

void File::readLines (StringArray& destLines) const
{
    destLines.addLines (loadFileAsString());
}

//==============================================================================
int File::findChildFiles (Array<File>& results,
                          const int whatToLookFor,
                          const bool searchRecursively,
                          const String& wildCardPattern) const
{
    int total = 0;

    for (DirectoryIterator di (*this, searchRecursively, wildCardPattern, whatToLookFor); di.next();)
    {
        results.add (di.getFile());
        ++total;
    }

    return total;
}

int File::getNumberOfChildFiles (const int whatToLookFor, const String& wildCardPattern) const
{
    int total = 0;

    for (DirectoryIterator di (*this, false, wildCardPattern, whatToLookFor); di.next();)
        ++total;

    return total;
}

bool File::containsSubDirectories() const
{
    if (! isDirectory())
        return false;

    DirectoryIterator di (*this, false, "*", findDirectories);
    return di.next();
}

//==============================================================================
File File::getNonexistentChildFile (const String& suggestedPrefix,
                                    const String& suffix,
                                    bool putNumbersInBrackets) const
{
    File f (getChildFile (suggestedPrefix + suffix));

    if (f.exists())
    {
        int number = 1;
        String prefix (suggestedPrefix);

        // remove any bracketed numbers that may already be on the end..
        if (prefix.trim().endsWithChar (')'))
        {
            putNumbersInBrackets = true;

            const int openBracks  = prefix.lastIndexOfChar ('(');
            const int closeBracks = prefix.lastIndexOfChar (')');

            if (openBracks > 0
                 && closeBracks > openBracks
                 && prefix.substring (openBracks + 1, closeBracks).containsOnly ("0123456789"))
            {
                number = prefix.substring (openBracks + 1, closeBracks).getIntValue();
                prefix = prefix.substring (0, openBracks);
            }
        }

        do
        {
            String newName (prefix);

            if (putNumbersInBrackets)
            {
                newName << '(' << ++number << ')';
            }
            else
            {
                if (CharacterFunctions::isDigit (prefix.getLastCharacter()))
                    newName << '_'; // pad with an underscore if the name already ends in a digit

                newName << ++number;
            }

            f = getChildFile (newName + suffix);

        } while (f.exists());
    }

    return f;
}

File File::getNonexistentSibling (const bool putNumbersInBrackets) const
{
    if (! exists())
        return *this;

    return getParentDirectory().getNonexistentChildFile (getFileNameWithoutExtension(),
                                                         getFileExtension(),
                                                         putNumbersInBrackets);
}

//==============================================================================
String File::getFileExtension() const
{
    const int indexOfDot = fullPath.lastIndexOfChar ('.');

    if (indexOfDot > fullPath.lastIndexOfChar (separator))
        return fullPath.substring (indexOfDot);

    return String();
}

bool File::hasFileExtension (StringRef possibleSuffix) const
{
    if (possibleSuffix.isEmpty())
        return fullPath.lastIndexOfChar ('.') <= fullPath.lastIndexOfChar (separator);

    const int semicolon = possibleSuffix.text.indexOf ((water_uchar) ';');

    if (semicolon >= 0)
        return hasFileExtension (String (possibleSuffix.text).substring (0, semicolon).trimEnd())
                || hasFileExtension ((possibleSuffix.text + (semicolon + 1)).findEndOfWhitespace());

    if (fullPath.endsWithIgnoreCase (possibleSuffix))
    {
        if (possibleSuffix.text[0] == '.')
            return true;

        const int dotPos = fullPath.length() - possibleSuffix.length() - 1;

        if (dotPos >= 0)
            return fullPath [dotPos] == '.';
    }

    return false;
}

File File::withFileExtension (StringRef newExtension) const
{
    if (fullPath.isEmpty())
        return File();

    String filePart (getFileName());

    const int i = filePart.lastIndexOfChar ('.');
    if (i >= 0)
        filePart = filePart.substring (0, i);

    if (newExtension.isNotEmpty() && newExtension.text[0] != '.')
        filePart << '.';

    return getSiblingFile (filePart + newExtension);
}

//==============================================================================
FileInputStream* File::createInputStream() const
{
    ScopedPointer<FileInputStream> fin (new FileInputStream (*this));

    if (fin->openedOk())
        return fin.release();

    return nullptr;
}

FileOutputStream* File::createOutputStream (const size_t bufferSize) const
{
    ScopedPointer<FileOutputStream> out (new FileOutputStream (*this, bufferSize));

    return out->failedToOpen() ? nullptr
                               : out.release();
}

//==============================================================================
bool File::appendData (const void* const dataToAppend,
                       const size_t numberOfBytes) const
{
    jassert (((ssize_t) numberOfBytes) >= 0);

    if (numberOfBytes == 0)
        return true;

    FileOutputStream out (*this, 8192);
    return out.openedOk() && out.write (dataToAppend, numberOfBytes);
}

bool File::replaceWithData (const void* const dataToWrite,
                            const size_t numberOfBytes) const
{
    if (numberOfBytes == 0)
        return deleteFile();

    TemporaryFile tempFile (*this, TemporaryFile::useHiddenFile);
    tempFile.getFile().appendData (dataToWrite, numberOfBytes);
    return tempFile.overwriteTargetFileWithTemporary();
}

bool File::appendText (const String& text,
                       const bool asUnicode,
                       const bool writeUnicodeHeaderBytes) const
{
    FileOutputStream out (*this);
    CARLA_SAFE_ASSERT_RETURN (! out.failedToOpen(), false);

    out.writeText (text, asUnicode, writeUnicodeHeaderBytes);
    return true;
}

bool File::replaceWithText (const String& textToWrite,
                            const bool asUnicode,
                            const bool writeUnicodeHeaderBytes) const
{
    TemporaryFile tempFile (*this, TemporaryFile::useHiddenFile);
    tempFile.getFile().appendText (textToWrite, asUnicode, writeUnicodeHeaderBytes);
    return tempFile.overwriteTargetFileWithTemporary();
}

bool File::hasIdenticalContentTo (const File& other) const
{
    if (other == *this)
        return true;

    if (getSize() == other.getSize() && existsAsFile() && other.existsAsFile())
    {
        FileInputStream in1 (*this), in2 (other);

        if (in1.openedOk() && in2.openedOk())
        {
            const int bufferSize = 4096;
            HeapBlock<char> buffer1, buffer2;

            CARLA_SAFE_ASSERT_RETURN(buffer1.malloc (bufferSize), false);
            CARLA_SAFE_ASSERT_RETURN(buffer2.malloc (bufferSize), false);

            for (;;)
            {
                const int num1 = in1.read (buffer1, bufferSize);
                const int num2 = in2.read (buffer2, bufferSize);

                if (num1 != num2)
                    break;

                if (num1 <= 0)
                    return true;

                if (memcmp (buffer1, buffer2, (size_t) num1) != 0)
                    break;
            }
        }
    }

    return false;
}

//==============================================================================
String File::createLegalPathName (const String& original)
{
    String s (original);
    String start;

    if (s.isNotEmpty() && s[1] == ':')
    {
        start = s.substring (0, 2);
        s = s.substring (2);
    }

    return start + s.removeCharacters ("\"#@,;:<>*^|?")
                    .substring (0, 1024);
}

String File::createLegalFileName (const String& original)
{
    String s (original.removeCharacters ("\"#@,;:<>*^|?\\/"));

    const int maxLength = 128; // only the length of the filename, not the whole path
    const int len = s.length();

    if (len > maxLength)
    {
        const int lastDot = s.lastIndexOfChar ('.');

        if (lastDot > jmax (0, len - 12))
        {
            s = s.substring (0, maxLength - (len - lastDot))
                 + s.substring (lastDot);
        }
        else
        {
            s = s.substring (0, maxLength);
        }
    }

    return s;
}

//==============================================================================
static int countNumberOfSeparators (String::CharPointerType s)
{
    int num = 0;

    for (;;)
    {
        const water_uchar c = s.getAndAdvance();

        if (c == 0)
            break;

        if (c == File::separator)
            ++num;
    }

    return num;
}

String File::getRelativePathFrom (const File& dir)  const
{
    String thisPath (fullPath);

    while (thisPath.endsWithChar (separator))
        thisPath = thisPath.dropLastCharacters (1);

    String dirPath (addTrailingSeparator (dir.existsAsFile() ? dir.getParentDirectory().getFullPathName()
                                                             : dir.fullPath));

    int commonBitLength = 0;
    String::CharPointerType thisPathAfterCommon (thisPath.getCharPointer());
    String::CharPointerType dirPathAfterCommon  (dirPath.getCharPointer());

    {
        String::CharPointerType thisPathIter (thisPath.getCharPointer());
        String::CharPointerType dirPathIter  (dirPath.getCharPointer());

        for (int i = 0;;)
        {
            const water_uchar c1 = thisPathIter.getAndAdvance();
            const water_uchar c2 = dirPathIter.getAndAdvance();

           #if NAMES_ARE_CASE_SENSITIVE
            if (c1 != c2
           #else
            if ((c1 != c2 && CharacterFunctions::toLowerCase (c1) != CharacterFunctions::toLowerCase (c2))
           #endif
                 || c1 == 0)
                break;

            ++i;

            if (c1 == separator)
            {
                thisPathAfterCommon = thisPathIter;
                dirPathAfterCommon  = dirPathIter;
                commonBitLength = i;
            }
        }
    }

    // if the only common bit is the root, then just return the full path..
    if (commonBitLength == 0 || (commonBitLength == 1 && thisPath[1] == separator))
        return fullPath;

    const int numUpDirectoriesNeeded = countNumberOfSeparators (dirPathAfterCommon);

    if (numUpDirectoriesNeeded == 0)
        return thisPathAfterCommon;

   #ifdef CARLA_OS_WIN
    String s (String::repeatedString ("..\\", numUpDirectoriesNeeded));
   #else
    String s (String::repeatedString ("../",  numUpDirectoriesNeeded));
   #endif
    s.appendCharPointer (thisPathAfterCommon);
    return s;
}

//==============================================================================
File File::createTempFile (StringRef fileNameEnding)
{
    const File tempFile (getSpecialLocation (tempDirectory)
                            .getChildFile ("temp_" + String::toHexString (Random::getSystemRandom().nextInt()))
                            .withFileExtension (fileNameEnding));

    if (tempFile.exists())
        return createTempFile (fileNameEnding);

    return tempFile;
}

bool File::createSymbolicLink (const File& linkFileToCreate, bool overwriteExisting) const
{
    if (linkFileToCreate.exists())
    {
        if (! linkFileToCreate.isSymbolicLink())
        {
            // user has specified an existing file / directory as the link
            // this is bad! the user could end up unintentionally destroying data
            jassertfalse;
            return false;
        }

        if (overwriteExisting)
            linkFileToCreate.deleteFile();
    }

   #ifndef CARLA_OS_WIN
    // one common reason for getting an error here is that the file already exists
    if (symlink (fullPath.toRawUTF8(), linkFileToCreate.getFullPathName().toRawUTF8()) == -1)
    {
        jassertfalse;
        return false;
    }

    return true;
   #elif _MSVC_VER
    return CreateSymbolicLink (linkFileToCreate.getFullPathName().toUTF8(),
                               fullPath.toUTF8(),
                               isDirectory() ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0) != FALSE;
   #else
    jassertfalse; // symbolic links not supported on this platform!
    return false;
   #endif
}

#if 0
//=====================================================================================================================
MemoryMappedFile::MemoryMappedFile (const File& file, MemoryMappedFile::AccessMode mode, bool exclusive)
    : address (nullptr), range (0, file.getSize()), fileHandle (0)
{
    openInternal (file, mode, exclusive);
}

MemoryMappedFile::MemoryMappedFile (const File& file, const Range<int64>& fileRange, AccessMode mode, bool exclusive)
    : address (nullptr), range (fileRange.getIntersectionWith (Range<int64> (0, file.getSize()))), fileHandle (0)
{
    openInternal (file, mode, exclusive);
}
#endif

//=====================================================================================================================
#ifdef CARLA_OS_WIN
namespace WindowsFileHelpers
{
    DWORD getAtts (const String& path)
    {
        return GetFileAttributes (path.toUTF8());
    }

    int64 fileTimeToTime (const FILETIME* const ft)
    {
        static_jassert (sizeof (ULARGE_INTEGER) == sizeof (FILETIME)); // tell me if this fails!

        return (int64) ((reinterpret_cast<const ULARGE_INTEGER*> (ft)->QuadPart - 116444736000000000LL) / 10000);
    }

    File getSpecialFolderPath (int type)
    {
        CHAR path [MAX_PATH + 256];

        if (SHGetSpecialFolderPath (0, path, type, FALSE))
            return File (String (path));

        return File();
    }

    File getModuleFileName (HINSTANCE moduleHandle)
    {
        CHAR dest [MAX_PATH + 256];
        dest[0] = 0;
        GetModuleFileName (moduleHandle, dest, (DWORD) numElementsInArray (dest));
        return File (String (dest));
    }
}

const water_uchar File::separator = '\\';
const String File::separatorString ("\\");

bool File::isDirectory() const
{
    const DWORD attr = WindowsFileHelpers::getAtts (fullPath);
    return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0 && attr != INVALID_FILE_ATTRIBUTES;
}

bool File::exists() const
{
    return fullPath.isNotEmpty()
            && WindowsFileHelpers::getAtts (fullPath) != INVALID_FILE_ATTRIBUTES;
}

bool File::existsAsFile() const
{
    return fullPath.isNotEmpty()
            && (WindowsFileHelpers::getAtts (fullPath) & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool File::hasWriteAccess() const
{
    if (fullPath.isEmpty())
        return true;

    const DWORD attr = WindowsFileHelpers::getAtts (fullPath);

    // NB: According to MS, the FILE_ATTRIBUTE_READONLY attribute doesn't work for
    // folders, and can be incorrectly set for some special folders, so we'll just say
    // that folders are always writable.
    return attr == INVALID_FILE_ATTRIBUTES
            || (attr & FILE_ATTRIBUTE_DIRECTORY) != 0
            || (attr & FILE_ATTRIBUTE_READONLY) == 0;
}

int64 File::getSize() const
{
    WIN32_FILE_ATTRIBUTE_DATA attributes;

    if (GetFileAttributesEx (fullPath.toUTF8(), GetFileExInfoStandard, &attributes))
        return (((int64) attributes.nFileSizeHigh) << 32) | attributes.nFileSizeLow;

    return 0;
}

bool File::deleteFile() const
{
    if (! exists())
        return true;

    return isDirectory() ? RemoveDirectory (fullPath.toUTF8()) != 0
                         : DeleteFile (fullPath.toUTF8()) != 0;
}

bool File::copyInternal (const File& dest) const
{
    return CopyFile (fullPath.toUTF8(), dest.getFullPathName().toUTF8(), false) != 0;
}

bool File::moveInternal (const File& dest) const
{
    return MoveFile (fullPath.toUTF8(), dest.getFullPathName().toUTF8()) != 0;
}

bool File::replaceInternal (const File& dest) const
{
    void* lpExclude = 0;
    void* lpReserved = 0;

    return ReplaceFile (dest.getFullPathName().toUTF8(), fullPath.toUTF8(),
                        0, REPLACEFILE_IGNORE_MERGE_ERRORS, lpExclude, lpReserved) != 0;
}

Result File::createDirectoryInternal (const String& fileName) const
{
    return CreateDirectory (fileName.toUTF8(), 0) ? Result::ok()
                                                  : getResultForLastError();
}

File File::getCurrentWorkingDirectory()
{
    CHAR dest [MAX_PATH + 256];
    dest[0] = 0;
    GetCurrentDirectory ((DWORD) numElementsInArray (dest), dest);
    return File (String (dest));
}

bool File::isSymbolicLink() const
{
    return (GetFileAttributes (fullPath.toUTF8()) & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

File File::getSpecialLocation (const SpecialLocationType type)
{
    int csidlType = 0;

    switch (type)
    {
        case userHomeDirectory:                 csidlType = CSIDL_PROFILE; break;

        case tempDirectory:
        {
            CHAR dest [2048];
            dest[0] = 0;
            GetTempPath ((DWORD) numElementsInArray (dest), dest);
            return File (String (dest));
        }

        case currentExecutableFile:
            return WindowsFileHelpers::getModuleFileName (water_getCurrentModuleInstanceHandle());

        case hostApplicationPath:
            return WindowsFileHelpers::getModuleFileName (nullptr);

        default:
            jassertfalse; // unknown type?
            return File();
    }

    return WindowsFileHelpers::getSpecialFolderPath (csidlType);
}

//=====================================================================================================================
class DirectoryIterator::NativeIterator::Pimpl
{
public:
    Pimpl (const File& directory, const String& wildCard)
        : directoryWithWildCard (directory.getFullPathName().isNotEmpty() ? File::addTrailingSeparator (directory.getFullPathName()) + wildCard : String()),
          handle (INVALID_HANDLE_VALUE)
    {
    }

    ~Pimpl()
    {
        if (handle != INVALID_HANDLE_VALUE)
            FindClose (handle);
    }

    bool next (String& filenameFound,
               bool* const isDir, int64* const fileSize,
               Time* const modTime, Time* const creationTime, bool* const isReadOnly)
    {
        using namespace WindowsFileHelpers;
        WIN32_FIND_DATA findData;

        if (handle == INVALID_HANDLE_VALUE)
        {
            handle = FindFirstFile (directoryWithWildCard.toUTF8(), &findData);

            if (handle == INVALID_HANDLE_VALUE)
                return false;
        }
        else
        {
            if (FindNextFile (handle, &findData) == 0)
                return false;
        }

        filenameFound = findData.cFileName;

        if (isDir != nullptr)         *isDir        = ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
        if (isReadOnly != nullptr)    *isReadOnly   = ((findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0);
        if (fileSize != nullptr)      *fileSize     = findData.nFileSizeLow + (((int64) findData.nFileSizeHigh) << 32);
        if (modTime != nullptr)       *modTime      = Time (fileTimeToTime (&findData.ftLastWriteTime));
        if (creationTime != nullptr)  *creationTime = Time (fileTimeToTime (&findData.ftCreationTime));

        return true;
    }

private:
    const String directoryWithWildCard;
    HANDLE handle;

    CARLA_DECLARE_NON_COPY_CLASS (Pimpl)
};
#else
//=====================================================================================================================
namespace
{
   #ifdef CARLA_OS_LINUX
    typedef struct stat64 water_statStruct;
    #define WATER_STAT    stat64
   #else
    typedef struct stat   water_statStruct;
    #define WATER_STAT    stat
   #endif

    bool water_stat (const String& fileName, water_statStruct& info)
    {
        return fileName.isNotEmpty()
                 && WATER_STAT (fileName.toUTF8(), &info) == 0;
    }

   #if 0 //def CARLA_OS_MAC
    static int64 getCreationTime (const water_statStruct& s) noexcept     { return (int64) s.st_birthtime; }
   #else
    static int64 getCreationTime (const water_statStruct& s) noexcept     { return (int64) s.st_ctime; }
   #endif

    void updateStatInfoForFile (const String& path, bool* const isDir, int64* const fileSize,
                                Time* const modTime, Time* const creationTime, bool* const isReadOnly)
    {
        if (isDir != nullptr || fileSize != nullptr || modTime != nullptr || creationTime != nullptr)
        {
            water_statStruct info;
            const bool statOk = water_stat (path, info);

            if (isDir != nullptr)         *isDir        = statOk && ((info.st_mode & S_IFDIR) != 0);
            if (fileSize != nullptr)      *fileSize     = statOk ? (int64) info.st_size : 0;
            if (modTime != nullptr)       *modTime      = Time (statOk ? (int64) info.st_mtime  * 1000 : 0);
            if (creationTime != nullptr)  *creationTime = Time (statOk ? getCreationTime (info) * 1000 : 0);
        }

        if (isReadOnly != nullptr)
            *isReadOnly = access (path.toUTF8(), W_OK) != 0;
    }

    Result getResultForReturnValue (int value)
    {
        return value == -1 ? getResultForErrno() : Result::ok();
    }
}

const water_uchar File::separator = '/';
const String File::separatorString ("/");

bool File::isDirectory() const
{
    water_statStruct info;

    return fullPath.isNotEmpty()
             && (water_stat (fullPath, info) && ((info.st_mode & S_IFDIR) != 0));
}

bool File::exists() const
{
    return fullPath.isNotEmpty()
             && access (fullPath.toUTF8(), F_OK) == 0;
}

bool File::existsAsFile() const
{
    return exists() && ! isDirectory();
}

bool File::hasWriteAccess() const
{
    if (exists())
        return access (fullPath.toUTF8(), W_OK) == 0;

    if ((! isDirectory()) && fullPath.containsChar (separator))
        return getParentDirectory().hasWriteAccess();

    return false;
}

int64 File::getSize() const
{
    water_statStruct info;
    return water_stat (fullPath, info) ? info.st_size : 0;
}

bool File::deleteFile() const
{
    if (! exists() && ! isSymbolicLink())
        return true;

    if (isDirectory())
        return rmdir (fullPath.toUTF8()) == 0;

    return remove (fullPath.toUTF8()) == 0;
}

bool File::moveInternal (const File& dest) const
{
    if (rename (fullPath.toUTF8(), dest.getFullPathName().toUTF8()) == 0)
        return true;

    if (hasWriteAccess() && copyInternal (dest))
    {
        if (deleteFile())
            return true;

        dest.deleteFile();
    }

    return false;
}

bool File::replaceInternal (const File& dest) const
{
    return moveInternal (dest);
}

Result File::createDirectoryInternal (const String& fileName) const
{
    return getResultForReturnValue (mkdir (fileName.toUTF8(), 0777));
}

File File::getCurrentWorkingDirectory()
{
    HeapBlock<char> heapBuffer;

    char localBuffer [1024];
    char* cwd = getcwd (localBuffer, sizeof (localBuffer) - 1);

    size_t bufferSize = 4096;
    while (cwd == nullptr && errno == ERANGE)
    {
        CARLA_SAFE_ASSERT_RETURN(heapBuffer.malloc (bufferSize), File());

        cwd = getcwd (heapBuffer, bufferSize - 1);
        bufferSize += 1024;
    }

    return File (CharPointer_UTF8 (cwd));
}

File water_getExecutableFile();
File water_getExecutableFile()
{
    struct DLAddrReader
    {
        static String getFilename()
        {
            Dl_info exeInfo;
            void* localSymbol = (void*) water_getExecutableFile;
            dladdr (localSymbol, &exeInfo);
            const CharPointer_UTF8 filename (exeInfo.dli_fname);

            // if the filename is absolute simply return it
            if (File::isAbsolutePath (filename))
                return filename;

            // if the filename is relative construct from CWD
            if (filename[0] == '.')
                return File::getCurrentWorkingDirectory().getChildFile (filename).getFullPathName();

            // filename is abstract, look up in PATH
            if (const char* const envpath = ::getenv ("PATH"))
            {
                StringArray paths (StringArray::fromTokens (envpath, ":", ""));

                for (int i=paths.size(); --i>=0;)
                {
                    const File filepath (File (paths[i]).getChildFile (filename));

                    if (filepath.existsAsFile())
                        return filepath.getFullPathName();
                }
            }

            // if we reach this, we failed to find ourselves...
            jassertfalse;
            return filename;
        }
    };

    static String filename (DLAddrReader::getFilename());
    return filename;
}

#ifdef CARLA_OS_MAC
static NSString* getFileLink (const String& path)
{
    return [[NSFileManager defaultManager] destinationOfSymbolicLinkAtPath: waterStringToNS (path) error: nil];
}

bool File::isSymbolicLink() const
{
    // FIXME
    return false;
    //return getFileLink (fullPath) != nil;
}

File File::getLinkedTarget() const
{
    if (NSString* dest = getFileLink (fullPath))
        return getSiblingFile (nsStringToWater (dest));

    return *this;
}

bool File::copyInternal (const File& dest) const
{
    //@autoreleasepool
    {
        NSFileManager* fm = [NSFileManager defaultManager];

        return [fm fileExistsAtPath: waterStringToNS (fullPath)]
               #if defined (MAC_OS_X_VERSION_10_6) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
                && [fm copyItemAtPath: waterStringToNS (fullPath)
                               toPath: waterStringToNS (dest.getFullPathName())
                                error: nil];
               #else
                && [fm copyPath: waterStringToNS (fullPath)
                         toPath: waterStringToNS (dest.getFullPathName())
                        handler: nil];
               #endif
    }
}

File File::getSpecialLocation (const SpecialLocationType type)
{
    //@autoreleasepool
    {
        String resultPath;

        switch (type)
        {
            case userHomeDirectory:                 resultPath = nsStringToWater (NSHomeDirectory()); break;

            case tempDirectory:
            {
                File tmp ("~/Library/Caches/" + water_getExecutableFile().getFileNameWithoutExtension());
                tmp.createDirectory();
                return File (tmp.getFullPathName());
            }

            case currentExecutableFile:
                return water_getExecutableFile();

            case hostApplicationPath:
            {
                unsigned int size = 8192;
                HeapBlock<char> buffer;
                buffer.calloc (size + 8);

                _NSGetExecutablePath (buffer.getData(), &size);
                return File (String::fromUTF8 (buffer, (int) size));
            }

            default:
                jassertfalse; // unknown type?
                break;
        }

        if (resultPath.isNotEmpty())
            return File (resultPath.convertToPrecomposedUnicode());
    }

    return File();
}
//==============================================================================
class DirectoryIterator::NativeIterator::Pimpl
{
public:
    Pimpl (const File& directory, const String& wildCard_)
        : parentDir (File::addTrailingSeparator (directory.getFullPathName())),
          wildCard (wildCard_),
          enumerator (nil)
    {
        //@autoreleasepool
        {
            enumerator = [[[NSFileManager defaultManager] enumeratorAtPath: waterStringToNS (directory.getFullPathName())] retain];
        }
    }

    ~Pimpl()
    {
        [enumerator release];
    }

    bool next (String& filenameFound,
               bool* const isDir, int64* const fileSize,
               Time* const modTime, Time* const creationTime, bool* const isReadOnly)
    {
        //@autoreleasepool
        {
            const char* wildcardUTF8 = nullptr;

            for (;;)
            {
                NSString* file;
                if (enumerator == nil || (file = [enumerator nextObject]) == nil)
                    return false;

                [enumerator skipDescendents];
                filenameFound = nsStringToWater (file).convertToPrecomposedUnicode();

                if (wildcardUTF8 == nullptr)
                    wildcardUTF8 = wildCard.toUTF8();

                if (fnmatch (wildcardUTF8, filenameFound.toUTF8(), FNM_CASEFOLD) != 0)
                    continue;

                const String fullPath (parentDir + filenameFound);
                updateStatInfoForFile (fullPath, isDir, fileSize, modTime, creationTime, isReadOnly);

                return true;
            }
        }
    }

private:
    String parentDir, wildCard;
    NSDirectoryEnumerator* enumerator;

    CARLA_DECLARE_NON_COPY_CLASS (Pimpl)
};
#else
static String getLinkedFile (const String& file)
{
    HeapBlock<char> buffer;
    CARLA_SAFE_ASSERT_RETURN(buffer.malloc(8194), String());

    const int numBytes = (int) readlink (file.toRawUTF8(), buffer, 8192);
    return String::fromUTF8 (buffer, jmax (0, numBytes));
}

bool File::isSymbolicLink() const
{
    return getLinkedFile (getFullPathName()).isNotEmpty();
}

File File::getLinkedTarget() const
{
    String f (getLinkedFile (getFullPathName()));

    if (f.isNotEmpty())
        return getSiblingFile (f);

    return *this;
}

bool File::copyInternal (const File& dest) const
{
    FileInputStream in (*this);

    if (dest.deleteFile())
    {
        {
            FileOutputStream out (dest);

            if (out.failedToOpen())
                return false;

            if (out.writeFromInputStream (in, -1) == getSize())
                return true;
        }

        dest.deleteFile();
    }

    return false;
}

File File::getSpecialLocation (const SpecialLocationType type)
{
    switch (type)
    {
        case userHomeDirectory:
        {
            if (const char* homeDir = getenv ("HOME"))
                return File (CharPointer_UTF8 (homeDir));

            if (struct passwd* const pw = getpwuid (getuid()))
                return File (CharPointer_UTF8 (pw->pw_dir));

            return File();
        }

        case tempDirectory:
        {
            File tmp ("/var/tmp");

            if (! tmp.isDirectory())
            {
                tmp = "/tmp";

                if (! tmp.isDirectory())
                    tmp = File::getCurrentWorkingDirectory();
            }

            return tmp;
        }

        case currentExecutableFile:
            return water_getExecutableFile();

        case hostApplicationPath:
        {
            const File f ("/proc/self/exe");
            return f.isSymbolicLink() ? f.getLinkedTarget() : water_getExecutableFile();
        }

        default:
            jassertfalse; // unknown type?
            break;
    }

    return File();
}
//==============================================================================
class DirectoryIterator::NativeIterator::Pimpl
{
public:
    Pimpl (const File& directory, const String& wc)
        : parentDir (File::addTrailingSeparator (directory.getFullPathName())),
          wildCard (wc), dir (opendir (directory.getFullPathName().toUTF8()))
    {
    }

    ~Pimpl()
    {
        if (dir != nullptr)
            closedir (dir);
    }

    bool next (String& filenameFound,
               bool* const isDir, int64* const fileSize,
               Time* const modTime, Time* const creationTime, bool* const isReadOnly)
    {
        if (dir != nullptr)
        {
            const char* wildcardUTF8 = nullptr;

            for (;;)
            {
                struct dirent* const de = readdir (dir);

                if (de == nullptr)
                    break;

                if (wildcardUTF8 == nullptr)
                    wildcardUTF8 = wildCard.toUTF8();

                if (fnmatch (wildcardUTF8, de->d_name, FNM_CASEFOLD) == 0)
                {
                    filenameFound = CharPointer_UTF8 (de->d_name);

                    updateStatInfoForFile (parentDir + filenameFound, isDir, fileSize, modTime, creationTime, isReadOnly);

                    return true;
                }
            }
        }

        return false;
    }

private:
    String parentDir, wildCard;
    DIR* dir;

    CARLA_DECLARE_NON_COPY_CLASS (Pimpl)
};
#endif
#endif

DirectoryIterator::NativeIterator::NativeIterator (const File& directory, const String& wildCardStr)
    : pimpl (new DirectoryIterator::NativeIterator::Pimpl (directory, wildCardStr))
{
}

DirectoryIterator::NativeIterator::~NativeIterator() {}

bool DirectoryIterator::NativeIterator::next (String& filenameFound,
                                              bool* isDir, int64* fileSize,
                                              Time* modTime, Time* creationTime, bool* isReadOnly)
{
    return pimpl->next (filenameFound, isDir, fileSize, modTime, creationTime, isReadOnly);
}

}
