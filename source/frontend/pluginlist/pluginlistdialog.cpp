// SPDX-FileCopyrightText: 2011-2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "pluginlistdialog.hpp"
#include "pluginrefreshdialog.hpp"

#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated-copy-with-user-provided-copy"
# pragma clang diagnostic ignored "-Wdeprecated-register"
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wclass-memaccess"
# pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QList>
#include <QtCore/QTimer>

#ifdef __clang__
# pragma clang diagnostic pop
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic pop
#endif

#include "qcarlastring.hpp"
#include "qsafesettings.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaJuceUtils.hpp"
#include "CarlaUtils.h"

#include "extra/ScopedPointer.hpp"

#include <cstdlib>

CARLA_BACKEND_USE_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
// getenv with a fallback value if unset

static inline
const char* getEnvWithFallback(const char* const env, const char* const fallback)
{
    if (const char* const value = std::getenv(env))
        return value;

    return fallback;
}

// --------------------------------------------------------------------------------------------------------------------
// Plugin paths (from env vars first, then default locations)

struct PluginPaths {
   #ifndef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
    QCarlaString ladspa;
    QCarlaString dssi;
   #endif
    QCarlaString lv2;
    QCarlaString vst2;
    QCarlaString vst3;
    QCarlaString clap;
   #ifdef CARLA_OS_MAC
    QCarlaString au;
   #endif
   #ifndef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
    QCarlaString jsfx;
    QCarlaString sf2;
    QCarlaString sfz;
   #endif

    PluginPaths()
    {
        // get common env vars
        const QString HOME = QDir::toNativeSeparators(QDir::homePath());

       #if defined(CARLA_OS_WIN)
        const char *const envAPPDATA = std::getenv("APPDATA");
        const char *const envLOCALAPPDATA = getEnvWithFallback("LOCALAPPDATA", envAPPDATA);
        const char *const envPROGRAMFILES = std::getenv("PROGRAMFILES");
        const char* const envPROGRAMFILESx86 = std::getenv("PROGRAMFILES(x86)");
        const char *const envCOMMONPROGRAMFILES = std::getenv("COMMONPROGRAMFILES");
        const char* const envCOMMONPROGRAMFILESx86 = std::getenv("COMMONPROGRAMFILES(x86)");

        // Small integrity tests
        if (envAPPDATA == nullptr)
        {
            qFatal("APPDATA variable not set, cannot continue");
            abort();
        }

        if (envPROGRAMFILES == nullptr)
        {
            qFatal("PROGRAMFILES variable not set, cannot continue");
            abort();
        }

        if (envCOMMONPROGRAMFILES == nullptr)
        {
            qFatal("COMMONPROGRAMFILES variable not set, cannot continue");
            abort();
        }

        const QCarlaString APPDATA(envAPPDATA);
        const QCarlaString LOCALAPPDATA(envLOCALAPPDATA);
        const QCarlaString PROGRAMFILES(envPROGRAMFILES);
        const QCarlaString COMMONPROGRAMFILES(envCOMMONPROGRAMFILES);
       #elif !defined(CARLA_OS_MAC)
        const QCarlaString CONFIG_HOME(getEnvWithFallback("XDG_CONFIG_HOME", (HOME + "/.config").toUtf8()));
       #endif

       #ifndef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
        // now set paths, listing format path spec if available
        if (const char *const envLADSPA = std::getenv("LADSPA_PATH"))
        {
            ladspa = envLADSPA;
        }
        else
        {
            // no official spec, use common paths
           #if defined(CARLA_OS_WIN)
            ladspa  = APPDATA + "\\LADSPA";
            ladspa += ";" + PROGRAMFILES + "\\LADSPA";
           #elif defined(CARLA_OS_HAIKU)
            ladspa  = HOME + "/.ladspa";
            ladspa += ":/system/add-ons/media/ladspaplugins";
            ladspa += ":/system/lib/ladspa";
           #elif defined(CARLA_OS_MAC)
            ladspa  = HOME + "/Library/Audio/Plug-Ins/LADSPA";
            ladspa += ":/Library/Audio/Plug-Ins/LADSPA";
           #else
            ladspa  = HOME + "/.ladspa";
            ladspa += ":/usr/local/lib/ladspa";
            ladspa += ":/usr/lib/ladspa";
           #endif
        }

        if (const char *const envDSSI = std::getenv("DSSI_PATH"))
        {
            dssi = envDSSI;
        }
        else
        {
            // no official spec, use common paths
           #if defined(CARLA_OS_WIN)
            dssi  = APPDATA + "\\DSSI";
            dssi += ";" + PROGRAMFILES + "\\DSSI";
           #elif defined(CARLA_OS_HAIKU)
            dssi  = HOME + "/.dssi";
            dssi += ":/system/add-ons/media/dssiplugins";
            dssi += ":/system/lib/dssi";
           #elif defined(CARLA_OS_MAC)
            dssi  = HOME + "/Library/Audio/Plug-Ins/DSSI";
            dssi += ":/Library/Audio/Plug-Ins/DSSI";
           #else
            dssi  = HOME + "/.dssi";
            dssi += ":/usr/local/lib/dssi";
            dssi += ":/usr/lib/dssi";
           #endif
        }
       #endif // !CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS

        if (const char *const envLV2 = std::getenv("LV2_PATH"))
        {
            lv2 = envLV2;
        }
        else
        {
            // https://lv2plug.in/pages/filesystem-hierarchy-standard.html
           #if defined(CARLA_OS_WIN)
            lv2 = APPDATA + "\\LV2";
            lv2 += ";" + COMMONPROGRAMFILES + "\\LV2";
           #elif defined(CARLA_OS_HAIKU)
            lv2  = HOME + "/.lv2";
            lv2 += ":/system/add-ons/media/lv2plugins";
           #elif defined(CARLA_OS_MAC)
            lv2 = HOME + "/Library/Audio/Plug-Ins/LV2";
            lv2 += ":/Library/Audio/Plug-Ins/LV2";
           #else
            lv2 = HOME + "/.lv2";
            lv2 += ":/usr/local/lib/lv2";
            lv2 += ":/usr/lib/lv2";
           #endif
        }

        if (const char *const envVST2 = std::getenv("VST_PATH"))
        {
            vst2 = envVST2;
        }
        else
        {
           #if defined(CARLA_OS_WIN)
            // https://helpcenter.steinberg.de/hc/en-us/articles/115000177084
            vst2 = PROGRAMFILES + "\\VSTPlugins";
            vst2 += ";" + PROGRAMFILES + "\\Steinberg\\VSTPlugins";
            vst2 += ";" + COMMONPROGRAMFILES + "\\VST2";
            vst2 += ";" + COMMONPROGRAMFILES + "\\Steinberg\\VST2";
           #elif defined(CARLA_OS_HAIKU)
            vst2  = HOME + "/.vst";
            vst2 += ":/system/add-ons/media/vstplugins";
           #elif defined(CARLA_OS_MAC)
            // https://helpcenter.steinberg.de/hc/en-us/articles/115000171310
            vst2 = HOME + "/Library/Audio/Plug-Ins/VST";
            vst2 += ":/Library/Audio/Plug-Ins/VST";
           #else
            // no official spec, use common paths
            vst2 = HOME + "/.vst";
            vst2 += ":" + HOME + "/.lxvst";
            vst2 += ":/usr/local/lib/vst";
            vst2 += ":/usr/local/lib/lxvst";
            vst2 += ":/usr/lib/vst";
            vst2 += ":/usr/lib/lxvst";
           #endif
        }

        if (const char *const envVST3 = std::getenv("VST3_PATH"))
        {
            vst3 = envVST3;
        }
        else
        {
            // https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Plugin+Locations.html
           #if defined(CARLA_OS_WIN)
            vst3 = LOCALAPPDATA + "\\Programs\\Common\\VST3";
            vst3 += ";" + COMMONPROGRAMFILES + "\\VST3";
           #elif defined(CARLA_OS_HAIKU)
            vst3  = HOME + "/.vst3";
            vst3 += ":/system/add-ons/media/vst3plugins";
           #elif defined(CARLA_OS_MAC)
            vst3 = HOME + "/Library/Audio/Plug-Ins/VST3";
            vst3 += ":/Library/Audio/Plug-Ins/VST3";
           #else
            vst3 = HOME + "/.vst3";
            vst3 += ":/usr/local/lib/vst3";
            vst3 += ":/usr/lib/vst3";
           #endif
        }

        if (const char *const envCLAP = std::getenv("CLAP_PATH"))
        {
            clap = envCLAP;
        }
        else
        {
            // https://github.com/free-audio/clap/blob/main/include/clap/entry.h
           #if defined(CARLA_OS_WIN)
            clap = LOCALAPPDATA + "\\Programs\\Common\\CLAP";
            clap += ";" + COMMONPROGRAMFILES + "\\CLAP";
           #elif defined(CARLA_OS_HAIKU)
            clap  = HOME + "/.clap";
            clap += ":/system/add-ons/media/clapplugins";
           #elif defined(CARLA_OS_MAC)
            clap = HOME + "/Library/Audio/Plug-Ins/CLAP";
            clap += ":/Library/Audio/Plug-Ins/CLAP";
           #else
            clap = HOME + "/.clap";
            clap += ":/usr/local/lib/clap";
            clap += ":/usr/lib/clap";
           #endif
        }

       #ifdef CARLA_OS_MAC
        au = HOME + "/Library/Audio/Plug-Ins/Components";
        au += ":/Library/Audio/Plug-Ins/Components";
        au += ":/System/Library/Components";
       #endif

       #ifndef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
        if (const char *const envJSFX = std::getenv("JSFX_PATH"))
        {
            jsfx = envJSFX;
        }
        else
        {
            // REAPER user data directory
           #if defined(CARLA_OS_WIN)
            jsfx = APPDATA + "\\REAPER\\Effects";
           #elif defined(CARLA_OS_MAC)
            jsfx = HOME + "/Library/Application Support/REAPER/Effects";
           #else
            jsfx = CONFIG_HOME + "/REAPER/Effects";
           #endif
        }

        if (const char *const envSF2 = std::getenv("SF2_PATH"))
        {
            sf2 = envSF2;
        }
        else
        {
           #if defined(CARLA_OS_WIN)
            sf2  = APPDATA + "\\SF2";
           #else
            sf2  = HOME + "/.sounds/sf2";
            sf2 += ":" + HOME + "/.sounds/sf3";
            sf2 += ":/usr/share/sounds/sf2";
            sf2 += ":/usr/share/sounds/sf3";
            sf2 += ":/usr/share/soundfonts";
           #endif
        }

        if (const char *const envSFZ = std::getenv("SFZ_PATH"))
        {
            sfz = envSFZ;
        }
        else
        {
           #if defined(CARLA_OS_WIN)
            sfz  = APPDATA + "\\SFZ";
           #else
            sfz  = HOME + "/.sounds/sfz";
            sfz += ":/usr/share/sounds/sfz";
           #endif
        }
       #endif // !CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS

       #ifdef CARLA_OS_WIN
        if (envPROGRAMFILESx86 != nullptr)
        {
            const QCarlaString PROGRAMFILESx86(envPROGRAMFILESx86);
           #ifndef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
            ladspa += ";" + PROGRAMFILESx86 + "\\LADSPA";
            dssi   += ";" + PROGRAMFILESx86 + "\\DSSI";
           #endif
            vst2   += ";" + PROGRAMFILESx86 + "\\VSTPlugins";
            vst2   += ";" + PROGRAMFILESx86 + "\\Steinberg\\VSTPlugins";
        }

        if (envCOMMONPROGRAMFILESx86 != nullptr)
        {
            const QCarlaString COMMONPROGRAMFILESx86(envCOMMONPROGRAMFILESx86);
            vst3   += COMMONPROGRAMFILESx86 + "\\VST3";
            clap   += COMMONPROGRAMFILESx86 + "\\CLAP";
        }
       #elif !defined(CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS)
        QCarlaString winePrefix;

        if (const char* const envWINEPREFIX = std::getenv("WINEPREFIX"))
            winePrefix = envWINEPREFIX;

        if (winePrefix.isEmpty())
            winePrefix = HOME + "/.wine";

        if (QDir(winePrefix).exists())
        {
            vst2 += ":" + winePrefix + "/drive_c/Program Files/VstPlugins";
            vst2 += ":" + winePrefix + "/drive_c/Program Files/Steinberg/VstPlugins";
            vst2 += ":" + winePrefix + "/drive_c/Program Files/Common Files/VST2";
            vst3 += ":" + winePrefix + "/drive_c/Program Files/Common Files/VST3";
            clap += ":" + winePrefix + "/drive_c/Program Files/Common Files/CLAP";

           #ifdef CARLA_OS_64BIT
            if (QDir(winePrefix + "/drive_c/Program Files (x86)").exists())
            {
                vst2 += ":" + winePrefix + "/drive_c/Program Files (x86)/VstPlugins";
                vst2 += ":" + winePrefix + "/drive_c/Program Files (x86)/Steinberg/VstPlugins";
                vst2 += ":" + winePrefix + "/drive_c/Program Files (x86)/Common Files/VST2";
                vst3 += ":" + winePrefix + "/drive_c/Program Files (x86)/Common Files/VST3";
                clap += ":" + winePrefix + "/drive_c/Program Files (x86)/Common Files/CLAP";
            }
           #endif
        }
       #endif
    }
};

// --------------------------------------------------------------------------------------------------------------------
// Backwards-compatible horizontalAdvance/width call, depending on Qt version

static inline
int fontMetricsHorizontalAdvance(const QFontMetrics& fontMetrics, const QString& string)
{
   #if QT_VERSION >= 0x50b00
    return fontMetrics.horizontalAdvance(string);
   #else
    return fontMetrics.width(string);
   #endif
}

// --------------------------------------------------------------------------------------------------------------------
// Qt-compatible plugin info

// convert PluginInfo to Qt types
static QVariant asByteArray(const PluginInfo& info)
{
    QByteArray qdata;

    // start with the POD data, stored as-is
    qdata.append(reinterpret_cast<const char*>(&info), sizeof(PluginInfoHeader));

    // then all the strings, with a null terminating byte
    {
        const QByteArray qcategory(info.category.toUtf8());
        qdata += qcategory.constData();
        qdata += '\0';
    }

    {
        const QByteArray qfilename(info.filename.toUtf8());
        qdata += qfilename.constData();
        qdata += '\0';
    }

    {
        const QByteArray qname(info.name.toUtf8());
        qdata += qname.constData();
        qdata += '\0';
    }

    {
        const QByteArray qlabel(info.label.toUtf8());
        qdata += qlabel.constData();
        qdata += '\0';
    }

    {
        const QByteArray qmaker(info.maker.toUtf8());
        qdata += qmaker.constData();
        qdata += '\0';
    }

    return qdata;
}

static QVariant asVariant(const PluginInfo& info)
{
    return QVariant(asByteArray(info));
}

// convert Qt types to PluginInfo
static PluginInfo asPluginInfo(const QByteArray &qdata)
{
    // make sure data is big enough to fit POD data + 5 strings
    CARLA_SAFE_ASSERT_RETURN(static_cast<size_t>(qdata.size()) >= sizeof(PluginInfoHeader) + sizeof(char) * 5, {});

    // read POD data first
    const PluginInfoHeader* const data = reinterpret_cast<const PluginInfoHeader*>(qdata.constData());

    PluginInfo info = {};
    info.build = data->build;
    info.type = data->type;
    info.hints = data->hints;
    info.uniqueId = data->uniqueId;
    info.audioIns = data->audioIns;
    info.audioOuts = data->audioOuts;
    info.cvIns = data->cvIns;
    info.cvOuts = data->cvOuts;
    info.midiIns = data->midiIns;
    info.midiOuts = data->midiOuts;
    info.parameterIns = data->parameterIns;
    info.parameterOuts = data->parameterOuts;

    // then all the strings, keeping the same order as in `asVariant`
    const char* sdata = reinterpret_cast<const char*>(data + 1);

    info.category = QString::fromUtf8(sdata);
    sdata += info.category.size() + 1;

    info.filename = QString::fromUtf8(sdata);
    sdata += info.filename.size() + 1;

    info.name = QString::fromUtf8(sdata);
    sdata += info.name.size() + 1;

    info.label = QString::fromUtf8(sdata);
    sdata += info.label.size() + 1;

    info.maker = QString::fromUtf8(sdata);
    sdata += info.maker.size() + 1;

    return info;
}

static PluginInfo asPluginInfo(const QVariant& var)
{
    return asPluginInfo(var.toByteArray());
}

static QList<PluginInfo> asPluginInfoList(const QVariant& var)
{
    QCarlaByteArray qdata(var.toByteArray());

    QList<PluginInfo> plist;

    while (!qdata.isEmpty())
    {
        const PluginInfo info = asPluginInfo(qdata);
        CARLA_SAFE_ASSERT_RETURN(info.build != BINARY_NONE, {});

        plist.append(info);
        qdata = qdata.sliced(sizeof(PluginInfoHeader)
                             + info.category.size() + info.filename.size() + info.name.size()
                             + info.label.size() + info.maker.size() + 5);
    }

    return plist;
}

// --------------------------------------------------------------------------------------------------------------------
// Qt-compatible plugin favorite

// base details, nicely packed and POD-only so we can directly use as binary
struct PluginFavoriteHeader {
    uint16_t type;
    uint64_t uniqueId;
};

// full details, now with non-POD types
struct PluginFavorite : PluginFavoriteHeader {
    QString filename;
    QString label;

    PluginFavorite()
    {
        type = PLUGIN_NONE;
        uniqueId = 0;
    }

    PluginFavorite(uint16_t t, uint64_t u, const QString& f, const QString& l)
        : filename(f), label(l)
    {
        type = t;
        uniqueId = u;
    }

    bool operator==(const PluginFavorite& other) const
    {
        return type == other.type && uniqueId == other.uniqueId && filename == other.filename && label == other.label;
    }
};

// convert PluginFavorite to Qt types
static QByteArray asByteArray(const PluginFavorite& fav)
{
    QByteArray qdata;

    // start with the POD data, stored as-is
    qdata.append(reinterpret_cast<const char*>(&fav), sizeof(PluginFavoriteHeader));

    // then all the strings, with a null terminating byte
    {
        const QByteArray qfilename(fav.filename.toUtf8());
        qdata += qfilename.constData();
        qdata += '\0';
    }

    {
        const QByteArray qlabel(fav.label.toUtf8());
        qdata += qlabel.constData();
        qdata += '\0';
    }

    return qdata;
}

static QVariant asVariant(const QList<PluginFavorite>& favlist)
{
    QByteArray qdata;

    for (const PluginFavorite &fav : favlist)
        qdata += asByteArray(fav);

    return QVariant(qdata);
}

// convert Qt types to PluginInfo
static PluginFavorite asPluginFavorite(const QByteArray& qdata)
{
    // make sure data is big enough to fit POD data + 3 strings
    CARLA_SAFE_ASSERT_RETURN(static_cast<size_t>(qdata.size()) >= sizeof(PluginFavoriteHeader) + sizeof(char) * 3, {});

    // read POD data first
    const PluginFavoriteHeader* const data = reinterpret_cast<const PluginFavoriteHeader*>(qdata.constData());
    PluginFavorite fav = { data->type, data->uniqueId, {}, {} };

    // then all the strings, keeping the same order as in `asVariant`
    const char* sdata = reinterpret_cast<const char*>(data + 1);

    fav.filename = QString::fromUtf8(sdata);
    sdata += fav.filename.size() + 1;

    fav.label = QString::fromUtf8(sdata);
    sdata += fav.label.size() + 1;

    return fav;
}

static QList<PluginFavorite> asPluginFavoriteList(const QVariant& var)
{
    QCarlaByteArray qdata(var.toByteArray());

    QList<PluginFavorite> favlist;

    while (!qdata.isEmpty())
    {
        const PluginFavorite fav = asPluginFavorite(qdata);
        CARLA_SAFE_ASSERT_RETURN(fav.type != PLUGIN_NONE, {});

        favlist.append(fav);
        qdata = qdata.sliced(sizeof(PluginFavoriteHeader) + fav.filename.size() + fav.label.size() + 2);
    }

    return favlist;
}

// create PluginFavorite from PluginInfo data
static PluginFavorite asPluginFavorite(const PluginInfo& info)
{
    return PluginFavorite(info.type, info.uniqueId, info.filename, info.label);
}

// --------------------------------------------------------------------------------------------------------------------
// discovery callbacks

static void discoveryCallback(void* const ptr, const CarlaPluginDiscoveryInfo* const info, const char* const sha1sum)
{
    static_cast<PluginListDialog*>(ptr)->addPluginInfo(info, sha1sum);
}

static bool checkCacheCallback(void* const ptr, const char* const filename, const char* const sha1sum)
{
    if (sha1sum == nullptr)
        return false;

    return static_cast<PluginListDialog*>(ptr)->checkPluginCache(filename, sha1sum);
}

// --------------------------------------------------------------------------------------------------------------------

struct PluginListDialog::PrivateData {
    int lastTableWidgetIndex = 0;
    int timerId = 0;
    PluginInfo retPlugin;

    // To be changed by parent
    bool hasLoadedLv2Plugins = false;

    struct Discovery {
        BinaryType btype = BINARY_NATIVE;
        PluginType ptype = PLUGIN_NONE;
        bool firstInit = true;
        bool ignoreCache = false;
        bool checkInvalid = false;
        bool usePluginBridges = false;
        bool useWineBridges = false;
        CarlaPluginDiscoveryHandle handle = nullptr;
        QCarlaString tool;
        ScopedPointer<PluginRefreshDialog> dialog;
        Discovery()
        {
            restart();
        }

        ~Discovery()
        {
            if (handle != nullptr)
                carla_plugin_discovery_stop(handle);
        }

       #ifndef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
        bool nextTool()
        {
            if (handle != nullptr)
            {
                carla_plugin_discovery_stop(handle);
                handle = nullptr;
            }

            if (! usePluginBridges)
                return false;

          #ifdef CARLA_OS_WIN
           #ifdef CARLA_OS_WIN64
            // look for win32 plugins on win64
            if (btype == BINARY_NATIVE)
            {
                btype = BINARY_WIN32;
                ptype = PLUGIN_NONE;
                tool = carla_get_library_folder();
                tool += CARLA_OS_SEP_STR "carla-discovery-win32.exe";

                if (QFile(tool).exists())
                    return true;
            }
           #endif

            // no other types to try
            return false;
          #else // CARLA_OS_WIN

           #ifndef CARLA_OS_MAC
            // try 32bit plugins on 64bit systems, skipping macOS where 32bit is no longer supported
            if (btype == BINARY_NATIVE)
            {
                btype = BINARY_POSIX32;
                ptype = PLUGIN_NONE;
                tool = carla_get_library_folder();
                tool += CARLA_OS_SEP_STR "carla-discovery-posix32";

                if (QFile(tool).exists())
                    return true;
            }
           #endif

            if (! useWineBridges)
                return false;

            // try wine bridges
           #ifdef CARLA_OS_64BIT
            if (btype == BINARY_NATIVE || btype == BINARY_POSIX32)
            {
                btype = BINARY_WIN64;
                ptype = PLUGIN_NONE;
                tool = carla_get_library_folder();
                tool += CARLA_OS_SEP_STR "carla-discovery-win64.exe";

                if (QFile(tool).exists())
                    return true;
            }
           #endif

            if (btype != BINARY_WIN32)
            {
                btype = BINARY_WIN32;
                ptype = PLUGIN_NONE;
                tool = carla_get_library_folder();
                tool += CARLA_OS_SEP_STR "carla-discovery-win32.exe";

                if (QFile(tool).exists())
                    return true;
            }

            return false;
          #endif // CARLA_OS_WIN
        }
       #endif // CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS

        void restart()
        {
            btype = BINARY_NATIVE;
            ptype = PLUGIN_NONE;
            tool = carla_get_library_folder();
            tool += CARLA_OS_SEP_STR "carla-discovery-native";
           #ifdef CARLA_OS_WIN
            tool += ".exe";
           #endif
        }
    } discovery;

    PluginPaths paths;

    struct {
       #ifndef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
        std::vector<PluginInfo> internal;
        std::vector<PluginInfo> ladspa;
        std::vector<PluginInfo> dssi;
       #endif
        std::vector<PluginInfo> lv2;
        std::vector<PluginInfo> vst2;
        std::vector<PluginInfo> vst3;
        std::vector<PluginInfo> clap;
       #ifdef CARLA_OS_MAC
        std::vector<PluginInfo> au;
       #endif
       #ifndef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
        std::vector<PluginInfo> jsfx;
        std::vector<PluginInfo> kits;
       #endif
        QMap<QString, QList<PluginInfo>> cache;
        QList<PluginFavorite> favorites;

        bool add(const PluginInfo& pinfo)
        {
            switch (pinfo.type)
            {
           #ifndef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
            case PLUGIN_INTERNAL: internal.push_back(pinfo); return true;
            case PLUGIN_LADSPA:   ladspa.push_back(pinfo);   return true;
            case PLUGIN_DSSI:     dssi.push_back(pinfo);     return true;
           #endif
            case PLUGIN_LV2:      lv2.push_back(pinfo);      return true;
            case PLUGIN_VST2:     vst2.push_back(pinfo);     return true;
            case PLUGIN_VST3:     vst3.push_back(pinfo);     return true;
            case PLUGIN_CLAP:     clap.push_back(pinfo);     return true;
           #ifdef CARLA_OS_MAC
            case PLUGIN_AU:       au.push_back(pinfo);       return true;
           #endif
           #ifndef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
            case PLUGIN_JSFX:     jsfx.push_back(pinfo);     return true;
            case PLUGIN_SF2:
            case PLUGIN_SFZ:      kits.push_back(pinfo);     return true;
           #endif
            default: return false;
            }
        }
    } plugins;
};

// --------------------------------------------------------------------------------------------------------------------
// Plugin List Dialog

PluginListDialog::PluginListDialog(QWidget* const parent, const HostSettings* const hostSettings)
    : QDialog(parent),
      p(new PrivateData)
{
    ui.setupUi(this);

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up global discovery options

    p->discovery.usePluginBridges = hostSettings->showPluginBridges;
    p->discovery.useWineBridges = hostSettings->showWineBridges;

   #ifndef CARLA_OS_WIN
    carla_plugin_discovery_set_option(ENGINE_OPTION_WINE_AUTO_PREFIX, hostSettings->wineAutoPrefix, nullptr);
    carla_plugin_discovery_set_option(ENGINE_OPTION_WINE_EXECUTABLE, 0, hostSettings->wineExecutable);
    carla_plugin_discovery_set_option(ENGINE_OPTION_WINE_FALLBACK_PREFIX, 0, hostSettings->wineFallbackPrefix);
   #endif

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up GUI

    ui.b_add->setEnabled(false);

    ui.tab_info->tabBar()->hide();
    ui.tab_reqs->tabBar()->hide();

   #ifdef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
    ui.ch_internal->hide();
    ui.ch_ladspa->hide();
    ui.ch_dssi->hide();
    ui.ch_au->hide();
    ui.ch_jsfx->hide();
    ui.ch_kits->hide();
    ui.ch_gui->hide();
    ui.ch_inline_display->hide();
    ui.toolBox->setItemEnabled(3, false);
   #endif

    // do not resize info frame so much
    const QLayout *const infoLayout = ui.tw_info->layout();
    const QMargins infoMargins = infoLayout->contentsMargins();
    ui.tab_info->setMinimumWidth(infoMargins.left() + infoMargins.right() + infoLayout->spacing() * 3
                                 + fontMetricsHorizontalAdvance(ui.la_id->fontMetrics(), "Has Custom GUI: 9999999999"));

    // start with no plugin selected
    checkPlugin(-1);

    // custom action that listens for Ctrl+F shortcut
    addAction(ui.act_focus_search);

   #ifdef CARLA_OS_64BIT
    ui.ch_bridged->setText(tr("Bridged (32bit)"));
   #else
    ui.ch_bridged->setChecked(false);
    ui.ch_bridged->setEnabled(false);
   #endif

   #if !(defined(CARLA_OS_LINUX) || defined(CARLA_OS_MAC))
    ui.ch_bridged_wine->setChecked(false);
    ui.ch_bridged_wine->setEnabled(false);
   #endif

   #ifdef CARLA_OS_MAC
    setWindowModality(Qt::WindowModal);
   #else
    ui.ch_au->setChecked(false);
    ui.ch_au->setEnabled(false);
    ui.ch_au->setVisible(false);
   #endif

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // ----------------------------------------------------------------------------------------------------------------
    // Load settings

    loadSettings();

    // ----------------------------------------------------------------------------------------------------------------
    // Disable bridges if not enabled in settings

#if 0
    // NOTE: We Assume win32 carla build will not run win64 plugins
    if (CARLA_OS_WIN and not CARLA_OS_64BIT) or not host.showPluginBridges:
        ui.ch_native.setChecked(True)
        ui.ch_native.setEnabled(False)
        ui.ch_native.setVisible(True)
        ui.ch_bridged.setChecked(False)
        ui.ch_bridged.setEnabled(False)
        ui.ch_bridged.setVisible(False)
        ui.ch_bridged_wine.setChecked(False)
        ui.ch_bridged_wine.setEnabled(False)
        ui.ch_bridged_wine.setVisible(False)

    elif not host.showWineBridges:
        ui.ch_bridged_wine.setChecked(False)
        ui.ch_bridged_wine.setEnabled(False)
        ui.ch_bridged_wine.setVisible(False)
#endif

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up Icons

    if (hostSettings->useSystemIcons)
    {
#if 0
        ui.b_add.setIcon(getIcon('list-add', 16, 'svgz'))
        ui.b_cancel.setIcon(getIcon('dialog-cancel', 16, 'svgz'))
        ui.b_clear_filters.setIcon(getIcon('edit-clear', 16, 'svgz'))
        ui.b_refresh.setIcon(getIcon('view-refresh', 16, 'svgz'))
        QTableWidgetItem* const hhi = ui.tableWidget->horizontalHeaderItem(TW_FAVORITE);
        hhi.setIcon(getIcon('bookmarks', 16, 'svgz'))
#endif
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up connections

    QObject::connect(this, &QDialog::finished, this, &PluginListDialog::saveSettings);
    QObject::connect(ui.b_add, &QPushButton::clicked, this, &QDialog::accept);
    QObject::connect(ui.b_cancel, &QPushButton::clicked, this, &QDialog::reject);

    QObject::connect(ui.b_refresh, &QPushButton::clicked, this, &PluginListDialog::refreshPlugins);
    QObject::connect(ui.b_clear_filters, &QPushButton::clicked, this, &PluginListDialog::clearFilters);
    QObject::connect(ui.lineEdit, &QLineEdit::textChanged, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.tableWidget, &QTableWidget::currentCellChanged, this, &PluginListDialog::checkPlugin);
    QObject::connect(ui.tableWidget, &QTableWidget::cellClicked, this, &PluginListDialog::cellClicked);
    QObject::connect(ui.tableWidget, &QTableWidget::cellDoubleClicked, this, &PluginListDialog::cellDoubleClicked);

    QObject::connect(ui.ch_internal, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_ladspa, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_dssi, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_lv2, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_vst, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_vst3, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_clap, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_au, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_jsfx, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_kits, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_effects, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_instruments, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_midi, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_other, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_native, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_bridged, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_bridged_wine, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_favorites, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_rtsafe, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_cv, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_gui, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_inline_display, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_stereo, &QCheckBox::clicked, this, &PluginListDialog::checkFilters);
    QObject::connect(ui.ch_cat_all, &QCheckBox::clicked, this, &PluginListDialog::checkFiltersCategoryAll);
    QObject::connect(ui.ch_cat_delay, &QCheckBox::clicked, this, &PluginListDialog::checkFiltersCategorySpecific);
    QObject::connect(ui.ch_cat_distortion, &QCheckBox::clicked, this, &PluginListDialog::checkFiltersCategorySpecific);
    QObject::connect(ui.ch_cat_dynamics, &QCheckBox::clicked, this, &PluginListDialog::checkFiltersCategorySpecific);
    QObject::connect(ui.ch_cat_eq, &QCheckBox::clicked, this, &PluginListDialog::checkFiltersCategorySpecific);
    QObject::connect(ui.ch_cat_filter, &QCheckBox::clicked, this, &PluginListDialog::checkFiltersCategorySpecific);
    QObject::connect(ui.ch_cat_modulator, &QCheckBox::clicked, this, &PluginListDialog::checkFiltersCategorySpecific);
    QObject::connect(ui.ch_cat_synth, &QCheckBox::clicked, this, &PluginListDialog::checkFiltersCategorySpecific);
    QObject::connect(ui.ch_cat_utility, &QCheckBox::clicked, this, &PluginListDialog::checkFiltersCategorySpecific);
    QObject::connect(ui.ch_cat_other, &QCheckBox::clicked, this, &PluginListDialog::checkFiltersCategorySpecific);

    QObject::connect(ui.act_focus_search, &QAction::triggered, this, &PluginListDialog::focusSearchFieldAndSelectAll);
}

PluginListDialog::~PluginListDialog()
{
    if (p->timerId != 0)
        killTimer(p->timerId);

    delete p;
}

// --------------------------------------------------------------------------------------------------------------------
// public methods

const PluginInfo& PluginListDialog::getSelectedPluginInfo() const
{
    return p->retPlugin;
}

void PluginListDialog::addPluginInfo(const CarlaPluginDiscoveryInfo* const info, const char* const sha1sum)
{
    if (info == nullptr)
    {
        if (sha1sum != nullptr)
        {
            QSafeSettings settings("falkTX", "CarlaDatabase3");
            settings.setValue(QString("PluginCache/%1").arg(sha1sum), QByteArray());

            const QString qsha1sum(sha1sum);
            p->plugins.cache[qsha1sum] = {};
        }
        return;
    }

    PluginInfo pinfo = {};
    pinfo.build = static_cast<uint16_t>(info->btype);
    pinfo.type = static_cast<uint16_t>(info->ptype);
    pinfo.hints = info->metadata.hints;
    pinfo.uniqueId = info->uniqueId;
    pinfo.audioIns = static_cast<uint16_t>(info->io.audioIns);
    pinfo.audioOuts = static_cast<uint16_t>(info->io.audioOuts);
    pinfo.cvIns = static_cast<uint16_t>(info->io.cvIns);
    pinfo.cvOuts = static_cast<uint16_t>(info->io.cvOuts);
    pinfo.midiIns = static_cast<uint16_t>(info->io.midiIns);
    pinfo.midiOuts = static_cast<uint16_t>(info->io.midiOuts);
    pinfo.parameterIns = static_cast<uint16_t>(info->io.parameterIns);
    pinfo.parameterOuts = static_cast<uint16_t>(info->io.parameterOuts);
    pinfo.category = getPluginCategoryAsString(info->metadata.category);
    pinfo.filename = QString::fromUtf8(info->filename);
    pinfo.name = QString::fromUtf8(info->metadata.name);
    pinfo.label = QString::fromUtf8(info->label);
    pinfo.maker = QString::fromUtf8(info->metadata.maker);

    if (sha1sum != nullptr)
    {
        QSafeSettings settings("falkTX", "CarlaDatabase3");
        const QString qsha1sum(sha1sum);
        const QString key = QString("PluginCache/%1").arg(sha1sum);

        // single sha1sum can contain >1 plugin
        QByteArray qdata;
        if (p->plugins.cache.contains(qsha1sum))
            qdata = settings.valueByteArray(key);
        qdata += asVariant(pinfo).toByteArray();

        settings.setValue(key, qdata);

        p->plugins.cache[qsha1sum].append(pinfo);
    }

   #ifdef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
    if ((pinfo.hints & PLUGIN_HAS_CUSTOM_EMBED_UI) == 0x0)
        return;
   #endif

    p->plugins.add(pinfo);
}

bool PluginListDialog::checkPluginCache(const char* const filename, const char* const sha1sum)
{
    // sha1sum is always valid for this call
    const QString qsha1sum(sha1sum);

    if (filename != nullptr)
        p->discovery.dialog->progressBar->setFormat(filename);

    if (!p->plugins.cache.contains(qsha1sum))
        return false;

    const QList<PluginInfo>& plist(p->plugins.cache[qsha1sum]);

    if (plist.isEmpty())
        return p->discovery.ignoreCache || !p->discovery.checkInvalid;

    // if filename does not match, abort (hash collision?)
    if (filename == nullptr || plist.first().filename != filename)
    {
        p->plugins.cache.remove(qsha1sum);
        return false;
    }

    for (const PluginInfo& info : plist)
    {
       #ifdef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
        if ((info.hints & PLUGIN_HAS_CUSTOM_EMBED_UI) == 0x0)
            continue;
       #endif
        p->plugins.add(info);
    }

    return true;
}

void PluginListDialog::setPluginPath(const PluginType ptype, const char* const path)
{
    switch (ptype)
    {
    case PLUGIN_LV2:
        p->paths.lv2 = path;
        break;
    case PLUGIN_VST2:
        p->paths.vst2 = path;
        break;
    case PLUGIN_VST3:
        p->paths.vst3 = path;
        break;
    case PLUGIN_CLAP:
        p->paths.clap = path;
        break;
   #ifdef CARLA_OS_MAC
    case PLUGIN_AU:
        p->paths.au = path;
        break;
   #endif
   #ifndef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
    case PLUGIN_LADSPA:
        p->paths.ladspa = path;
        break;
    case PLUGIN_DSSI:
        p->paths.dssi = path;
        break;
    case PLUGIN_SF2:
        p->paths.sf2 = path;
        break;
    case PLUGIN_SFZ:
        p->paths.sfz = path;
        break;
    case PLUGIN_JSFX:
        p->paths.jsfx = path;
        break;
   #endif
    default:
        break;
    }
}

// --------------------------------------------------------------------------------------------------------------------
// protected methods

void PluginListDialog::done(const int r)
{
    if (r == QDialog::Accepted && ui.tableWidget->currentRow() >= 0)
    {
        QTableWidgetItem* const widget = ui.tableWidget->item(ui.tableWidget->currentRow(), TW_NAME);
        p->retPlugin = asPluginInfo(widget->data(Qt::UserRole + UR_PLUGIN_INFO));
    }
    else
    {
        p->retPlugin = {};
    }

    QDialog::done(r);
}

void PluginListDialog::showEvent(QShowEvent* const event)
{
    focusSearchFieldAndSelectAll();
    QDialog::showEvent(event);

    // Set up initial discovery
    if (p->discovery.firstInit)
    {
        p->discovery.firstInit = false;

        p->discovery.dialog = new PluginRefreshDialog(this);
        p->discovery.dialog->b_start->setEnabled(false);
        p->discovery.dialog->b_skip->setEnabled(true);
        p->discovery.dialog->ch_updated->setChecked(true);
        p->discovery.dialog->ch_invalid->setChecked(false);
        p->discovery.dialog->group->setEnabled(false);
        p->discovery.dialog->group_formats->hide();
        p->discovery.dialog->progressBar->setFormat("Starting initial discovery...");
        p->discovery.dialog->adjustSize();

        QObject::connect(p->discovery.dialog->b_skip, &QPushButton::clicked,
                         this, &PluginListDialog::refreshPluginsSkip);
        QObject::connect(p->discovery.dialog, &QDialog::finished,
                         this, &PluginListDialog::refreshPluginsStop);

        p->timerId = startTimer(0);

        QTimer::singleShot(0, p->discovery.dialog, &QDialog::exec);
    }
}

void PluginListDialog::timerEvent(QTimerEvent* const event)
{
    if (event->timerId() == p->timerId)
    {
        do {
            // discovery in progress, keep it going
            if (p->discovery.handle != nullptr)
            {
                if (!carla_plugin_discovery_idle(p->discovery.handle))
                {
                    carla_plugin_discovery_stop(p->discovery.handle);
                    p->discovery.handle = nullptr;
                }
                break;
            }

            // start next discovery
            QCarlaString path;
            switch (p->discovery.ptype)
            {
            case PLUGIN_NONE:
           #ifndef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
                if (p->discovery.btype == BINARY_NATIVE)
                {
                    ui.label->setText(tr("Discovering internal plugins..."));
                    p->discovery.ptype = PLUGIN_INTERNAL;
                    break;
                }
                [[fallthrough]];
            case PLUGIN_INTERNAL:
                if (p->discovery.dialog->ch_ladspa->isChecked() || !p->discovery.dialog->group_formats->isVisible())
                {
                    ui.label->setText(tr("Discovering LADSPA plugins..."));
                    path = p->paths.ladspa;
                    p->discovery.ptype = PLUGIN_LADSPA;
                    break;
                }
                [[fallthrough]];
            case PLUGIN_LADSPA:
                if (p->discovery.dialog->ch_dssi->isChecked() || !p->discovery.dialog->group_formats->isVisible())
                {
                    ui.label->setText(tr("Discovering DSSI plugins..."));
                    path = p->paths.dssi;
                    p->discovery.ptype = PLUGIN_DSSI;
                    break;
                }
                [[fallthrough]];
            case PLUGIN_DSSI:
           #endif
                if (p->discovery.dialog->ch_lv2->isChecked() || !p->discovery.dialog->group_formats->isVisible())
                {
                    if (p->discovery.btype == BINARY_NATIVE && p->paths.lv2.isNotEmpty())
                    {
                        ui.label->setText(tr("Discovering LV2 plugins..."));
                        path = p->paths.lv2;
                        p->discovery.ptype = PLUGIN_LV2;
                        break;
                    }
                }
                [[fallthrough]];
            case PLUGIN_LV2:
                if (p->discovery.dialog->ch_vst2->isChecked() || !p->discovery.dialog->group_formats->isVisible())
                {
                    ui.label->setText(tr("Discovering VST2 plugins..."));
                    path = p->paths.vst2;
                    p->discovery.ptype = PLUGIN_VST2;
                    break;
                }
                [[fallthrough]];
            case PLUGIN_VST2:
                if (p->discovery.dialog->ch_vst3->isChecked() || !p->discovery.dialog->group_formats->isVisible())
                {
                    ui.label->setText(tr("Discovering VST3 plugins..."));
                    path = p->paths.vst3;
                    p->discovery.ptype = PLUGIN_VST3;
                    break;
                }
                [[fallthrough]];
            case PLUGIN_VST3:
                if (p->discovery.dialog->ch_clap->isChecked() || !p->discovery.dialog->group_formats->isVisible())
                {
                    ui.label->setText(tr("Discovering CLAP plugins..."));
                    path = p->paths.clap;
                    p->discovery.ptype = PLUGIN_CLAP;
                    break;
                }
                [[fallthrough]];
            case PLUGIN_CLAP:
           #ifdef CARLA_OS_MAC
                if (p->discovery.dialog->ch_au->isChecked() || !p->discovery.dialog->group_formats->isVisible())
                {
                    ui.label->setText(tr("Discovering AU plugins..."));
                    path = p->paths.au;
                    p->discovery.ptype = PLUGIN_AU;
                    break;
                }
                [[fallthrough]];
            case PLUGIN_AU:
           #endif
          #ifndef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
                if (p->discovery.dialog->ch_jsfx->isChecked() || !p->discovery.dialog->group_formats->isVisible())
                {
                    if (p->discovery.btype == BINARY_NATIVE && p->paths.jsfx.isNotEmpty())
                    {
                        ui.label->setText(tr("Discovering JSFX plugins..."));
                        path = p->paths.jsfx;
                        p->discovery.ptype = PLUGIN_JSFX;
                        break;
                    }
                }
                [[fallthrough]];
            case PLUGIN_JSFX:
                if (p->discovery.dialog->ch_sf2->isChecked() || !p->discovery.dialog->group_formats->isVisible())
                {
                    if (p->discovery.btype == BINARY_NATIVE && p->paths.sf2.isNotEmpty())
                    {
                        ui.label->setText(tr("Discovering SF2 kits..."));
                        path = p->paths.sf2;
                        p->discovery.ptype = PLUGIN_SF2;
                        break;
                    }
                }
                [[fallthrough]];
            case PLUGIN_SF2:
                if (p->discovery.dialog->ch_sfz->isChecked() || !p->discovery.dialog->group_formats->isVisible())
                {
                    if (p->discovery.btype == BINARY_NATIVE && p->paths.sfz.isNotEmpty())
                    {
                        ui.label->setText(tr("Discovering SFZ kits..."));
                        path = p->paths.sfz;
                        p->discovery.ptype = PLUGIN_SFZ;
                        break;
                    }
                }
                [[fallthrough]];
            case PLUGIN_SFZ:
          #endif
            default:
                // discovery complete?
               #ifndef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
                if (p->discovery.nextTool())
                    continue;
               #endif

                refreshPluginsStop();
                break;
            }

            if (p->timerId == 0)
                break;

            if (p->discovery.dialog != nullptr)
                p->discovery.dialog->progressBar->setFormat(ui.label->text());

            p->discovery.handle = carla_plugin_discovery_start(p->discovery.tool.toUtf8().constData(),
                                                               p->discovery.btype,
                                                               p->discovery.ptype,
                                                               path.toUtf8().constData(),
                                                               discoveryCallback, checkCacheCallback, this);
        } while (false);
    }

    QDialog::timerEvent(event);
}

// --------------------------------------------------------------------------------------------------------------------
// private methods

void PluginListDialog::addPluginsToTable()
{
    // ----------------------------------------------------------------------------------------------------------------
    // sum plugins first, creating all needed rows in advance

    ui.tableWidget->setSortingEnabled(false);
    ui.tableWidget->clearContents();

   #ifdef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
    ui.tableWidget->setRowCount(
        int(p->plugins.lv2.size() + p->plugins.vst2.size() + p->plugins.vst3.size() + p->plugins.clap.size()));

    constexpr const char* const txt = "Have %1 LV2, %2 VST2, %3 VST3 and %4 CLAP plugins";

    ui.label->setText(tr(txt)
        .arg(QString::number(p->plugins.lv2.size()))
        .arg(QString::number(p->plugins.vst2.size()))
        .arg(QString::number(p->plugins.vst3.size()))
        .arg(QString::number(p->plugins.clap.size())));
   #else
    ui.tableWidget->setRowCount(
        int(p->plugins.internal.size() + p->plugins.ladspa.size() + p->plugins.dssi.size() +
            p->plugins.lv2.size() + p->plugins.vst2.size() + p->plugins.vst3.size() + p->plugins.clap.size() +
           #ifdef CARLA_OS_MAC
            p->plugins.au.size() +
           #endif
            p->plugins.jsfx.size() + p->plugins.kits.size()));

    constexpr const char* const txt = "Have %1 Internal, %2 LADSPA, %3 DSSI, %4 LV2, %5 VST2, %6 VST3, %7 CLAP"
                                     #ifdef CARLA_OS_MAC
                                      ", %8 AudioUnit and %9 JSFX plugins, plus %10 Sound Kits"
                                     #endif
                                      " and %8 JSFX plugins, plus %9 Sound Kits";

    ui.label->setText(tr(txt)
        .arg(QString::number(p->plugins.internal.size()))
        .arg(QString::number(p->plugins.ladspa.size()))
        .arg(QString::number(p->plugins.dssi.size()))
        .arg(QString::number(p->plugins.lv2.size()))
        .arg(QString::number(p->plugins.vst2.size()))
        .arg(QString::number(p->plugins.vst3.size()))
        .arg(QString::number(p->plugins.clap.size()))
       #ifdef CARLA_OS_MAC
        .arg(QString::number(p->plugins.au.size()))
       #endif
        .arg(QString::number(p->plugins.jsfx.size()))
        .arg(QString::number(p->plugins.kits.size())));
   #endif

    // ----------------------------------------------------------------------------------------------------------------
    // now add all plugins to the table

    auto addPluginToTable = [=](const PluginInfo& info) {
        const int index = p->lastTableWidgetIndex++;
        const bool isFav = p->plugins.favorites.contains(asPluginFavorite(info));

        QTableWidgetItem* const itemFav = new QTableWidgetItem;
        itemFav->setCheckState(isFav ? Qt::Checked : Qt::Unchecked);
        itemFav->setText(isFav ? " " : "  ");

        const QString pluginText = (info.name + info.label + info.maker + info.filename).toLower();
        ui.tableWidget->setItem(index, TW_FAVORITE, itemFav);
        ui.tableWidget->setItem(index, TW_NAME, new QTableWidgetItem(info.name));
        ui.tableWidget->setItem(index, TW_LABEL, new QTableWidgetItem(info.label));
        ui.tableWidget->setItem(index, TW_MAKER, new QTableWidgetItem(info.maker));
        ui.tableWidget->setItem(index, TW_BINARY, new QTableWidgetItem(QFileInfo(info.filename).fileName()));

        QTableWidgetItem *const itemName = ui.tableWidget->item(index, TW_NAME);
        itemName->setData(Qt::UserRole + UR_PLUGIN_INFO, asVariant(info));
        itemName->setData(Qt::UserRole + UR_SEARCH_TEXT, pluginText);
    };

    p->lastTableWidgetIndex = 0;

   #ifndef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
    for (const PluginInfo &plugin : p->plugins.internal)
        addPluginToTable(plugin);

    for (const PluginInfo &plugin : p->plugins.ladspa)
        addPluginToTable(plugin);

    for (const PluginInfo &plugin : p->plugins.dssi)
        addPluginToTable(plugin);
   #endif

    for (const PluginInfo &plugin : p->plugins.lv2)
        addPluginToTable(plugin);

    for (const PluginInfo &plugin : p->plugins.vst2)
        addPluginToTable(plugin);

    for (const PluginInfo &plugin : p->plugins.vst3)
        addPluginToTable(plugin);

    for (const PluginInfo& plugin : p->plugins.clap)
        addPluginToTable(plugin);

   #ifdef CARLA_OS_MAC
    for (const PluginInfo& plugin : p->plugins.au)
        addPluginToTable(plugin);
   #endif

   #ifndef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
    for (const PluginInfo& plugin : p->plugins.jsfx)
        addPluginToTable(plugin);

    for (const PluginInfo& plugin : p->plugins.kits)
        addPluginToTable(plugin);
   #endif

    CARLA_SAFE_ASSERT_INT2(p->lastTableWidgetIndex == ui.tableWidget->rowCount(),
                           p->lastTableWidgetIndex, ui.tableWidget->rowCount());

    // ----------------------------------------------------------------------------------------------------------------
    // and reenable sorting + filtering

    ui.tableWidget->setSortingEnabled(true);

    checkFilters();
    checkPlugin(ui.tableWidget->currentRow());
}

void PluginListDialog::loadSettings()
{
    const QSafeSettings settings("falkTX", "CarlaDatabase3");

    restoreGeometry(settings.valueByteArray("PluginDatabase/Geometry"));
    ui.ch_effects->setChecked(settings.valueBool("PluginDatabase/ShowEffects", true));
    ui.ch_instruments->setChecked(settings.valueBool("PluginDatabase/ShowInstruments", true));
    ui.ch_midi->setChecked(settings.valueBool("PluginDatabase/ShowMIDI", true));
    ui.ch_other->setChecked(settings.valueBool("PluginDatabase/ShowOther", true));
    ui.ch_internal->setChecked(settings.valueBool("PluginDatabase/ShowInternal", true));
    ui.ch_ladspa->setChecked(settings.valueBool("PluginDatabase/ShowLADSPA", true));
    ui.ch_dssi->setChecked(settings.valueBool("PluginDatabase/ShowDSSI", true));
    ui.ch_lv2->setChecked(settings.valueBool("PluginDatabase/ShowLV2", true));
    ui.ch_vst->setChecked(settings.valueBool("PluginDatabase/ShowVST2", true));
    ui.ch_vst3->setChecked(settings.valueBool("PluginDatabase/ShowVST3", true));
    ui.ch_clap->setChecked(settings.valueBool("PluginDatabase/ShowCLAP", true));
   #ifdef CARLA_OS_MAC
    ui.ch_au->setChecked(settings.valueBool("PluginDatabase/ShowAU", true));
   #endif
    ui.ch_jsfx->setChecked(settings.valueBool("PluginDatabase/ShowJSFX", true));
    ui.ch_kits->setChecked(settings.valueBool("PluginDatabase/ShowKits", true));
    ui.ch_native->setChecked(settings.valueBool("PluginDatabase/ShowNative", true));
    ui.ch_bridged->setChecked(settings.valueBool("PluginDatabase/ShowBridged", true));
    ui.ch_bridged_wine->setChecked(settings.valueBool("PluginDatabase/ShowBridgedWine", true));
    ui.ch_favorites->setChecked(settings.valueBool("PluginDatabase/ShowFavorites", false));
    ui.ch_rtsafe->setChecked(settings.valueBool("PluginDatabase/ShowRtSafe", false));
    ui.ch_cv->setChecked(settings.valueBool("PluginDatabase/ShowHasCV", false));
    ui.ch_gui->setChecked(settings.valueBool("PluginDatabase/ShowHasGUI", false));
    ui.ch_inline_display->setChecked(settings.valueBool("PluginDatabase/ShowHasInlineDisplay", false));
    ui.ch_stereo->setChecked(settings.valueBool("PluginDatabase/ShowStereoOnly", false));
    ui.lineEdit->setText(settings.valueString("PluginDatabase/SearchText", ""));

    const QString categories = settings.valueString("PluginDatabase/ShowCategory", "all");
    if (categories == "all" or categories.length() < 2)
    {
        ui.ch_cat_all->setChecked(true);
        ui.ch_cat_delay->setChecked(false);
        ui.ch_cat_distortion->setChecked(false);
        ui.ch_cat_dynamics->setChecked(false);
        ui.ch_cat_eq->setChecked(false);
        ui.ch_cat_filter->setChecked(false);
        ui.ch_cat_modulator->setChecked(false);
        ui.ch_cat_synth->setChecked(false);
        ui.ch_cat_utility->setChecked(false);
        ui.ch_cat_other->setChecked(false);
    }
    else
    {
        ui.ch_cat_all->setChecked(false);
        ui.ch_cat_delay->setChecked(categories.contains(":delay:"));
        ui.ch_cat_distortion->setChecked(categories.contains(":distortion:"));
        ui.ch_cat_dynamics->setChecked(categories.contains(":dynamics:"));
        ui.ch_cat_eq->setChecked(categories.contains(":eq:"));
        ui.ch_cat_filter->setChecked(categories.contains(":filter:"));
        ui.ch_cat_modulator->setChecked(categories.contains(":modulator:"));
        ui.ch_cat_synth->setChecked(categories.contains(":synth:"));
        ui.ch_cat_utility->setChecked(categories.contains(":utility:"));
        ui.ch_cat_other->setChecked(categories.contains(":other:"));
    }

    const QByteArray tableGeometry = settings.valueByteArray("PluginDatabase/TableGeometry");
    QHeaderView* const horizontalHeader = ui.tableWidget->horizontalHeader();
    if (! tableGeometry.isNull())
    {
        horizontalHeader->restoreState(tableGeometry);
    }
    else
    {
        ui.tableWidget->setColumnWidth(TW_NAME, 250);
        ui.tableWidget->setColumnWidth(TW_LABEL, 200);
        ui.tableWidget->setColumnWidth(TW_MAKER, 150);
        ui.tableWidget->sortByColumn(TW_NAME, Qt::AscendingOrder);
    }

    horizontalHeader->setSectionResizeMode(TW_FAVORITE, QHeaderView::Fixed);
    ui.tableWidget->setColumnWidth(TW_FAVORITE, 24);
    ui.tableWidget->setSortingEnabled(true);
    
    p->plugins.favorites = asPluginFavoriteList(settings.valueByteArray("PluginListDialog/Favorites"));

    // load entire plugin cache
    const QStringList keys = settings.allKeys();
    for (const QCarlaString key : keys)
    {
        if (!key.startsWith("PluginCache/"))
            continue;

        const QByteArray data(settings.valueByteArray(key));

        if (data.isEmpty())
            p->plugins.cache.insert(key.sliced(12), {});
        else
            p->plugins.cache.insert(key.sliced(12), asPluginInfoList(data));
    }

   #ifdef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
    // these are not visible, force their value
    ui.ch_native->setChecked(true);
    ui.ch_bridged->setChecked(false);
    ui.ch_bridged_wine->setChecked(false);
    ui.ch_inline_display->setChecked(false);
   #endif
}

// --------------------------------------------------------------------------------------------------------------------
// private slots

void PluginListDialog::cellClicked(const int row, const int column)
{
    if (column != TW_FAVORITE)
        return;

    const PluginInfo info = asPluginInfo(ui.tableWidget->item(row, TW_NAME)->data(Qt::UserRole + UR_PLUGIN_INFO));
    const PluginFavorite fav = asPluginFavorite(info);
    const bool isFavorite = p->plugins.favorites.contains(fav);

    if (ui.tableWidget->item(row, TW_FAVORITE)->checkState() == Qt::Checked)
    {
        if (!isFavorite)
            p->plugins.favorites.append(fav);
    }
    else if (isFavorite)
    {
        p->plugins.favorites.removeAll(fav);
    }

    QSafeSettings settings("falkTX", "CarlaDatabase3");
    settings.setValue("PluginListDialog/Favorites", asVariant(p->plugins.favorites));
}

void PluginListDialog::cellDoubleClicked(int, const int column)
{
    if (column != TW_FAVORITE)
        done(QDialog::Accepted);
}

void PluginListDialog::focusSearchFieldAndSelectAll()
{
    ui.lineEdit->setFocus();
    ui.lineEdit->selectAll();
}

void PluginListDialog::checkFilters()
{
    const QCarlaString text = ui.lineEdit->text().toLower();

    const bool hideEffects     = !ui.ch_effects->isChecked();
    const bool hideInstruments = !ui.ch_instruments->isChecked();
    const bool hideMidi        = !ui.ch_midi->isChecked();
    const bool hideOther       = !ui.ch_other->isChecked();

    const bool hideInternal = !ui.ch_internal->isChecked();
    const bool hideLadspa   = !ui.ch_ladspa->isChecked();
    const bool hideDSSI     = !ui.ch_dssi->isChecked();
    const bool hideLV2      = !ui.ch_lv2->isChecked();
    const bool hideVST2     = !ui.ch_vst->isChecked();
    const bool hideVST3     = !ui.ch_vst3->isChecked();
    const bool hideCLAP     = !ui.ch_clap->isChecked();
    const bool hideAU       = !ui.ch_au->isChecked();
    const bool hideJSFX     = !ui.ch_jsfx->isChecked();
    const bool hideKits     = !ui.ch_kits->isChecked();

    const bool hideNative  = !ui.ch_native->isChecked();
    const bool hideBridged = !ui.ch_bridged->isChecked();
    const bool hideBridgedWine = !ui.ch_bridged_wine->isChecked();

    const bool hideNonFavs   = ui.ch_favorites->isChecked();
    const bool hideNonRtSafe = ui.ch_rtsafe->isChecked();
    const bool hideNonCV     = ui.ch_cv->isChecked();
    const bool hideNonGui    = ui.ch_gui->isChecked();
    const bool hideNonIDisp  = ui.ch_inline_display->isChecked();
    const bool hideNonStereo = ui.ch_stereo->isChecked();

   #if defined(CARLA_OS_WIN64)
    static constexpr const BinaryType nativeBins[2] = { BINARY_WIN32, BINARY_WIN64 };
    static constexpr const BinaryType wineBins[2] = { BINARY_NONE, BINARY_NONE };
   #elif defined(CARLA_OS_WIN32)
    static constexpr const BinaryType nativeBins[2] = { BINARY_WIN32, BINARY_NONE };
    static constexpr const BinaryType wineBins[2] = { BINARY_NONE, BINARY_NONE };
   #elif defined(CARLA_OS_MAC)
    static constexpr const BinaryType nativeBins[2] = { BINARY_POSIX64, BINARY_NONE };
    static constexpr const BinaryType wineBins[2] = { BINARY_WIN32, BINARY_WIN64 };
   #else
    static constexpr const BinaryType nativeBins[2] = { BINARY_POSIX32, BINARY_POSIX64 };
    static constexpr const BinaryType wineBins[2] = { BINARY_WIN32, BINARY_WIN64 };
   #endif

    for (int i=0, c=ui.tableWidget->rowCount(); i<c; ++i)
    {
        const PluginInfo info = asPluginInfo(ui.tableWidget->item(i, TW_NAME)->data(Qt::UserRole + UR_PLUGIN_INFO));
        const QString ptext = ui.tableWidget->item(i, TW_NAME)->data(Qt::UserRole + UR_SEARCH_TEXT).toString();

        const uint16_t aIns   = info.audioIns;
        const uint16_t aOuts  = info.audioOuts;
        const uint16_t cvIns  = info.cvIns;
        const uint16_t cvOuts = info.cvOuts;
        const uint16_t mIns   = info.midiIns;
        const uint16_t mOuts  = info.midiOuts;
        const uint32_t phints = info.hints;
        const uint16_t ptype  = info.type;
        const QString categ = info.category;
        const bool isSynth  = phints & PLUGIN_IS_SYNTH;
        const bool isEffect = aIns > 0 && aOuts > 0  && !isSynth;
        const bool isMidi   = aIns == 0 && aOuts == 0 && mIns > 0 && mOuts > 0;
        const bool isKit    = ptype == PLUGIN_SF2 || ptype == PLUGIN_SFZ;
        const bool isOther  = !(isEffect || isSynth || isMidi || isKit);
        const bool isNative = info.build == BINARY_NATIVE;
        const bool isRtSafe = phints & PLUGIN_IS_RTSAFE;
        const bool isStereo = (aIns == 2 && aOuts == 2) || (isSynth && aOuts == 2);
        const bool hasCV    = cvIns + cvOuts > 0;
        const bool hasGui   = phints & PLUGIN_HAS_CUSTOM_UI;
        const bool hasIDisp = phints & PLUGIN_HAS_INLINE_DISPLAY;
        const bool isBridged = !isNative && (nativeBins[0] == info.build || nativeBins[1] == info.build);
        const bool isBridgedWine = !isNative && (wineBins[0] == info.build || wineBins[1] == info.build);

        const auto hasText = [text, ptext]() {
            const QStringList textSplit = text.strip().split(' ');
            for (const QString& t : textSplit)
                if (ptext.contains(t))
                    return true;
            return false;
        };

        /**/ if (hideEffects && isEffect)
            ui.tableWidget->hideRow(i);
        else if (hideInstruments && isSynth)
            ui.tableWidget->hideRow(i);
        else if (hideMidi && isMidi)
            ui.tableWidget->hideRow(i);
        else if (hideOther && isOther)
            ui.tableWidget->hideRow(i);
        else if (hideKits && isKit)
            ui.tableWidget->hideRow(i);
        else if (hideInternal && ptype == PLUGIN_INTERNAL)
            ui.tableWidget->hideRow(i);
        else if (hideLadspa && ptype == PLUGIN_LADSPA)
            ui.tableWidget->hideRow(i);
        else if (hideDSSI && ptype == PLUGIN_DSSI)
            ui.tableWidget->hideRow(i);
        else if (hideLV2 && ptype == PLUGIN_LV2)
            ui.tableWidget->hideRow(i);
        else if (hideVST2 && ptype == PLUGIN_VST2)
            ui.tableWidget->hideRow(i);
        else if (hideVST3 && ptype == PLUGIN_VST3)
            ui.tableWidget->hideRow(i);
        else if (hideCLAP && ptype == PLUGIN_CLAP)
            ui.tableWidget->hideRow(i);
        else if (hideAU && ptype == PLUGIN_AU)
            ui.tableWidget->hideRow(i);
        else if (hideJSFX && ptype == PLUGIN_JSFX)
            ui.tableWidget->hideRow(i);
        else if (hideNative && isNative)
            ui.tableWidget->hideRow(i);
        else if (hideBridged && isBridged)
            ui.tableWidget->hideRow(i);
        else if (hideBridgedWine && isBridgedWine)
            ui.tableWidget->hideRow(i);
        else if (hideNonRtSafe && not isRtSafe)
            ui.tableWidget->hideRow(i);
        else if (hideNonCV && not hasCV)
            ui.tableWidget->hideRow(i);
        else if (hideNonGui && not hasGui)
            ui.tableWidget->hideRow(i);
        else if (hideNonIDisp && not hasIDisp)
            ui.tableWidget->hideRow(i);
        else if (hideNonStereo && not isStereo)
            ui.tableWidget->hideRow(i);
        else if (text.isNotEmpty() && ! hasText())
            ui.tableWidget->hideRow(i);
        else if (hideNonFavs && !p->plugins.favorites.contains(asPluginFavorite(info)))
            ui.tableWidget->hideRow(i);
        else if (ui.ch_cat_all->isChecked() or
                    (ui.ch_cat_delay->isChecked() && categ == "delay") or
                    (ui.ch_cat_distortion->isChecked() && categ == "distortion") or
                    (ui.ch_cat_dynamics->isChecked() && categ == "dynamics") or
                    (ui.ch_cat_eq->isChecked() && categ == "eq") or
                    (ui.ch_cat_filter->isChecked() && categ == "filter") or
                    (ui.ch_cat_modulator->isChecked() && categ == "modulator") or
                    (ui.ch_cat_synth->isChecked() && categ == "synth") or
                    (ui.ch_cat_utility->isChecked() && categ == "utility") or
                    (ui.ch_cat_other->isChecked() && categ == "other"))
            ui.tableWidget->showRow(i);
        else
            ui.tableWidget->hideRow(i);
    }
}

void PluginListDialog::checkFiltersCategoryAll(const bool clicked)
{
    const bool notClicked = !clicked;
    ui.ch_cat_delay->setChecked(notClicked);
    ui.ch_cat_distortion->setChecked(notClicked);
    ui.ch_cat_dynamics->setChecked(notClicked);
    ui.ch_cat_eq->setChecked(notClicked);
    ui.ch_cat_filter->setChecked(notClicked);
    ui.ch_cat_modulator->setChecked(notClicked);
    ui.ch_cat_synth->setChecked(notClicked);
    ui.ch_cat_utility->setChecked(notClicked);
    ui.ch_cat_other->setChecked(notClicked);
    checkFilters();
}

void PluginListDialog::checkFiltersCategorySpecific(bool clicked)
{
    if (clicked)
    {
        ui.ch_cat_all->setChecked(false);
    }
    else if (! (ui.ch_cat_delay->isChecked() ||
                ui.ch_cat_distortion->isChecked() ||
                ui.ch_cat_dynamics->isChecked() ||
                ui.ch_cat_eq->isChecked() ||
                ui.ch_cat_filter->isChecked() ||
                ui.ch_cat_modulator->isChecked() ||
                ui.ch_cat_synth->isChecked() ||
                ui.ch_cat_utility->isChecked() ||
                ui.ch_cat_other->isChecked()))
    {
        ui.ch_cat_all->setChecked(true);
    }
    checkFilters();
}

void PluginListDialog::clearFilters()
{
    auto setCheckedWithoutSignaling = [](QCheckBox* const w, const bool checked)
    {
        w->blockSignals(true);
        w->setChecked(checked);
        w->blockSignals(false);
    };

    setCheckedWithoutSignaling(ui.ch_internal, true);
    setCheckedWithoutSignaling(ui.ch_ladspa, true);
    setCheckedWithoutSignaling(ui.ch_dssi, true);
    setCheckedWithoutSignaling(ui.ch_lv2, true);
    setCheckedWithoutSignaling(ui.ch_vst, true);
    setCheckedWithoutSignaling(ui.ch_vst3, true);
    setCheckedWithoutSignaling(ui.ch_clap, true);
    setCheckedWithoutSignaling(ui.ch_jsfx, true);
    setCheckedWithoutSignaling(ui.ch_kits, true);

    setCheckedWithoutSignaling(ui.ch_instruments, true);
    setCheckedWithoutSignaling(ui.ch_effects, true);
    setCheckedWithoutSignaling(ui.ch_midi, true);
    setCheckedWithoutSignaling(ui.ch_other, true);

    setCheckedWithoutSignaling(ui.ch_native, true);
    setCheckedWithoutSignaling(ui.ch_bridged, false);
    setCheckedWithoutSignaling(ui.ch_bridged_wine, false);

    setCheckedWithoutSignaling(ui.ch_favorites, false);
    setCheckedWithoutSignaling(ui.ch_rtsafe, false);
    setCheckedWithoutSignaling(ui.ch_stereo, false);
    setCheckedWithoutSignaling(ui.ch_cv, false);
    setCheckedWithoutSignaling(ui.ch_gui, false);
    setCheckedWithoutSignaling(ui.ch_inline_display, false);

    if (ui.ch_au->isEnabled())
        setCheckedWithoutSignaling(ui.ch_au, true);

    setCheckedWithoutSignaling(ui.ch_cat_all, true);
    setCheckedWithoutSignaling(ui.ch_cat_delay, false);
    setCheckedWithoutSignaling(ui.ch_cat_distortion, false);
    setCheckedWithoutSignaling(ui.ch_cat_dynamics, false);
    setCheckedWithoutSignaling(ui.ch_cat_eq, false);
    setCheckedWithoutSignaling(ui.ch_cat_filter, false);
    setCheckedWithoutSignaling(ui.ch_cat_modulator, false);
    setCheckedWithoutSignaling(ui.ch_cat_synth, false);
    setCheckedWithoutSignaling(ui.ch_cat_utility, false);
    setCheckedWithoutSignaling(ui.ch_cat_other, false);

    ui.lineEdit->blockSignals(true);
    ui.lineEdit->clear();
    ui.lineEdit->blockSignals(false);

    checkFilters();
}

// --------------------------------------------------------------------------------------------------------------------

void PluginListDialog::checkPlugin(const int row)
{
    if (row >= 0)
    {
        ui.b_add->setEnabled(true);

        const PluginInfo info = asPluginInfo(ui.tableWidget->item(row, TW_NAME)->data(Qt::UserRole + UR_PLUGIN_INFO));

        const bool isSynth  = info.hints & PLUGIN_IS_SYNTH;
        const bool isEffect = info.audioIns > 0 && info.audioOuts > 0 && !isSynth;
        const bool isMidi   = info.audioIns == 0 && info.audioOuts == 0 && info.midiIns > 0 && info.midiOuts > 0;

        QString ptype;
        /**/ if (isSynth)
            ptype = "Instrument";
        else if (isEffect)
            ptype = "Effect";
        else if (isMidi)
            ptype = "MIDI Plugin";
        else
            ptype = "Other";

        QString parch;
        /**/ if (info.build == BINARY_NATIVE)
            parch = tr("Native");
        else if (info.build == BINARY_POSIX32)
            parch = "posix32";
        else if (info.build == BINARY_POSIX64)
            parch = "posix64";
        else if (info.build == BINARY_WIN32)
            parch = "win32";
        else if (info.build == BINARY_WIN64)
            parch = "win64";
        else if (info.build == BINARY_OTHER)
            parch = tr("Other");
        else if (info.build == BINARY_WIN32)
            parch = tr("Unknown");

        ui.l_format->setText(getPluginTypeAsString(static_cast<PluginType>(info.type)));

        ui.l_type->setText(ptype);
        ui.l_arch->setText(parch);
        ui.l_id->setText(QString::number(info.uniqueId));
        ui.l_ains->setText(QString::number(info.audioIns));
        ui.l_aouts->setText(QString::number(info.audioOuts));
        ui.l_cvins->setText(QString::number(info.cvIns));
        ui.l_cvouts->setText(QString::number(info.cvOuts));
        ui.l_mins->setText(QString::number(info.midiIns));
        ui.l_mouts->setText(QString::number(info.midiOuts));
        ui.l_pins->setText(QString::number(info.parameterIns));
        ui.l_pouts->setText(QString::number(info.parameterOuts));
        ui.l_gui->setText(info.hints & PLUGIN_HAS_CUSTOM_UI ? tr("Yes") : tr("No"));
        ui.l_idisp->setText(info.hints & PLUGIN_HAS_INLINE_DISPLAY ? tr("Yes") : tr("No"));
        ui.l_bridged->setText(info.hints & PLUGIN_IS_BRIDGE ? tr("Yes") : tr("No"));
        ui.l_synth->setText(isSynth ? tr("Yes") : tr("No"));
    }
    else
    {
        ui.b_add->setEnabled(false);
        ui.l_format->setText("---");
        ui.l_type->setText("---");
        ui.l_arch->setText("---");
        ui.l_id->setText("---");
        ui.l_ains->setText("---");
        ui.l_aouts->setText("---");
        ui.l_cvins->setText("---");
        ui.l_cvouts->setText("---");
        ui.l_mins->setText("---");
        ui.l_mouts->setText("---");
        ui.l_pins->setText("---");
        ui.l_pouts->setText("---");
        ui.l_gui->setText("---");
        ui.l_idisp->setText("---");
        ui.l_bridged->setText("---");
        ui.l_synth->setText("---");
    }
}

// --------------------------------------------------------------------------------------------------------------------

void PluginListDialog::refreshPlugins()
{
    refreshPluginsStop();

    p->discovery.dialog = new PluginRefreshDialog(this);

    QObject::connect(p->discovery.dialog->b_start, &QPushButton::clicked,
                     this, &PluginListDialog::refreshPluginsStart);
    QObject::connect(p->discovery.dialog->b_skip, &QPushButton::clicked,
                     this, &PluginListDialog::refreshPluginsSkip);
    QObject::connect(p->discovery.dialog, &QDialog::finished,
                     this, &PluginListDialog::refreshPluginsStop);

    p->discovery.dialog->exec();
}

void PluginListDialog::refreshPluginsStart()
{
    // remove old plugins
   #ifndef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
    p->plugins.internal.clear();
    p->plugins.ladspa.clear();
    p->plugins.dssi.clear();
   #endif
    p->plugins.lv2.clear();
    p->plugins.vst2.clear();
    p->plugins.vst3.clear();
    p->plugins.clap.clear();
   #ifdef CARLA_OS_MAC
    p->plugins.au.clear();
   #endif
   #ifndef CARLA_FRONTEND_ONLY_EMBEDDABLE_PLUGINS
    p->plugins.jsfx.clear();
    p->plugins.kits.clear();
   #endif
    p->discovery.dialog->b_start->setEnabled(false);
    p->discovery.dialog->b_skip->setEnabled(true);
    p->discovery.ignoreCache = p->discovery.dialog->ch_all->isChecked();
    p->discovery.checkInvalid =
        p->discovery.dialog->ch_invalid->isChecked();
    if (p->discovery.ignoreCache)
        p->plugins.cache.clear();

    // start discovery again
    p->discovery.restart();

    if (p->timerId == 0)
        p->timerId = startTimer(0);
}

void PluginListDialog::refreshPluginsStop()
{
    // stop previous discovery if still running
    if (p->discovery.handle != nullptr)
    {
        carla_plugin_discovery_stop(p->discovery.handle);
        p->discovery.handle = nullptr;
    }

    if (p->discovery.dialog)
    {
        p->discovery.dialog->close();
        p->discovery.dialog = nullptr;
    }

    if (p->timerId != 0)
    {
        killTimer(p->timerId);
        p->timerId = 0;
        addPluginsToTable();
    }
}

void PluginListDialog::refreshPluginsSkip()
{
    if (p->discovery.handle != nullptr)
        carla_plugin_discovery_skip(p->discovery.handle);
}

// --------------------------------------------------------------------------------------------------------------------

void PluginListDialog::saveSettings()
{
    QSafeSettings settings("falkTX", "CarlaDatabase3");

    settings.setValue("PluginDatabase/Geometry", saveGeometry());
    settings.setValue("PluginDatabase/TableGeometry", ui.tableWidget->horizontalHeader()->saveState());
    settings.setValue("PluginDatabase/ShowEffects", ui.ch_effects->isChecked());
    settings.setValue("PluginDatabase/ShowInstruments", ui.ch_instruments->isChecked());
    settings.setValue("PluginDatabase/ShowMIDI", ui.ch_midi->isChecked());
    settings.setValue("PluginDatabase/ShowOther", ui.ch_other->isChecked());
    settings.setValue("PluginDatabase/ShowInternal", ui.ch_internal->isChecked());
    settings.setValue("PluginDatabase/ShowLADSPA", ui.ch_ladspa->isChecked());
    settings.setValue("PluginDatabase/ShowDSSI", ui.ch_dssi->isChecked());
    settings.setValue("PluginDatabase/ShowLV2", ui.ch_lv2->isChecked());
    settings.setValue("PluginDatabase/ShowVST2", ui.ch_vst->isChecked());
    settings.setValue("PluginDatabase/ShowVST3", ui.ch_vst3->isChecked());
    settings.setValue("PluginDatabase/ShowCLAP", ui.ch_clap->isChecked());
    settings.setValue("PluginDatabase/ShowAU", ui.ch_au->isChecked());
    settings.setValue("PluginDatabase/ShowJSFX", ui.ch_jsfx->isChecked());
    settings.setValue("PluginDatabase/ShowKits", ui.ch_kits->isChecked());
    settings.setValue("PluginDatabase/ShowNative", ui.ch_native->isChecked());
    settings.setValue("PluginDatabase/ShowBridged", ui.ch_bridged->isChecked());
    settings.setValue("PluginDatabase/ShowBridgedWine", ui.ch_bridged_wine->isChecked());
    settings.setValue("PluginDatabase/ShowFavorites", ui.ch_favorites->isChecked());
    settings.setValue("PluginDatabase/ShowRtSafe", ui.ch_rtsafe->isChecked());
    settings.setValue("PluginDatabase/ShowHasCV", ui.ch_cv->isChecked());
    settings.setValue("PluginDatabase/ShowHasGUI", ui.ch_gui->isChecked());
    settings.setValue("PluginDatabase/ShowHasInlineDisplay", ui.ch_inline_display->isChecked());
    settings.setValue("PluginDatabase/ShowStereoOnly", ui.ch_stereo->isChecked());
    settings.setValue("PluginDatabase/SearchText", ui.lineEdit->text());

    if (ui.ch_cat_all->isChecked())
    {
        settings.setValue("PluginDatabase/ShowCategory", "all");
    }
    else
    {
        QCarlaString categories;
        if (ui.ch_cat_delay->isChecked())
            categories += ":delay";
        if (ui.ch_cat_distortion->isChecked())
            categories += ":distortion";
        if (ui.ch_cat_dynamics->isChecked())
            categories += ":dynamics";
        if (ui.ch_cat_eq->isChecked())
            categories += ":eq";
        if (ui.ch_cat_filter->isChecked())
            categories += ":filter";
        if (ui.ch_cat_modulator->isChecked())
            categories += ":modulator";
        if (ui.ch_cat_synth->isChecked())
            categories += ":synth";
        if (ui.ch_cat_utility->isChecked())
            categories += ":utility";
        if (ui.ch_cat_other->isChecked())
            categories += ":other";
        if (categories.isNotEmpty())
            categories += ":";
        settings.setValue("PluginDatabase/ShowCategory", categories);
    }

    settings.setValue("PluginListDialog/Favorites", asVariant(p->plugins.favorites));
}

// --------------------------------------------------------------------------------------------------------------------
