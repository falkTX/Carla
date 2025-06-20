/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2025 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_IS_STANDALONE
# error Wrong build configuration
#endif

#include "../extra/String.hpp"
#include "../DistrhoPluginUtils.hpp"
#include "../DistrhoStandaloneUtils.hpp"

#ifdef DISTRHO_OS_WINDOWS
# include <direct.h>
# include <shlobj.h>
# include <windows.h>
#else
# ifndef STATIC_BUILD
#  include <dlfcn.h>
# endif
# include <fcntl.h>
# include <limits.h>
# include <pwd.h>
# include <stdlib.h>
# include <sys/stat.h>
# include <unistd.h>
#endif

#ifdef DISTRHO_OS_WINDOWS
# if DISTRHO_IS_STANDALONE || defined(STATIC_BUILD)
static constexpr const HINSTANCE hInstance = nullptr;
# else
static HINSTANCE hInstance = nullptr;

DISTRHO_PLUGIN_EXPORT
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
        hInstance = hInst;
    return 1;
}
# endif
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

const char* getBinaryFilename()
{
    static String filename;

  #ifndef STATIC_BUILD
    if (filename.isNotEmpty())
        return filename;

   #ifdef DISTRHO_OS_WINDOWS
    CHAR filenameBuf[MAX_PATH];
    filenameBuf[0] = '\0';
    GetModuleFileNameA(hInstance, filenameBuf, sizeof(filenameBuf));
    filename = filenameBuf;
   #else
    Dl_info info;
    dladdr((void*)getBinaryFilename, &info);
    char filenameBuf[PATH_MAX];
    filename = realpath(info.dli_fname, filenameBuf);
   #endif
  #endif

    return filename;
}

const char* getConfigDir()
{
   #if defined(DISTRHO_OS_MAC) || defined(DISTRHO_OS_WASM) || defined(DISTRHO_OS_WINDOWS)
    return getDocumentsDir();
   #else
    static String dir;

    if (dir.isEmpty())
    {
        if (const char* const xdgEnv = getenv("XDG_CONFIG_HOME"))
            dir = xdgEnv;

        if (dir.isEmpty())
        {
            dir  = getHomeDir();
            dir += "/.config";
        }

        // ensure main config dir exists
        if (access(dir, F_OK) != 0)
            mkdir(dir, 0755);

        // and also our custom subdir
        dir += "/" DISTRHO_PLUGIN_NAME "/";
        if (access(dir, F_OK) != 0)
            mkdir(dir, 0755);
    }

    return dir;
   #endif
}

const char* getDocumentsDir()
{
    static String dir;

    if (dir.isEmpty())
    {
       #if defined(DISTRHO_OS_MAC)
        dir  = getHomeDir();
        dir += "/Documents/" DISTRHO_PLUGIN_NAME "/";
       #elif defined(DISTRHO_OS_WASM)
        dir  = getHomeDir();
        dir += "/";
       #elif defined(DISTRHO_OS_WINDOWS)
        WCHAR wpath[MAX_PATH];
        if (SHGetFolderPathW(nullptr, CSIDL_MYDOCUMENTS, nullptr, SHGFP_TYPE_CURRENT, wpath) == S_OK)
        {
            CHAR apath[MAX_PATH];
            if (WideCharToMultiByte(CP_UTF8, 0, wpath, -1, apath, MAX_PATH, nullptr, nullptr) != 0)
            {
                dir = apath;
                dir += "\\" DISTRHO_PLUGIN_NAME "\\";
                wcscat(wpath, L"\\" DISTRHO_PLUGIN_NAME "\\");
            }
        }
       #else
        String xdgDirsConfigPath(getConfigDir());
        xdgDirsConfigPath += "/user-dirs.dirs";

        if (FILE* const f = std::fopen(xdgDirsConfigPath, "r"))
        {
            std::fseek(f, 0, SEEK_END);
            const long size = std::ftell(f);
            std::fseek(f, 0, SEEK_SET);

            // something is wrong if config dirs file is longer than 1MiB!
            if (size > 0 && size < 1024 * 1024)
            {
                if (char* filedata = static_cast<char*>(std::malloc(size)))
                {
                    for (long r = 0, total = 0; total < size;)
                    {
                        r = std::fread(filedata + total, 1, size - total, f);

                        if (r == 0)
                        {
                            std::free(filedata);
                            filedata = nullptr;
                            break;
                        }

                        total += r;
                    }

                    if (filedata != nullptr)
                    {
                        if (char* const xdgDocsDir = std::strstr(filedata, "XDG_DOCUMENTS_DIR=\""))
                        {
                            if (char* const xdgDocsDirNL = std::strstr(xdgDocsDir, "\"\n"))
                            {
                                *xdgDocsDirNL = '\0';
                                String sdir(xdgDocsDir + 19);

                                if (sdir.startsWith("$HOME"))
                                {
                                    dir  = getHomeDir();
                                    dir += sdir.buffer() + 5;
                                }
                                else
                                {
                                    dir = sdir;
                                }

                                // ensure main config dir exists
                                if (access(dir, F_OK) != 0)
                                    mkdir(dir, 0755);
                            }
                        }

                        std::free(filedata);
                    }
                }
            }

            std::fclose(f);
        }

        // ${XDG_CONFIG_HOME}/user-dirs.dirs does not exist or has bad data
        if (dir.isEmpty())
        {
            dir  = getDocumentsDir();
            dir += DISTRHO_PLUGIN_NAME "/";
        }
       #endif

        // ensure our custom subdir exists
        if (dir.isNotEmpty())
        {
           #ifdef DISTRHO_OS_WINDOWS
            _wmkdir(wpath);
           #else
            if (access(dir, F_OK) != 0)
                mkdir(dir, 0755);
           #endif
        }
    }

    return dir;
}

const char* getHomeDir()
{
    static String dir;

    if (dir.isEmpty())
    {
       #ifdef DISTRHO_OS_WINDOWS
        WCHAR wpath[MAX_PATH];
        if (SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, SHGFP_TYPE_CURRENT, wpath) == S_OK)
        {
            CHAR apath[MAX_PATH];
            if (WideCharToMultiByte(CP_UTF8, 0, wpath, -1, apath, MAX_PATH, nullptr, nullptr) != 0)
                dir = apath;
        }
       #else
        if (const char* const homeEnv = getenv("HOME"))
            dir = homeEnv;

        if (dir.isEmpty())
            if (struct passwd* const pwd = getpwuid(getuid()))
                dir = pwd->pw_dir;

        if (dir.isNotEmpty() && ! dir.endsWith('/'))
            dir += "/";
       #endif
    }

    return dir;
}

const char* getPluginFormatName() noexcept
{
#if defined(DISTRHO_PLUGIN_TARGET_AU)
    return "AudioUnit";
#elif defined(DISTRHO_PLUGIN_TARGET_CARLA)
    return "Carla";
#elif defined(DISTRHO_PLUGIN_TARGET_JACK)
   #if defined(DISTRHO_OS_WASM)
    return "Wasm/Standalone";
   #elif defined(HAVE_JACK)
    return "JACK/Standalone";
   #else
    return "Standalone";
   #endif
#elif defined(DISTRHO_PLUGIN_TARGET_LADSPA)
    return "LADSPA";
#elif defined(DISTRHO_PLUGIN_TARGET_DSSI)
    return "DSSI";
#elif defined(DISTRHO_PLUGIN_TARGET_LV2)
    return "LV2";
#elif defined(DISTRHO_PLUGIN_TARGET_VST2)
    return "VST2";
#elif defined(DISTRHO_PLUGIN_TARGET_VST3)
    return "VST3";
#elif defined(DISTRHO_PLUGIN_TARGET_CLAP)
    return "CLAP";
#elif defined(DISTRHO_PLUGIN_TARGET_STATIC) && defined(DISTRHO_PLUGIN_TARGET_STATIC_NAME)
    return DISTRHO_PLUGIN_TARGET_STATIC_NAME;
#else
    return "Unknown";
#endif
}

const char* getResourcePath(const char* const bundlePath) noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(bundlePath != nullptr, nullptr);

   #if defined(DISTRHO_PLUGIN_TARGET_AU) || defined(DISTRHO_PLUGIN_TARGET_JACK) || defined(DISTRHO_PLUGIN_TARGET_VST2) || defined(DISTRHO_PLUGIN_TARGET_CLAP)
    static String resourcePath;

    if (resourcePath.isEmpty())
    {
        resourcePath = bundlePath;
       #ifdef DISTRHO_OS_MAC
        resourcePath += "/Contents/Resources";
       #else
        resourcePath += DISTRHO_OS_SEP_STR "resources";
       #endif
    }

    return resourcePath.buffer();
   #elif defined(DISTRHO_PLUGIN_TARGET_LV2)
    static String resourcePath;

    if (resourcePath.isEmpty())
    {
        resourcePath = bundlePath;
        resourcePath += DISTRHO_OS_SEP_STR "resources";
    }

    return resourcePath.buffer();
   #elif defined(DISTRHO_PLUGIN_TARGET_VST3)
    static String resourcePath;

    if (resourcePath.isEmpty())
    {
        resourcePath = bundlePath;
        resourcePath += "/Contents/Resources";
    }

    return resourcePath.buffer();
   #endif

    return nullptr;
}

#ifndef DISTRHO_PLUGIN_TARGET_JACK
// all these are null for non-standalone targets
bool isUsingNativeAudio() noexcept { return false; }
bool supportsAudioInput() { return false; }
bool supportsBufferSizeChanges() { return false; }
bool supportsMIDI() { return false; }
bool isAudioInputEnabled() { return false; }
bool isMIDIEnabled() { return false; }
uint getBufferSize() { return 0; }
bool requestAudioInput() { return false; }
bool requestBufferSizeChange(uint) { return false; }
bool requestMIDI() { return false; }
#endif

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
