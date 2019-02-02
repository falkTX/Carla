/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

CriticalSection::CriticalSection() noexcept
{
    pthread_mutexattr_t atts;
    pthread_mutexattr_init (&atts);
    pthread_mutexattr_settype (&atts, PTHREAD_MUTEX_RECURSIVE);
   #if ! JUCE_ANDROID
    pthread_mutexattr_setprotocol (&atts, PTHREAD_PRIO_INHERIT);
   #endif
    pthread_mutex_init (&lock, &atts);
    pthread_mutexattr_destroy (&atts);
}

CriticalSection::~CriticalSection() noexcept        { pthread_mutex_destroy (&lock); }
void CriticalSection::enter() const noexcept        { pthread_mutex_lock (&lock); }
bool CriticalSection::tryEnter() const noexcept     { return pthread_mutex_trylock (&lock) == 0; }
void CriticalSection::exit() const noexcept         { pthread_mutex_unlock (&lock); }

//==============================================================================
WaitableEvent::WaitableEvent (const bool useManualReset) noexcept
    : triggered (false), manualReset (useManualReset)
{
    pthread_cond_init (&condition, 0);

    pthread_mutexattr_t atts;
    pthread_mutexattr_init (&atts);
   #if ! JUCE_ANDROID
    pthread_mutexattr_setprotocol (&atts, PTHREAD_PRIO_INHERIT);
   #endif
    pthread_mutex_init (&mutex, &atts);
    pthread_mutexattr_destroy (&atts);
}

WaitableEvent::~WaitableEvent() noexcept
{
    pthread_cond_destroy (&condition);
    pthread_mutex_destroy (&mutex);
}

bool WaitableEvent::wait (const int timeOutMillisecs) const noexcept
{
    pthread_mutex_lock (&mutex);

    if (! triggered)
    {
        if (timeOutMillisecs < 0)
        {
            do
            {
                pthread_cond_wait (&condition, &mutex);
            }
            while (! triggered);
        }
        else
        {
            struct timeval now;
            gettimeofday (&now, 0);

            struct timespec time;
            time.tv_sec  = now.tv_sec  + (timeOutMillisecs / 1000);
            time.tv_nsec = (now.tv_usec + ((timeOutMillisecs % 1000) * 1000)) * 1000;

            if (time.tv_nsec >= 1000000000)
            {
                time.tv_nsec -= 1000000000;
                time.tv_sec++;
            }

            do
            {
                if (pthread_cond_timedwait (&condition, &mutex, &time) == ETIMEDOUT)
                {
                    pthread_mutex_unlock (&mutex);
                    return false;
                }
            }
            while (! triggered);
        }
    }

    if (! manualReset)
        triggered = false;

    pthread_mutex_unlock (&mutex);
    return true;
}

void WaitableEvent::signal() const noexcept
{
    pthread_mutex_lock (&mutex);

    if (! triggered)
    {
        triggered = true;
        pthread_cond_broadcast (&condition);
    }

    pthread_mutex_unlock (&mutex);
}

void WaitableEvent::reset() const noexcept
{
    pthread_mutex_lock (&mutex);
    triggered = false;
    pthread_mutex_unlock (&mutex);
}

//==============================================================================
void JUCE_CALLTYPE Thread::sleep (int millisecs)
{
    struct timespec time;
    time.tv_sec = millisecs / 1000;
    time.tv_nsec = (millisecs % 1000) * 1000000;
    nanosleep (&time, nullptr);
}

void JUCE_CALLTYPE Process::terminate()
{
   #if JUCE_ANDROID
    _exit (EXIT_FAILURE);
   #else
    std::_Exit (EXIT_FAILURE);
   #endif
}


#if JUCE_MAC || JUCE_LINUX
bool Process::setMaxNumberOfFileHandles (int newMaxNumber) noexcept
{
    rlimit lim;
    if (getrlimit (RLIMIT_NOFILE, &lim) == 0)
    {
        if (newMaxNumber <= 0 && lim.rlim_cur == RLIM_INFINITY && lim.rlim_max == RLIM_INFINITY)
            return true;

        if (lim.rlim_cur >= (rlim_t) newMaxNumber)
            return true;
    }

    lim.rlim_cur = lim.rlim_max = newMaxNumber <= 0 ? RLIM_INFINITY : (rlim_t) newMaxNumber;
    return setrlimit (RLIMIT_NOFILE, &lim) == 0;
}

struct MaxNumFileHandlesInitialiser
{
    MaxNumFileHandlesInitialiser() noexcept
    {
       #ifndef JUCE_PREFERRED_MAX_FILE_HANDLES
        enum { JUCE_PREFERRED_MAX_FILE_HANDLES = 8192 };
       #endif

        // Try to give our app a decent number of file handles by default
        if (! Process::setMaxNumberOfFileHandles (0))
        {
            for (int num = JUCE_PREFERRED_MAX_FILE_HANDLES; num > 256; num -= 1024)
                if (Process::setMaxNumberOfFileHandles (num))
                    break;
        }
    }
};

static MaxNumFileHandlesInitialiser maxNumFileHandlesInitialiser;
#endif

//==============================================================================
const juce_wchar File::separator = '/';
const String File::separatorString ("/");

//==============================================================================
File File::getCurrentWorkingDirectory()
{
    HeapBlock<char> heapBuffer;

    char localBuffer [1024];
    char* cwd = getcwd (localBuffer, sizeof (localBuffer) - 1);
    size_t bufferSize = 4096;

    while (cwd == nullptr && errno == ERANGE)
    {
        heapBuffer.malloc (bufferSize);
        cwd = getcwd (heapBuffer, bufferSize - 1);
        bufferSize += 1024;
    }

    return File (CharPointer_UTF8 (cwd));
}

bool File::setAsCurrentWorkingDirectory() const
{
    return chdir (getFullPathName().toUTF8()) == 0;
}

#if JUCE_ANDROID
 typedef unsigned long juce_sigactionflags_type;
#else
 typedef int juce_sigactionflags_type;
#endif

//==============================================================================
// The unix siginterrupt function is deprecated - this does the same job.
int juce_siginterrupt (int sig, int flag)
{
    struct ::sigaction act;
    (void) ::sigaction (sig, nullptr, &act);

    if (flag != 0)
        act.sa_flags &= static_cast<juce_sigactionflags_type> (~SA_RESTART);
    else
        act.sa_flags |= static_cast<juce_sigactionflags_type> (SA_RESTART);

    return ::sigaction (sig, &act, nullptr);
}

//==============================================================================
namespace
{
   #if JUCE_LINUX || (JUCE_IOS && ! __DARWIN_ONLY_64_BIT_INO_T) // (this iOS stuff is to avoid a simulator bug)
    typedef struct stat64 juce_statStruct;
    #define JUCE_STAT     stat64
   #else
    typedef struct stat   juce_statStruct;
    #define JUCE_STAT     stat
   #endif

    bool juce_stat (const String& fileName, juce_statStruct& info)
    {
        return fileName.isNotEmpty()
                 && JUCE_STAT (fileName.toUTF8(), &info) == 0;
    }

    // if this file doesn't exist, find a parent of it that does..
    bool juce_doStatFS (File f, struct statfs& result)
    {
        for (int i = 5; --i >= 0;)
        {
            if (f.exists())
                break;

            f = f.getParentDirectory();
        }

        return statfs (f.getFullPathName().toUTF8(), &result) == 0;
    }

   #if (JUCE_MAC && MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_5) || JUCE_IOS
    static int64 getCreationTime (const juce_statStruct& s) noexcept     { return (int64) s.st_birthtime; }
   #else
    static int64 getCreationTime (const juce_statStruct& s) noexcept     { return (int64) s.st_ctime; }
   #endif

    void updateStatInfoForFile (const String& path, bool* const isDir, int64* const fileSize,
                                Time* const modTime, Time* const creationTime, bool* const isReadOnly)
    {
        if (isDir != nullptr || fileSize != nullptr || modTime != nullptr || creationTime != nullptr)
        {
            juce_statStruct info;
            const bool statOk = juce_stat (path, info);

            if (isDir != nullptr)         *isDir        = statOk && ((info.st_mode & S_IFDIR) != 0);
            if (fileSize != nullptr)      *fileSize     = statOk ? (int64) info.st_size : 0;
            if (modTime != nullptr)       *modTime      = Time (statOk ? (int64) info.st_mtime  * 1000 : 0);
            if (creationTime != nullptr)  *creationTime = Time (statOk ? getCreationTime (info) * 1000 : 0);
        }

        if (isReadOnly != nullptr)
            *isReadOnly = access (path.toUTF8(), W_OK) != 0;
    }

    Result getResultForErrno()
    {
        return Result::fail (String (strerror (errno)));
    }

    Result getResultForReturnValue (int value)
    {
        return value == -1 ? getResultForErrno() : Result::ok();
    }

    int getFD (void* handle) noexcept        { return (int) (pointer_sized_int) handle; }
    void* fdToVoidPointer (int fd) noexcept  { return (void*) (pointer_sized_int) fd; }
}

bool File::isDirectory() const
{
    juce_statStruct info;

    return fullPath.isNotEmpty()
             && (juce_stat (fullPath, info) && ((info.st_mode & S_IFDIR) != 0));
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

int64 File::getSize() const
{
    juce_statStruct info;
    return juce_stat (fullPath, info) ? info.st_size : 0;
}

uint64 File::getFileIdentifier() const
{
    juce_statStruct info;
    return juce_stat (fullPath, info) ? (uint64) info.st_ino : 0;
}

static bool hasEffectiveRootFilePermissions()
{
   #if JUCE_LINUX
    return (geteuid() == 0);
   #else
    return false;
   #endif
}

//==============================================================================
bool File::hasWriteAccess() const
{
    if (exists())
        return (hasEffectiveRootFilePermissions()
             || access (fullPath.toUTF8(), W_OK) == 0);

    if ((! isDirectory()) && fullPath.containsChar (separator))
        return getParentDirectory().hasWriteAccess();

    return false;
}

static bool setFileModeFlags (const String& fullPath, mode_t flags, bool shouldSet) noexcept
{
    juce_statStruct info;
    if (! juce_stat (fullPath, info))
        return false;

    info.st_mode &= 0777;

    if (shouldSet)
        info.st_mode |= flags;
    else
        info.st_mode &= ~flags;

    return chmod (fullPath.toUTF8(), (mode_t) info.st_mode) == 0;
}

bool File::setFileReadOnlyInternal (bool shouldBeReadOnly) const
{
    // Hmm.. should we give global write permission or just the current user?
    return setFileModeFlags (fullPath, S_IWUSR | S_IWGRP | S_IWOTH, ! shouldBeReadOnly);
}

bool File::setFileExecutableInternal (bool shouldBeExecutable) const
{
    return setFileModeFlags (fullPath, S_IXUSR | S_IXGRP | S_IXOTH, shouldBeExecutable);
}

void File::getFileTimesInternal (int64& modificationTime, int64& accessTime, int64& creationTime) const
{
    modificationTime = 0;
    accessTime = 0;
    creationTime = 0;

    juce_statStruct info;

    if (juce_stat (fullPath, info))
    {
        modificationTime  = (int64) info.st_mtime * 1000;
        accessTime        = (int64) info.st_atime * 1000;
       #if (JUCE_MAC && MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_5) || JUCE_IOS
        creationTime      = (int64) info.st_birthtime * 1000;
       #else
        creationTime      = (int64) info.st_ctime * 1000;
       #endif
    }
}

bool File::setFileTimesInternal (int64 modificationTime, int64 accessTime, int64 /*creationTime*/) const
{
    juce_statStruct info;

    if ((modificationTime != 0 || accessTime != 0) && juce_stat (fullPath, info))
    {
        struct utimbuf times;
        times.actime  = accessTime != 0       ? static_cast<time_t> (accessTime / 1000)       : static_cast<time_t> (info.st_atime);
        times.modtime = modificationTime != 0 ? static_cast<time_t> (modificationTime / 1000) : static_cast<time_t> (info.st_mtime);

        return utime (fullPath.toUTF8(), &times) == 0;
    }

    return false;
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

//==============================================================================
int64 juce_fileSetPosition (void* handle, int64 pos)
{
    if (handle != 0 && lseek (getFD (handle), (off_t) pos, SEEK_SET) == pos)
        return pos;

    return -1;
}

void FileInputStream::openHandle()
{
    const int f = open (file.getFullPathName().toUTF8(), O_RDONLY, 00644);

    if (f != -1)
        fileHandle = fdToVoidPointer (f);
    else
        status = getResultForErrno();
}

FileInputStream::~FileInputStream()
{
    if (fileHandle != 0)
        close (getFD (fileHandle));
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

//==============================================================================
void FileOutputStream::openHandle()
{
    if (file.exists())
    {
        const int f = open (file.getFullPathName().toUTF8(), O_RDWR, 00644);

        if (f != -1)
        {
            currentPosition = lseek (f, 0, SEEK_END);

            if (currentPosition >= 0)
            {
                fileHandle = fdToVoidPointer (f);
            }
            else
            {
                status = getResultForErrno();
                close (f);
            }
        }
        else
        {
            status = getResultForErrno();
        }
    }
    else
    {
        const int f = open (file.getFullPathName().toUTF8(), O_RDWR + O_CREAT, 00644);

        if (f != -1)
            fileHandle = fdToVoidPointer (f);
        else
            status = getResultForErrno();
    }
}

void FileOutputStream::closeHandle()
{
    if (fileHandle != 0)
    {
        close (getFD (fileHandle));
        fileHandle = 0;
    }
}

ssize_t FileOutputStream::writeInternal (const void* const data, const size_t numBytes)
{
    ssize_t result = 0;

    if (fileHandle != 0)
    {
        result = ::write (getFD (fileHandle), data, numBytes);

        if (result == -1)
            status = getResultForErrno();
    }

    return result;
}

#ifndef JUCE_ANDROID
void FileOutputStream::flushInternal()
{
    if (fileHandle != 0 && fsync (getFD (fileHandle)) == -1)
        status = getResultForErrno();
}
#endif

Result FileOutputStream::truncate()
{
    if (fileHandle == 0)
        return status;

    flush();
    return getResultForReturnValue (ftruncate (getFD (fileHandle), (off_t) currentPosition));
}

//==============================================================================
String SystemStats::getEnvironmentVariable (const String& name, const String& defaultValue)
{
    if (const char* s = ::getenv (name.toUTF8()))
        return String::fromUTF8 (s);

    return defaultValue;
}

//==============================================================================
void MemoryMappedFile::openInternal (const File& file, AccessMode mode, bool exclusive)
{
    jassert (mode == readOnly || mode == readWrite);

    if (range.getStart() > 0)
    {
        const long pageSize = sysconf (_SC_PAGE_SIZE);
        range.setStart (range.getStart() - (range.getStart() % pageSize));
    }

    fileHandle = open (file.getFullPathName().toUTF8(),
                       mode == readWrite ? (O_CREAT + O_RDWR) : O_RDONLY, 00644);

    if (fileHandle != -1)
    {
        void* m = mmap (0, (size_t) range.getLength(),
                        mode == readWrite ? (PROT_READ | PROT_WRITE) : PROT_READ,
                        exclusive ? MAP_PRIVATE : MAP_SHARED, fileHandle,
                        (off_t) range.getStart());

        if (m != MAP_FAILED)
        {
            address = m;
            madvise (m, (size_t) range.getLength(), MADV_SEQUENTIAL);
        }
        else
        {
            range = Range<int64>();
        }
    }
}

MemoryMappedFile::~MemoryMappedFile()
{
    if (address != nullptr)
        munmap (address, (size_t) range.getLength());

    if (fileHandle != 0)
        close (fileHandle);
}

//==============================================================================
File juce_getExecutableFile();
File juce_getExecutableFile()
{
   #if JUCE_ANDROID
    return File (android.appFile);
   #else
    struct DLAddrReader
    {
        static String getFilename()
        {
            Dl_info exeInfo;

            void* localSymbol = (void*) juce_getExecutableFile;
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

    static String filename = DLAddrReader::getFilename();
    return filename;
   #endif
}

//==============================================================================
int64 File::getBytesFreeOnVolume() const
{
    struct statfs buf;
    if (juce_doStatFS (*this, buf))
        return (int64) buf.f_bsize * (int64) buf.f_bavail; // Note: this returns space available to non-super user

    return 0;
}

int64 File::getVolumeTotalSize() const
{
    struct statfs buf;
    if (juce_doStatFS (*this, buf))
        return (int64) buf.f_bsize * (int64) buf.f_blocks;

    return 0;
}

String File::getVolumeLabel() const
{
   #if JUCE_MAC
    struct VolAttrBuf
    {
        u_int32_t       length;
        attrreference_t mountPointRef;
        char            mountPointSpace [MAXPATHLEN];
    } attrBuf;

    struct attrlist attrList;
    zerostruct (attrList); // (can't use "= { 0 }" on this object because it's typedef'ed as a C struct)
    attrList.bitmapcount = ATTR_BIT_MAP_COUNT;
    attrList.volattr = ATTR_VOL_INFO | ATTR_VOL_NAME;

    File f (*this);

    for (;;)
    {
        if (getattrlist (f.getFullPathName().toUTF8(), &attrList, &attrBuf, sizeof (attrBuf), 0) == 0)
            return String::fromUTF8 (((const char*) &attrBuf.mountPointRef) + attrBuf.mountPointRef.attr_dataoffset,
                                     (int) attrBuf.mountPointRef.attr_length);

        const File parent (f.getParentDirectory());

        if (f == parent)
            break;

        f = parent;
    }
   #endif

    return {};
}

int File::getVolumeSerialNumber() const
{
    int result = 0;
/*    int fd = open (getFullPathName().toUTF8(), O_RDONLY | O_NONBLOCK);

    char info [512];

    #ifndef HDIO_GET_IDENTITY
     #define HDIO_GET_IDENTITY 0x030d
    #endif

    if (ioctl (fd, HDIO_GET_IDENTITY, info) == 0)
    {
        DBG (String (info + 20, 20));
        result = String (info + 20, 20).trim().getIntValue();
    }

    close (fd);*/
    return result;
}

//==============================================================================
#if ! JUCE_IOS
void juce_runSystemCommand (const String&);
void juce_runSystemCommand (const String& command)
{
    int result = system (command.toUTF8());
    ignoreUnused (result);
}

String juce_getOutputFromCommand (const String&);
String juce_getOutputFromCommand (const String& command)
{
    // slight bodge here, as we just pipe the output into a temp file and read it...
    const File tempFile (File::getSpecialLocation (File::tempDirectory)
                           .getNonexistentChildFile (String::toHexString (Random::getSystemRandom().nextInt()), ".tmp", false));

    juce_runSystemCommand (command + " > " + tempFile.getFullPathName());

    String result (tempFile.loadFileAsString());
    tempFile.deleteFile();
    return result;
}
#endif

//==============================================================================
#if JUCE_IOS
class InterProcessLock::Pimpl
{
public:
    Pimpl (const String&, int)
        : handle (1), refCount (1) // On iOS just fake success..
    {
    }

    int handle, refCount;
};

#else

class InterProcessLock::Pimpl
{
public:
    Pimpl (const String& lockName, const int timeOutMillisecs)
        : handle (0), refCount (1)
    {
       #if JUCE_MAC
        if (! createLockFile (File ("~/Library/Caches/com.juce.locks").getChildFile (lockName), timeOutMillisecs))
            // Fallback if the user's home folder is on a network drive with no ability to lock..
            createLockFile (File ("/tmp/com.juce.locks").getChildFile (lockName), timeOutMillisecs);

       #else
        File tempFolder ("/var/tmp");
        if (! tempFolder.isDirectory())
            tempFolder = "/tmp";

        createLockFile (tempFolder.getChildFile (lockName), timeOutMillisecs);
       #endif
    }

    ~Pimpl()
    {
        closeFile();
    }

    bool createLockFile (const File& file, const int timeOutMillisecs)
    {
        file.create();
        handle = open (file.getFullPathName().toUTF8(), O_RDWR);

        if (handle != 0)
        {
            struct flock fl;
            zerostruct (fl);

            fl.l_whence = SEEK_SET;
            fl.l_type = F_WRLCK;

            const int64 endTime = Time::currentTimeMillis() + timeOutMillisecs;

            for (;;)
            {
                const int result = fcntl (handle, F_SETLK, &fl);

                if (result >= 0)
                    return true;

                const int error = errno;

                if (error != EINTR)
                {
                    if (error == EBADF || error == ENOTSUP)
                        return false;

                    if (timeOutMillisecs == 0
                         || (timeOutMillisecs > 0 && Time::currentTimeMillis() >= endTime))
                        break;

                    Thread::sleep (10);
                }
            }
        }

        closeFile();
        return true; // only false if there's a file system error. Failure to lock still returns true.
    }

    void closeFile()
    {
        if (handle != 0)
        {
            struct flock fl;
            zerostruct (fl);

            fl.l_whence = SEEK_SET;
            fl.l_type = F_UNLCK;

            while (! (fcntl (handle, F_SETLKW, &fl) >= 0 || errno != EINTR))
            {}

            close (handle);
            handle = 0;
        }
    }

    int handle, refCount;
};
#endif

InterProcessLock::InterProcessLock (const String& nm)  : name (nm)
{
}

InterProcessLock::~InterProcessLock()
{
}

bool InterProcessLock::enter (const int timeOutMillisecs)
{
    const ScopedLock sl (lock);

    if (pimpl == nullptr)
    {
        pimpl = new Pimpl (name, timeOutMillisecs);

        if (pimpl->handle == 0)
            pimpl = nullptr;
    }
    else
    {
        pimpl->refCount++;
    }

    return pimpl != nullptr;
}

void InterProcessLock::exit()
{
    const ScopedLock sl (lock);

    // Trying to release the lock too many times!
    jassert (pimpl != nullptr);

    if (pimpl != nullptr && --(pimpl->refCount) == 0)
        pimpl = nullptr;
}

//==============================================================================
void JUCE_API juce_threadEntryPoint (void*);

#if JUCE_ANDROID
extern JavaVM* androidJNIJavaVM;
#endif

extern "C" void* threadEntryProc (void*);
extern "C" void* threadEntryProc (void* userData)
{
   #if JUCE_ANDROID
    // JNI_OnLoad was not called - make sure you load the JUCE shared library
    // using System.load inside of Java
    jassert (androidJNIJavaVM != nullptr);

    JNIEnv* env;
    androidJNIJavaVM->AttachCurrentThread (&env, nullptr);
    setEnv (env);
   #endif

    JUCE_AUTORELEASEPOOL
    {
        juce_threadEntryPoint (userData);
    }

    return nullptr;
}

#if JUCE_ANDROID && JUCE_MODULE_AVAILABLE_juce_audio_devices && (JUCE_USE_ANDROID_OPENSLES || (! defined(JUCE_USE_ANDROID_OPENSLES) && JUCE_ANDROID_API_VERSION > 8))
#define JUCE_ANDROID_REALTIME_THREAD_AVAILABLE 1
#endif

#if JUCE_ANDROID_REALTIME_THREAD_AVAILABLE
extern pthread_t juce_createRealtimeAudioThread (void* (*entry) (void*), void* userPtr);
#endif

void Thread::launchThread()
{
   #if JUCE_ANDROID
    if (isAndroidRealtimeThread)
    {
       #if JUCE_ANDROID_REALTIME_THREAD_AVAILABLE
        threadHandle = (void*) juce_createRealtimeAudioThread (threadEntryProc, this);
        threadId = (ThreadID) threadHandle;

        return;
       #else
        jassertfalse;
       #endif
    }
   #endif

    threadHandle = 0;
    pthread_t handle = 0;
    pthread_attr_t attr;
    pthread_attr_t* attrPtr = nullptr;

    if (pthread_attr_init (&attr) == 0)
    {
        attrPtr = &attr;

        pthread_attr_setstacksize (attrPtr, threadStackSize);
    }

    if (pthread_create (&handle, attrPtr, threadEntryProc, this) == 0)
    {
        pthread_detach (handle);
        threadHandle = (void*) handle;
        threadId = (ThreadID) threadHandle;
    }

    if (attrPtr != nullptr)
        pthread_attr_destroy (attrPtr);
}

void Thread::closeThreadHandle()
{
    threadId = 0;
    threadHandle = 0;
}

void Thread::killThread()
{
    if (threadHandle != 0)
    {
       #if JUCE_ANDROID
        jassertfalse; // pthread_cancel not available!
       #else
        pthread_cancel ((pthread_t) threadHandle);
       #endif
    }
}

void JUCE_CALLTYPE Thread::setCurrentThreadName (const String& name)
{
   #if JUCE_IOS || JUCE_MAC
    JUCE_AUTORELEASEPOOL
    {
        [[NSThread currentThread] setName: juceStringToNS (name)];
    }
   #elif JUCE_LINUX || JUCE_ANDROID
    #if ((JUCE_LINUX && (__GLIBC__ * 1000 + __GLIBC_MINOR__) >= 2012) \
          || JUCE_ANDROID && __ANDROID_API__ >= 9)
     pthread_setname_np (pthread_self(), name.toRawUTF8());
    #else
     prctl (PR_SET_NAME, name.toRawUTF8(), 0, 0, 0);
    #endif
   #endif
}

bool Thread::setThreadPriority (void* handle, int priority)
{
    struct sched_param param;
    int policy;
    priority = jlimit (0, 10, priority);

    if (handle == nullptr)
        handle = (void*) pthread_self();

    if (pthread_getschedparam ((pthread_t) handle, &policy, &param) != 0)
        return false;

    policy = priority == 0 ? SCHED_OTHER : SCHED_RR;

    const int minPriority = sched_get_priority_min (policy);
    const int maxPriority = sched_get_priority_max (policy);

    param.sched_priority = ((maxPriority - minPriority) * priority) / 10 + minPriority;
    return pthread_setschedparam ((pthread_t) handle, policy, &param) == 0;
}

Thread::ThreadID JUCE_CALLTYPE Thread::getCurrentThreadId()
{
    return (ThreadID) pthread_self();
}

void JUCE_CALLTYPE Thread::yield()
{
    sched_yield();
}

//==============================================================================
/* Remove this macro if you're having problems compiling the cpu affinity
   calls (the API for these has changed about quite a bit in various Linux
   versions, and a lot of distros seem to ship with obsolete versions)
*/
#if defined (CPU_ISSET) && ! defined (SUPPORT_AFFINITIES)
 #define SUPPORT_AFFINITIES 1
#endif

void JUCE_CALLTYPE Thread::setCurrentThreadAffinityMask (const uint32 affinityMask)
{
   #if SUPPORT_AFFINITIES
    cpu_set_t affinity;
    CPU_ZERO (&affinity);

    for (int i = 0; i < 32; ++i)
        if ((affinityMask & (1 << i)) != 0)
            CPU_SET (i, &affinity);

   #if (! JUCE_ANDROID) && ((! JUCE_LINUX) || ((__GLIBC__ * 1000 + __GLIBC_MINOR__) >= 2004))
    pthread_setaffinity_np (pthread_self(), sizeof (cpu_set_t), &affinity);
   #elif JUCE_ANDROID
    sched_setaffinity (gettid(), sizeof (cpu_set_t), &affinity);
   #else
    // NB: this call isn't really correct because it sets the affinity of the process,
    // (getpid) not the thread (not gettid). But it's included here as a fallback for
    // people who are using ridiculously old versions of glibc
    sched_setaffinity (getpid(), sizeof (cpu_set_t), &affinity);
   #endif

    sched_yield();

   #else
    // affinities aren't supported because either the appropriate header files weren't found,
    // or the SUPPORT_AFFINITIES macro was turned off
    jassertfalse;
    ignoreUnused (affinityMask);
   #endif
}

//==============================================================================
bool DynamicLibrary::open (const String& name)
{
    close();
    handle = dlopen (name.isEmpty() ? nullptr : name.toUTF8().getAddress(), RTLD_LOCAL | RTLD_NOW);
    return handle != nullptr;
}

void DynamicLibrary::close()
{
    if (handle != nullptr)
    {
        dlclose (handle);
        handle = nullptr;
    }
}

void* DynamicLibrary::getFunction (const String& functionName) noexcept
{
    return handle != nullptr ? dlsym (handle, functionName.toUTF8()) : nullptr;
}


//==============================================================================
static inline String readPosixConfigFileValue (const char* file, const char* const key)
{
    StringArray lines;
    File (file).readLines (lines);

    for (int i = lines.size(); --i >= 0;) // (NB - it's important that this runs in reverse order)
        if (lines[i].upToFirstOccurrenceOf (":", false, false).trim().equalsIgnoreCase (key))
            return lines[i].fromFirstOccurrenceOf (":", false, false).trim();

    return {};
}


//==============================================================================
class ChildProcess::ActiveProcess
{
public:
    ActiveProcess (const StringArray& arguments, int streamFlags)
        : childPID (0), pipeHandle (0), readHandle (0)
    {
        String exe (arguments[0].unquoted());

        // Looks like you're trying to launch a non-existent exe or a folder (perhaps on OSX
        // you're trying to launch the .app folder rather than the actual binary inside it?)
        jassert (File::getCurrentWorkingDirectory().getChildFile (exe).existsAsFile()
                  || ! exe.containsChar (File::separator));

        int pipeHandles[2] = { 0 };

        if (pipe (pipeHandles) == 0)
        {
              Array<char*> argv;
              for (int i = 0; i < arguments.size(); ++i)
                  if (arguments[i].isNotEmpty())
                      argv.add (const_cast<char*> (arguments[i].toRawUTF8()));

              argv.add (nullptr);

#if JUCE_USE_VFORK
            const pid_t result = vfork();
#else
            const pid_t result = fork();
#endif

            if (result < 0)
            {
                close (pipeHandles[0]);
                close (pipeHandles[1]);
            }
            else if (result == 0)
            {
#if ! JUCE_USE_VFORK
                // we're the child process..
                close (pipeHandles[0]);   // close the read handle

                if ((streamFlags & wantStdOut) != 0)
                    dup2 (pipeHandles[1], STDOUT_FILENO); // turns the pipe into stdout
                else
                    dup2 (open ("/dev/null", O_WRONLY), STDOUT_FILENO);

                if ((streamFlags & wantStdErr) != 0)
                    dup2 (pipeHandles[1], STDERR_FILENO);
                else
                    dup2 (open ("/dev/null", O_WRONLY), STDERR_FILENO);

                close (pipeHandles[1]);
#endif

                if (execvp (argv[0], argv.getRawDataPointer()) < 0)
                    _exit (-1);
            }
            else
            {
                // we're the parent process..
                childPID = result;
                pipeHandle = pipeHandles[0];
                close (pipeHandles[1]); // close the write handle
            }
        }

        ignoreUnused (streamFlags);
    }

    ~ActiveProcess()
    {
        if (readHandle != 0)
            fclose (readHandle);

        if (pipeHandle != 0)
            close (pipeHandle);
    }

    bool isRunning() const noexcept
    {
        if (childPID != 0)
        {
            int childState;
            const int pid = waitpid (childPID, &childState, WNOHANG);
            return pid == 0 || ! (WIFEXITED (childState) || WIFSIGNALED (childState));
        }

        return false;
    }

    int read (void* const dest, const int numBytes) noexcept
    {
        jassert (dest != nullptr);

        #ifdef fdopen
         #error // the zlib headers define this function as NULL!
        #endif

        if (readHandle == 0 && childPID != 0)
            readHandle = fdopen (pipeHandle, "r");

        if (readHandle != 0)
            return (int) fread (dest, 1, (size_t) numBytes, readHandle);

        return 0;
    }

    bool killProcess() const noexcept
    {
        return ::kill (childPID, SIGKILL) == 0;
    }

    uint32 getExitCode() const noexcept
    {
        if (childPID != 0)
        {
            int childState = 0;
            const int pid = waitpid (childPID, &childState, WNOHANG);

            if (pid >= 0 && WIFEXITED (childState))
                return WEXITSTATUS (childState);
        }

        return 0;
    }

    int getPID() const noexcept
    {
        return childPID;
    }

    int childPID;

private:
    int pipeHandle;
    FILE* readHandle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ActiveProcess)
};

bool ChildProcess::start (const String& command, int streamFlags)
{
    return start (StringArray::fromTokens (command, true), streamFlags);
}

bool ChildProcess::start (const StringArray& args, int streamFlags)
{
    if (args.size() == 0)
        return false;

    activeProcess = new ActiveProcess (args, streamFlags);

    if (activeProcess->childPID == 0)
        activeProcess = nullptr;

    return activeProcess != nullptr;
}

//==============================================================================
struct HighResolutionTimer::Pimpl
{
    Pimpl (HighResolutionTimer& t)  : owner (t), thread (0), destroyThread (false), isRunning (false)
    {
        pthread_condattr_t attr;
        pthread_condattr_init (&attr);

       #if JUCE_LINUX || (JUCE_ANDROID && defined(__ANDROID_API__) && __ANDROID_API__ >= 21)
        pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
       #endif

        pthread_cond_init (&stopCond, &attr);
        pthread_condattr_destroy (&attr);
        pthread_mutex_init (&timerMutex, nullptr);
    }

    ~Pimpl()
    {
        jassert (! isRunning);
        stop();
    }

    void start (int newPeriod)
    {
        if (periodMs != newPeriod)
        {
            if (thread != pthread_self())
            {
                stop();

                periodMs = newPeriod;
                destroyThread = false;
                isRunning = true;

                if (pthread_create (&thread, nullptr, timerThread, this) == 0)
                    setThreadToRealtime (thread, (uint64) newPeriod);
                else
                    jassertfalse;
            }
            else
            {
                periodMs = newPeriod;
                isRunning = true;
                destroyThread = false;
            }
        }
    }

    void stop()
    {
        isRunning = false;

        if (thread == 0)
            return;

        if (thread == pthread_self())
        {
            periodMs = 3600000;
            return;
        }

        isRunning = false;
        destroyThread = true;

        pthread_mutex_lock (&timerMutex);
        pthread_cond_signal (&stopCond);
        pthread_mutex_unlock (&timerMutex);

        pthread_join (thread, nullptr);
        thread = 0;
    }

    HighResolutionTimer& owner;
    int volatile periodMs;

private:
    pthread_t thread;
    pthread_cond_t stopCond;
    pthread_mutex_t timerMutex;

    bool volatile destroyThread;
    bool volatile isRunning;

    static void* timerThread (void* param)
    {
       #if JUCE_ANDROID
        // JNI_OnLoad was not called - make sure you load the JUCE shared library
        // using System.load inside of Java
        jassert (androidJNIJavaVM != nullptr);

        JNIEnv* env;
        androidJNIJavaVM->AttachCurrentThread (&env, nullptr);
        setEnv (env);
       #else
        int dummy;
        pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, &dummy);
       #endif

        reinterpret_cast<Pimpl*> (param)->timerThread();
        return nullptr;
    }

    void timerThread()
    {
        int lastPeriod = periodMs;
        Clock clock (lastPeriod);

        pthread_mutex_lock (&timerMutex);

        while (! destroyThread)
        {
            clock.next();
            while (! destroyThread && clock.wait (stopCond, timerMutex));

            if (destroyThread)
                break;

            if (isRunning)
                owner.hiResTimerCallback();

            if (lastPeriod != periodMs)
            {
                lastPeriod = periodMs;
                clock = Clock (lastPeriod);
            }
        }

        periodMs = 0;

        pthread_mutex_unlock (&timerMutex);
        pthread_exit (nullptr);
    }

    struct Clock
    {
       #if JUCE_MAC || JUCE_IOS
        Clock (double millis) noexcept
        {
            (void) mach_timebase_info (&timebase);
            delta = (((uint64_t) (millis * 1000000.0)) * timebase.denom) / timebase.numer;
            time = mach_absolute_time();
        }

        bool wait (pthread_cond_t& cond, pthread_mutex_t& mutex) noexcept
        {
            struct timespec left;

            if (! hasExpired (left))
                return (pthread_cond_timedwait_relative_np (&cond, &mutex, &left) != ETIMEDOUT);

            return false;
        }

        uint64_t time, delta;
        mach_timebase_info_data_t timebase;

        bool hasExpired(struct timespec& time_left) noexcept
        {
            uint64_t now = mach_absolute_time();

            if (now < time)
            {
                uint64_t left = time - now;
                uint64_t nanos = (left * static_cast<uint64_t> (timebase.numer)) / static_cast<uint64_t> (timebase.denom);
                time_left.tv_sec = static_cast<__darwin_time_t> (nanos / 1000000000ULL);
                time_left.tv_nsec = static_cast<long> (nanos - (static_cast<uint64_t> (time_left.tv_sec) * 1000000000ULL));

                return false;
            }

            return true;
        }
      #else
        Clock (double millis) noexcept  : delta ((uint64) (millis * 1000000))
        {
            struct timespec t;
            clock_gettime (CLOCK_MONOTONIC, &t);
            time = (uint64) (1000000000 * (int64) t.tv_sec + (int64) t.tv_nsec);
        }

        bool wait (pthread_cond_t& cond, pthread_mutex_t& mutex) noexcept
        {
            struct timespec absExpire;

            if (! hasExpired (absExpire))
                return (pthread_cond_timedwait (&cond, &mutex, &absExpire) != ETIMEDOUT);

            return false;
        }

        uint64 time, delta;

        bool hasExpired(struct timespec& expiryTime) noexcept
        {
            struct timespec t;
            clock_gettime (CLOCK_MONOTONIC, &t);
            uint64 now = (uint64) (1000000000 * (int64) t.tv_sec + (int64) t.tv_nsec);

            if (now < time)
            {
                expiryTime.tv_sec  = (time_t) (time / 1000000000);
                expiryTime.tv_nsec = (long)   (time % 1000000000);

                return false;
            }

            return true;
        }
       #endif

        void next() noexcept
        {
            time += delta;
        }
    };

    static bool setThreadToRealtime (pthread_t thread, uint64 periodMs)
    {
       #if JUCE_MAC || JUCE_IOS
        thread_time_constraint_policy_data_t policy;
        policy.period      = (uint32_t) (periodMs * 1000000);
        policy.computation = 50000;
        policy.constraint  = policy.period;
        policy.preemptible = true;

        return thread_policy_set (pthread_mach_thread_np (thread),
                                  THREAD_TIME_CONSTRAINT_POLICY,
                                  (thread_policy_t) &policy,
                                  THREAD_TIME_CONSTRAINT_POLICY_COUNT) == KERN_SUCCESS;

       #else
        ignoreUnused (periodMs);
        struct sched_param param;
        param.sched_priority = sched_get_priority_max (SCHED_RR);
        return pthread_setschedparam (thread, SCHED_RR, &param) == 0;

       #endif
    }

    JUCE_DECLARE_NON_COPYABLE (Pimpl)
};

} // namespace juce
