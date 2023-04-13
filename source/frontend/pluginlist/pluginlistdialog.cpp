/*
 * Carla plugin host
 * Copyright (C) 2011-2023 Filipe Coelho <falktx@falktx.com>
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

#include "pluginlistdialog.hpp"

#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated-copy-with-user-provided-copy"
# pragma clang diagnostic ignored "-Wdeprecated-register"
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wclass-memaccess"
# pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif

#include "ui_pluginlistdialog.h"
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QList>

#ifdef __clang__
# pragma clang diagnostic pop
#elif defined(__GNUC__) && __GNUC__ >= 8
# pragma GCC diagnostic pop
#endif

#include "qcarlastring.hpp"
#include "qsafesettings.hpp"

#include "CarlaBackendUtils.hpp"
#include "CarlaFrontend.h"
#include "CarlaUtils.h"

#include "CarlaString.hpp"

#include <cstdlib>

CARLA_BACKEND_USE_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
// Carla Settings keys

#define CARLA_KEY_PATHS_LADSPA "Paths/LADSPA"
#define CARLA_KEY_PATHS_DSSI   "Paths/DSSI"
#define CARLA_KEY_PATHS_LV2    "Paths/LV2"
#define CARLA_KEY_PATHS_VST2   "Paths/VST2"
#define CARLA_KEY_PATHS_VST3   "Paths/VST3"
#define CARLA_KEY_PATHS_CLAP   "Paths/CLAP"
#define CARLA_KEY_PATHS_SF2    "Paths/SF2"
#define CARLA_KEY_PATHS_SFZ    "Paths/SFZ"
#define CARLA_KEY_PATHS_JSFX   "Paths/JSFX"

// --------------------------------------------------------------------------------------------------------------------
// Carla Settings defaults

// --------------------------------------------------------------------------------------------------------------------
// utils

const char* getEnv(const char* const env, const char* const fallback)
{
    if (const char* const value = std::getenv(env))
        return value;

    return fallback;
}

QCarlaString getTMP()
{
    QCarlaString tmp;

    if (const char* const envTMP = std::getenv("TMP"))
    {
        tmp = envTMP;
    }
    else
    {
#ifdef CARLA_OS_WIN
        qWarning("TMP variable not set")
#endif
        tmp = QDir::tempPath();
    }

    if (!QDir(tmp).exists())
    {
        qWarning("TMP does not exist");
        tmp = "/";
    }

    return tmp;
}

QCarlaString getHome()
{
    QCarlaString home;

    if (const char* const envHOME = std::getenv("HOME"))
    {
        home = envHOME;
    }
    else
    {
#ifndef CARLA_OS_WIN
        qWarning("HOME variable not set");
#endif
        home = QDir::toNativeSeparators(QDir::homePath());
    }

    if (!QDir(home).exists())
    {
        qWarning("HOME does not exist");
        home = getTMP();
    }

    return home;
}

// --------------------------------------------------------------------------------------------------------------------
// Default Plugin Folders (get)

struct DefaultPaths {
    QCarlaString ladspa;
    QCarlaString dssi;
    QCarlaString lv2;
    QCarlaString vst2;
    QCarlaString vst3;
    QCarlaString clap;
    QCarlaString sf2;
    QCarlaString sfz;
    QCarlaString jsfx;

    void init()
    {
        const QCarlaString HOME = getHome();

#if defined(CARLA_OS_WIN)
        APPDATA = os.getenv("APPDATA");
        LOCALAPPDATA = os.getenv("LOCALAPPDATA", APPDATA);
        PROGRAMFILES = os.getenv("PROGRAMFILES");
        PROGRAMFILESx86 = os.getenv("PROGRAMFILES(x86)");
        COMMONPROGRAMFILES = os.getenv("COMMONPROGRAMFILES");
        COMMONPROGRAMFILESx86 = os.getenv("COMMONPROGRAMFILES(x86)");

        // Small integrity tests
        if not APPDATA:
        {
            qFatal("APPDATA variable not set, cannot continue");
            sys.exit(1);
        }

        if not PROGRAMFILES:
        {
            qFatal("PROGRAMFILES variable not set, cannot continue");
            sys.exit(1);
        }

        if not COMMONPROGRAMFILES:
        {
            qFatal("COMMONPROGRAMFILES variable not set, cannot continue");
            sys.exit(1);
        }

        ladspa  = APPDATA + "\\LADSPA";
        ladspa += ";" + PROGRAMFILES + "\\LADSPA";

        dssi    = APPDATA + "\\DSSI";
        dssi   += ";" + PROGRAMFILES + "\\DSSI";

        lv2     = APPDATA + "\\LV2";
        lv2    += ";" + COMMONPROGRAMFILES + "\\LV2";

        vst2    = PROGRAMFILES + "\\VstPlugins";
        vst2   += ";" + PROGRAMFILES + "\\Steinberg\\VstPlugins";

        jsfx    = APPDATA + "\\REAPER\\Effects";
        //jsfx   += ";" + PROGRAMFILES + "\\REAPER\\InstallData\\Effects";

        // TODO ifdef 64bit build
       #if 1
        vst2  += ";" + COMMONPROGRAMFILES + "\\VST2";
       #endif

        vst3    = COMMONPROGRAMFILES + "\\VST3";
        vst3   += ";" + LOCALAPPDATA + "\\Programs\\Common\\VST3";

        clap    = COMMONPROGRAMFILES + "\\CLAP";
        clap   += ";" + LOCALAPPDATA + "\\Programs\\Common\\CLAP";

        DEFAULT_SF2_PATH     = APPDATA + "\\SF2";
        DEFAULT_SFZ_PATH     = APPDATA + "\\SFZ";

        if PROGRAMFILESx86:
            ladspa += ";" + PROGRAMFILESx86 + "\\LADSPA";
            dssi   += ";" + PROGRAMFILESx86 + "\\DSSI";
            vst2   += ";" + PROGRAMFILESx86 + "\\VstPlugins";
            vst2   += ";" + PROGRAMFILESx86 + "\\Steinberg\\VstPlugins";
            //jsfx   += ";" + PROGRAMFILESx86 + "\\REAPER\\InstallData\\Effects";

        if COMMONPROGRAMFILESx86:
            vst3   += COMMONPROGRAMFILESx86 + "\\VST3";
            clap   += COMMONPROGRAMFILESx86 + "\\CLAP";

#elif defined(CARLA_OS_HAIKU)
        ladspa  = HOME + "/.ladspa";
        ladspa += ":/system/add-ons/media/ladspaplugins";
        ladspa += ":/system/lib/ladspa";

        dssi    = HOME + "/.dssi";
        dssi   += ":/system/add-ons/media/dssiplugins";
        dssi   += ":/system/lib/dssi";

        lv2     = HOME + "/.lv2";
        lv2    += ":/system/add-ons/media/lv2plugins";

        vst2    = HOME + "/.vst";
        vst2   += ":/system/add-ons/media/vstplugins";

        vst3    = HOME + "/.vst3";
        vst3   += ":/system/add-ons/media/vst3plugins";

        clap    = HOME + "/.clap";
        clap   += ":/system/add-ons/media/clapplugins";

#elif defined(CARLA_OS_MAC)
        ladspa  = HOME + "/Library/Audio/Plug-Ins/LADSPA";
        ladspa += ":/Library/Audio/Plug-Ins/LADSPA";

        dssi    = HOME + "/Library/Audio/Plug-Ins/DSSI";
        dssi   += ":/Library/Audio/Plug-Ins/DSSI";

        lv2     = HOME + "/Library/Audio/Plug-Ins/LV2";
        lv2    += ":/Library/Audio/Plug-Ins/LV2";

        vst2    = HOME + "/Library/Audio/Plug-Ins/VST";
        vst2   += ":/Library/Audio/Plug-Ins/VST";

        vst3    = HOME + "/Library/Audio/Plug-Ins/VST3";
        vst3   += ":/Library/Audio/Plug-Ins/VST3";

        clap    = HOME + "/Library/Audio/Plug-Ins/CLAP";
        clap   += ":/Library/Audio/Plug-Ins/CLAP";

        jsfx    = HOME + "/Library/Application Support/REAPER/Effects";
        //jsfx   += ":/Applications/REAPER.app/Contents/InstallFiles/Effects";

#else
        const QCarlaString CONFIG_HOME(getEnv("XDG_CONFIG_HOME", (HOME + "/.config").toUtf8()));

        ladspa  = HOME + "/.ladspa";
        ladspa += ":/usr/lib/ladspa";
        ladspa += ":/usr/local/lib/ladspa";

        dssi    = HOME + "/.dssi";
        dssi   += ":/usr/lib/dssi";
        dssi   += ":/usr/local/lib/dssi";

        lv2     = HOME + "/.lv2";
        lv2    += ":/usr/lib/lv2";
        lv2    += ":/usr/local/lib/lv2";

        vst2    = HOME + "/.vst";
        vst2   += ":/usr/lib/vst";
        vst2   += ":/usr/local/lib/vst";

        vst2   += HOME + "/.lxvst";
        vst2   += ":/usr/lib/lxvst";
        vst2   += ":/usr/local/lib/lxvst";

        vst3    = HOME + "/.vst3";
        vst3   += ":/usr/lib/vst3";
        vst3   += ":/usr/local/lib/vst3";

        clap    = HOME + "/.clap";
        clap   += ":/usr/lib/clap";
        clap   += ":/usr/local/lib/clap";

        sf2     = HOME + "/.sounds/sf2";
        sf2    += ":" + HOME + "/.sounds/sf3";
        sf2    += ":/usr/share/sounds/sf2";
        sf2    += ":/usr/share/sounds/sf3";
        sf2    += ":/usr/share/soundfonts";

        sfz     = HOME + "/.sounds/sfz";
        sfz    += ":/usr/share/sounds/sfz";

        jsfx    = CONFIG_HOME + "/REAPER/Effects";
        //jsfx   += ":" + "/opt/REAPER/InstallData/Effects";
#endif

#ifndef CARLA_OS_WIN
        QCarlaString winePrefix;

        if (const char* const envWINEPREFIX = std::getenv("WINEPREFIX"))
            winePrefix = envWINEPREFIX;

        if (winePrefix.isEmpty())
            winePrefix = HOME + "/.wine";

        if (QDir(winePrefix).exists())
        {
            vst2 += ":" + winePrefix + "/drive_c/Program Files/VstPlugins";
            vst3 += ":" + winePrefix + "/drive_c/Program Files/Common Files/VST3";
            clap += ":" + winePrefix + "/drive_c/Program Files/Common Files/CLAP";

            // TODO ifdef 64bit build
            if (QDir(winePrefix + "/drive_c/Program Files (x86)").exists())
            {
                vst2 += ":" + winePrefix + "/drive_c/Program Files (x86)/VstPlugins";
                vst3 += ":" + winePrefix + "/drive_c/Program Files (x86)/Common Files/VST3";
                clap += ":" + winePrefix + "/drive_c/Program Files (x86)/Common Files/CLAP";
            }
        }
#endif
    }

    void loadFromEnv()
    {
        if (const char* const envLADSPA = std::getenv("LADSPA_PATH"))
            ladspa = envLADSPA;
        if (const char* const envDSSI = std::getenv("DSSI_PATH"))
            dssi = envDSSI;
        if (const char* const envLV2 = std::getenv("LV2_PATH"))
            lv2 = envLV2;
        if (const char* const envVST = std::getenv("VST_PATH"))
            vst2 = envVST;
        if (const char* const envVST3 = std::getenv("VST3_PATH"))
            vst3 = envVST3;
        if (const char* const envCLAP = std::getenv("CLAP_PATH"))
            clap = envCLAP;
        if (const char* const envSF2 = std::getenv("SF2_PATH"))
            sf2 = envSF2;
        if (const char* const envSFZ = std::getenv("SFZ_PATH"))
            sfz = envSFZ;
        if (const char* const envJSFX = std::getenv("JSFX_PATH"))
            jsfx = envJSFX;
    }
};

// --------------------------------------------------------------------------------------------------------------------

PluginInfo checkPluginCached(const CarlaCachedPluginInfo* const desc, const PluginType ptype)
{
    PluginInfo pinfo = {};
    pinfo.API   = PLUGIN_QUERY_API_VERSION;
    pinfo.build = BINARY_NATIVE;
    pinfo.type  = ptype;
    pinfo.hints = desc->hints;
    pinfo.name  = desc->name;
    pinfo.label = desc->label;
    pinfo.maker = desc->maker;
    pinfo.category = getPluginCategoryAsString(desc->category);

    pinfo.audioIns  = desc->audioIns;
    pinfo.audioOuts = desc->audioOuts;

    pinfo.cvIns  = desc->cvIns;
    pinfo.cvOuts = desc->cvOuts;

    pinfo.midiIns  = desc->midiIns;
    pinfo.midiOuts = desc->midiOuts;

    pinfo.parametersIns  = desc->parameterIns;
    pinfo.parametersOuts = desc->parameterOuts;

    switch (ptype)
    {
    case PLUGIN_LV2:
        {
            const QString label(desc->label);
            pinfo.filename = label.split(CARLA_OS_SEP).first();
            pinfo.label = label.section(CARLA_OS_SEP, 1);
        }
        break;
    case PLUGIN_SFZ:
        pinfo.filename = pinfo.label;
        pinfo.label    = pinfo.name;
        break;
    default:
        break;
    }

    return pinfo;
}

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

typedef QList<PluginInfo> QPluginInfoList;

class QSafePluginListSettings : public QSafeSettings
{
public:
    inline QSafePluginListSettings()
        : QSafeSettings() {}

    inline QSafePluginListSettings(const QString& organization, const QString& application)
        : QSafeSettings(organization, application) {}

    QPluginInfoList valuePluginInfoList(const QString& key) const
    {
        /*
        QVariant var(value(key, {}));

        if (!var.isNull() && var.convert(QVariant::List) && var.isValid())
        {
            QList<QVariant> varVariant(var.toList());
            QPluginInfoList varReal;

            varReal.reserve(varVariant.size());

            for (QVariant v : varVariant)
            {
                CARLA_SAFE_ASSERT_BREAK(!v.isNull());
                CARLA_SAFE_ASSERT_BREAK(v.convert(QVariant::UserType));
                CARLA_SAFE_ASSERT_BREAK(v.isValid());
                // varReal.append(v.toU);
            }
        }
        */

        return {};
    }

    void setValue(const QString& key, const uint value)
    {
        QSafeSettings::setValue(key, value);
    }

    void setValue(const QString& key, const QPluginInfoList& value)
    {
    }
};

// --------------------------------------------------------------------------------------------------------------------

template <class T> inline T static_cast_void(const void* ptr)
{
    return static_cast<T>(ptr);
}

struct PluginInfoBytes {
    uint API;
    uint build;
    uint type;
    uint hints;
    uint64_t uniqueId;
    uint audioIns;
    uint audioOuts;
    uint cvIns;
    uint cvOuts;
    uint midiIns;
    uint midiOuts;
    uint parametersIns;
    uint parametersOuts;
};

QVariant asVariant(const PluginInfo& plugin)
{
    const PluginInfoBytes data = {
        plugin.API,
        plugin.build,
        plugin.type,
        plugin.hints,
        plugin.uniqueId,
        plugin.audioIns,
        plugin.audioOuts,
        plugin.cvIns,
        plugin.cvOuts,
        plugin.midiIns,
        plugin.midiOuts,
        plugin.parametersIns,
        plugin.parametersOuts
    };
    QByteArray qdata(static_cast_void<const char*>(&data), sizeof(data));

    {
        const QByteArray qcategory(plugin.category.toUtf8());
        qdata += qcategory.constData();
        qdata += '\0';
    }

    {
        const QByteArray qfilename(plugin.filename.toUtf8());
        qdata += qfilename.constData();
        qdata += '\0';
    }

    {
        const QByteArray qname(plugin.name.toUtf8());
        qdata += qname.constData();
        qdata += '\0';
    }

    {
        const QByteArray qlabel(plugin.label.toUtf8());
        qdata += qlabel.constData();
        qdata += '\0';
    }

    {
        const QByteArray qmaker(plugin.maker.toUtf8());
        qdata += qmaker.constData();
        qdata += '\0';
    }

    QVariant var;
    var.setValue(qdata);
    return var;
}

PluginInfo asPluginInfo(const QVariant& var)
{
    const QByteArray qdata(var.toByteArray());
    CARLA_SAFE_ASSERT_RETURN((size_t)qdata.size() >= sizeof(PluginInfoBytes) + sizeof(char)*5, {});

    const PluginInfoBytes* const data = static_cast_void<const PluginInfoBytes*>(qdata.constData());
    PluginInfo plugin = {
        data->API,
        data->build,
        static_cast<PluginType>(data->type),
        data->hints,
        {}, {}, {}, {}, {},
        data->uniqueId,
        data->audioIns,
        data->audioOuts,
        data->cvIns,
        data->cvOuts,
        data->midiIns,
        data->midiOuts,
        data->parametersIns,
        data->parametersOuts
    };

    const char* sdata = static_cast_void<const char*>(data) + sizeof(PluginInfoBytes);

    plugin.category = QString::fromUtf8(sdata);
    sdata += plugin.category.size() + 1;

    plugin.filename = QString::fromUtf8(sdata);
    sdata += plugin.filename.size() + 1;

    plugin.name = QString::fromUtf8(sdata);
    sdata += plugin.name.size() + 1;

    plugin.label = QString::fromUtf8(sdata);
    sdata += plugin.label.size() + 1;

    plugin.maker = QString::fromUtf8(sdata);
    sdata += plugin.maker.size() + 1;

    return plugin;
}

// --------------------------------------------------------------------------------------------------------------------
// Plugin List Dialog

struct PluginListDialog::Self {
    enum TableIndex {
        TABLEWIDGET_ITEM_FAVORITE,
        TABLEWIDGET_ITEM_NAME,
        TABLEWIDGET_ITEM_LABEL,
        TABLEWIDGET_ITEM_MAKER,
        TABLEWIDGET_ITEM_BINARY,
    };

    // To be changed by parent
    bool hasLoadedLv2Plugins = false;

    Ui_PluginListDialog ui;
    HostSettings hostSettings = {};

    int fLastTableIndex = 0;
    PluginInfo fRetPlugin;
    QWidget* const fRealParent;
    QStringList fFavoritePlugins;
    bool fFavoritePluginsChanged = false;

    const QString fTrYes;
    const QString fTrNo;
    const QString fTrNative;

    Self(QWidget* const parent)
        : fRealParent(parent),
          fTrYes(tr("Yes")),
          fTrNo(tr("No")),
          fTrNative(tr("Native")) {}

    static Self& create(QWidget* const parent)
    {
        Self* const self = new Self(parent);
        return *self;
    }

    inline QString tr(const char* const txt)
    {
        return fRealParent != nullptr ? fRealParent->tr(txt) : QString::fromUtf8(txt);
    }

    void createFavoritePluginDict()
    {
#if 0
        return {
            'name'    : plugin['name'],
            'build'   : plugin['build'],
            'type'    : plugin['type'],
            'filename': plugin['filename'],
            'label'   : plugin['label'],
            'uniqueId': plugin['uniqueId'],
        }
#endif
    }

    void checkFilters()
    {
        const QCarlaString text = ui.lineEdit->text().toLower();

        const bool hideEffects     = !ui.ch_effects->isChecked();
        const bool hideInstruments = !ui.ch_instruments->isChecked();
        const bool hideMidi        = !ui.ch_midi->isChecked();
        const bool hideOther       = !ui.ch_other->isChecked();

        const bool hideInternal = !ui.ch_internal->isChecked();
        const bool hideLadspa   = !ui.ch_ladspa->isChecked();
        const bool hideDssi     = !ui.ch_dssi->isChecked();
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

#if 0
        if HAIKU or LINUX or MACOS:
            nativeBins = [BINARY_POSIX32, BINARY_POSIX64]
            wineBins   = [BINARY_WIN32, BINARY_WIN64]
        elif WINDOWS:
            nativeBins = [BINARY_WIN32, BINARY_WIN64]
            wineBins   = []
        else:
            nativeBins = []
            wineBins   = []
#endif

        ui.tableWidget->setRowCount(fLastTableIndex);

        for (int i=0; i<fLastTableIndex; ++i)
        {
            const PluginInfo plugin = asPluginInfo(ui.tableWidget->item(i, TABLEWIDGET_ITEM_NAME)->data(Qt::UserRole+1));

            const QString ptext  = ui.tableWidget->item(i, TABLEWIDGET_ITEM_NAME)->data(Qt::UserRole+2).toString();
            const uint aIns   = plugin.audioIns;
            const uint aOuts  = plugin.audioOuts;
            const uint cvIns  = plugin.cvIns;
            const uint cvOuts = plugin.cvOuts;
            const uint mIns   = plugin.midiIns;
            const uint mOuts  = plugin.midiOuts;
            const uint phints = plugin.hints;
            const PluginType ptype  = plugin.type;
            const QString categ = plugin.category;
            const bool isSynth  = phints & PLUGIN_IS_SYNTH;
            const bool isEffect = aIns > 0 && aOuts > 0  && !isSynth;
            const bool isMidi   = aIns == 0 && aOuts == 0 && mIns > 0 && mOuts > 0;
            const bool isKit    = ptype == PLUGIN_SF2 || ptype == PLUGIN_SFZ;
            const bool isOther  = !(isEffect || isSynth || isMidi || isKit);
            const bool isNative = plugin.build == BINARY_NATIVE;
            const bool isRtSafe = phints & PLUGIN_IS_RTSAFE;
            const bool isStereo = (aIns == 2 && aOuts == 2) || (isSynth && aOuts == 2);
            const bool hasCV    = cvIns + cvOuts > 0;
            const bool hasGui   = phints & PLUGIN_HAS_CUSTOM_UI;
            const bool hasIDisp = phints & PLUGIN_HAS_INLINE_DISPLAY;

#if 0
            const bool isBridged = bool(not isNative and plugin.build in nativeBins);
            const bool isBridgedWine = bool(not isNative and plugin.build in wineBins);
#else
            const bool isBridged = false;
            const bool isBridgedWine = false;
#endif

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
            else if (hideDssi && ptype == PLUGIN_DSSI)
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
#if 0
            else if (hideNonFavs && _createFavoritePluginDict(plugin) not in fFavoritePlugins)
                ui.tableWidget->hideRow(i);
#endif
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

    void addPluginToTable(const PluginInfo& plugin, const PluginType ptype)
    {
        if (plugin.API != PLUGIN_QUERY_API_VERSION)
            return;

        PluginInfo plugincopy = plugin;
        switch (ptype)
        {
        case PLUGIN_INTERNAL:
        case PLUGIN_LV2:
        case PLUGIN_SF2:
        case PLUGIN_SFZ:
        case PLUGIN_JSFX:
            plugincopy.build = BINARY_NATIVE;
            break;
        default:
            break;
        }

        const int index = fLastTableIndex;
#if 0
        const bool isFav = bool(self._createFavoritePluginDict(plugin) in self.fFavoritePlugins)
#else
        const bool isFav = false;
#endif
        QTableWidgetItem* const itemFav = new QTableWidgetItem;
        itemFav->setCheckState(isFav ? Qt::Checked : Qt::Unchecked);
        itemFav->setText(isFav ? " " : "  ");

        const QString pluginText = (plugin.name + plugin.label + plugin.maker + plugin.filename).toLower();
        ui.tableWidget->setItem(index, TABLEWIDGET_ITEM_FAVORITE, itemFav);
        ui.tableWidget->setItem(index, TABLEWIDGET_ITEM_NAME, new QTableWidgetItem(plugin.name));
        ui.tableWidget->setItem(index, TABLEWIDGET_ITEM_LABEL, new QTableWidgetItem(plugin.label));
        ui.tableWidget->setItem(index, TABLEWIDGET_ITEM_MAKER, new QTableWidgetItem(plugin.maker));
        ui.tableWidget->setItem(index, TABLEWIDGET_ITEM_BINARY, new QTableWidgetItem(QFileInfo(plugin.filename).fileName()));

        QTableWidgetItem* const itemName = ui.tableWidget->item(index, TABLEWIDGET_ITEM_NAME);
        itemName->setData(Qt::UserRole+1, asVariant(plugincopy));
        itemName->setData(Qt::UserRole+2, pluginText);

        fLastTableIndex += 1;
    }

    uint reAddInternalHelper(QSafePluginListSettings& settingsDB, const PluginType ptype, const char* const path)
    {
        QString ptypeStr, ptypeStrTr;

        switch (ptype)
        {
        case PLUGIN_INTERNAL:
            ptypeStr   = "Internal";
            ptypeStrTr = tr("Internal");
            break;
        case PLUGIN_LV2:
            ptypeStr   = "LV2";
            ptypeStrTr = ptypeStr;
            break;
        case PLUGIN_AU:
            ptypeStr   = "AU";
            ptypeStrTr = ptypeStr;
            break;
        case PLUGIN_SFZ:
            ptypeStr   = "SFZ";
            ptypeStrTr = ptypeStr;
            break;
        // TODO(jsfx) what to do here?
        default:
            return 0;
        }

        QPluginInfoList plugins = settingsDB.valuePluginInfoList("Plugins/" + ptypeStr);
        uint pluginCount = settingsDB.valueUInt("PluginCount/" + ptypeStr, 0);

        if (ptype == PLUGIN_AU)
            carla_juce_init();

        const uint pluginCountNew = carla_get_cached_plugin_count(ptype, path);

#if 0
        if (pluginCountNew != pluginCount || plugins.size() != pluginCount ||
            (plugins.size() > 0 && plugins[0].API != PLUGIN_QUERY_API_VERSION))
#endif
        {
            plugins.clear();
            pluginCount = pluginCountNew;

            QApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents, 50);

            if (ptype == PLUGIN_AU)
                carla_juce_idle();

            for (uint i=0; i<pluginCountNew; ++i)
            {
                const CarlaCachedPluginInfo* const descInfo = carla_get_cached_plugin_info(ptype, i);

                if (descInfo == nullptr)
                    continue;

                const PluginInfo info = checkPluginCached(descInfo, ptype);
                plugins.append(info);

                if ((i % 50) == 0)
                {
                    QApplication::instance()->processEvents(QEventLoop::ExcludeUserInputEvents, 50);

                    if (ptype == PLUGIN_AU)
                        carla_juce_idle();
                }
            }

            settingsDB.setValue("Plugins/" + ptypeStr, plugins);
            settingsDB.setValue("PluginCount/" + ptypeStr, pluginCount);
        }

        if (ptype == PLUGIN_AU)
            carla_juce_cleanup();

        // prepare rows in advance
        ui.tableWidget->setRowCount(fLastTableIndex + plugins.size());

        for (const PluginInfo& plugin : plugins)
            addPluginToTable(plugin, ptype);

        return pluginCount;
    }

    void reAddPlugins()
    {
        QSafePluginListSettings settingsDB("falkTX", "CarlaPlugins5");

        fLastTableIndex = 0;
        ui.tableWidget->setSortingEnabled(false);
        ui.tableWidget->clearContents();

        DefaultPaths paths;
        paths.init();
        paths.loadFromEnv();

        QCarlaString LV2_PATH;
        {
            const QSafeSettings settings("falkTX", "Carla2");
            LV2_PATH = settings.valueString(CARLA_KEY_PATHS_LV2, paths.lv2);
        }

        // ------------------------------------------------------------------------------------------------------------
        // plugins handled through backend

        const uint internalCount = reAddInternalHelper(settingsDB, PLUGIN_INTERNAL, "");
        const uint lv2Count = reAddInternalHelper(settingsDB, PLUGIN_LV2, LV2_PATH.toUtf8());
       #ifdef CARLA_OS_MAC
        const uint auCount = reAddInternalHelper(settingsDB, PLUGIN_AU, "");
       #else
        const uint auCount = 0;
       #endif

        // ------------------------------------------------------------------------------------------------------------
        // LADSPA

        QList<QPluginInfoList> ladspaPlugins;
        ladspaPlugins += settingsDB.valuePluginInfoList("Plugins/LADSPA_native");
        ladspaPlugins += settingsDB.valuePluginInfoList("Plugins/LADSPA_posix32");
        ladspaPlugins += settingsDB.valuePluginInfoList("Plugins/LADSPA_posix64");
        ladspaPlugins += settingsDB.valuePluginInfoList("Plugins/LADSPA_win32");
        ladspaPlugins += settingsDB.valuePluginInfoList("Plugins/LADSPA_win64");

        // ------------------------------------------------------------------------------------------------------------
        // DSSI

        QList<QPluginInfoList> dssiPlugins;
        dssiPlugins += settingsDB.valuePluginInfoList("Plugins/DSSI_native");
        dssiPlugins += settingsDB.valuePluginInfoList("Plugins/DSSI_posix32");
        dssiPlugins += settingsDB.valuePluginInfoList("Plugins/DSSI_posix64");
        dssiPlugins += settingsDB.valuePluginInfoList("Plugins/DSSI_win32");
        dssiPlugins += settingsDB.valuePluginInfoList("Plugins/DSSI_win64");

        // ------------------------------------------------------------------------------------------------------------
        // VST2

        QList<QPluginInfoList> vst2Plugins;
        vst2Plugins += settingsDB.valuePluginInfoList("Plugins/VST2_native");
        vst2Plugins += settingsDB.valuePluginInfoList("Plugins/VST2_posix32");
        vst2Plugins += settingsDB.valuePluginInfoList("Plugins/VST2_posix64");
        vst2Plugins += settingsDB.valuePluginInfoList("Plugins/VST2_win32");
        vst2Plugins += settingsDB.valuePluginInfoList("Plugins/VST2_win64");

        // ------------------------------------------------------------------------------------------------------------
        // VST3

        QList<QPluginInfoList> vst3Plugins;
        vst3Plugins += settingsDB.valuePluginInfoList("Plugins/VST3_native");
        vst3Plugins += settingsDB.valuePluginInfoList("Plugins/VST3_posix32");
        vst3Plugins += settingsDB.valuePluginInfoList("Plugins/VST3_posix64");
        vst3Plugins += settingsDB.valuePluginInfoList("Plugins/VST3_win32");
        vst3Plugins += settingsDB.valuePluginInfoList("Plugins/VST3_win64");

        // ------------------------------------------------------------------------------------------------------------
        // CLAP

        QList<QPluginInfoList> clapPlugins;
        clapPlugins += settingsDB.valuePluginInfoList("Plugins/CLAP_native");
        clapPlugins += settingsDB.valuePluginInfoList("Plugins/CLAP_posix32");
        clapPlugins += settingsDB.valuePluginInfoList("Plugins/CLAP_posix64");
        clapPlugins += settingsDB.valuePluginInfoList("Plugins/CLAP_win32");
        clapPlugins += settingsDB.valuePluginInfoList("Plugins/CLAP_win64");

       #ifdef CARLA_OS_MAC
        // ------------------------------------------------------------------------------------------------------------
        // AU (extra non-cached)

        QList<QPluginInfoList> auPlugins32 = settingsDB.valuePluginInfoList("Plugins/AU_posix32");
       #endif

        // ------------------------------------------------------------------------------------------------------------
        // JSFX

        QPluginInfoList jsfxPlugins = settingsDB.valuePluginInfoList("Plugins/JSFX");

        // ------------------------------------------------------------------------------------------------------------
        // Kits

        QList<QPluginInfoList> sf2s;
        sf2s += settingsDB.valuePluginInfoList("Plugins/SF2");

        QPluginInfoList sfzs = settingsDB.valuePluginInfoList("Plugins/SFZ");

        // ------------------------------------------------------------------------------------------------------------
        // count plugins first, so we can create rows in advance

        int ladspaCount = 0;
        int dssiCount   = 0;
        int vstCount    = 0;
        int vst3Count   = 0;
        int clapCount   = 0;
        int au32Count   = 0;
        int jsfxCount   = jsfxPlugins.size();
        int sf2Count    = 0;
        int sfzCount    = sfzs.size();

        for (const QPluginInfoList& plugins : ladspaPlugins)
            ladspaCount += plugins.size();

        for (const QPluginInfoList& plugins : dssiPlugins)
            dssiCount += plugins.size();

        for (const QPluginInfoList& plugins : vst2Plugins)
            vstCount += plugins.size();

        for (const QPluginInfoList& plugins : vst3Plugins)
            vst3Count += plugins.size();

        for (const QPluginInfoList& plugins : clapPlugins)
            clapCount += plugins.size();

       #ifdef CARLA_OS_MAC
        for (const QPluginInfoList& plugins : auPlugins32)
            au32Count += plugins.size();
       #endif

        for (const QPluginInfoList& plugins : sf2s)
            sf2Count += plugins.size();

        ui.tableWidget->setRowCount(fLastTableIndex +
                                    ladspaCount + dssiCount + vstCount + vst3Count + clapCount +
                                    auCount + au32Count + jsfxCount + sf2Count + sfzCount);

        constexpr const char* const txt = "Have %1 Internal, %2 LADSPA, %3 DSSI, %4 LV2, %5 VST2, %6 VST3, %7 CLAP"
                                         #ifdef CARLA_OS_MAC
                                          ", %8 AudioUnit and %9 JSFX plugins, plus %10 Sound Kits"
                                         #endif
                                          " and %8 JSFX plugins, plus %9 Sound Kits";

        ui.label->setText(fRealParent->tr(txt)
            .arg(QString::number(internalCount))
            .arg(QString::number(ladspaCount))
            .arg(QString::number(dssiCount))
            .arg(QString::number(lv2Count))
            .arg(QString::number(vstCount))
            .arg(QString::number(vst3Count))
            .arg(QString::number(clapCount))
           #ifdef CARLA_OS_MAC
            .arg(QString::number(auCount+au32Count))
           #endif
            .arg(QString::number(jsfxCount))
            .arg(QString::number(sf2Count+sfzCount))
        );

        // ------------------------------------------------------------------------------------------------------------
        // now add all plugins to the table

        for (const QPluginInfoList& plugins : ladspaPlugins)
            for (const PluginInfo& plugin : plugins)
                addPluginToTable(plugin, PLUGIN_LADSPA);

        for (const QPluginInfoList& plugins : dssiPlugins)
            for (const PluginInfo& plugin : plugins)
                addPluginToTable(plugin, PLUGIN_DSSI);

        for (const QPluginInfoList& plugins : vst2Plugins)
            for (const PluginInfo& plugin : plugins)
                addPluginToTable(plugin, PLUGIN_VST2);

        for (const QPluginInfoList& plugins : vst3Plugins)
            for (const PluginInfo& plugin : plugins)
                addPluginToTable(plugin, PLUGIN_VST3);

        for (const QPluginInfoList& plugins : clapPlugins)
            for (const PluginInfo& plugin : plugins)
                addPluginToTable(plugin, PLUGIN_CLAP);

       #ifdef CARLA_OS_MAC
        for (const QPluginInfoList& plugins : auPlugins32)
            for (const PluginInfo& plugin : plugins)
                addPluginToTable(plugin, PLUGIN_AU);
       #endif

        for (const PluginInfo& plugin : jsfxPlugins)
            addPluginToTable(plugin, PLUGIN_JSFX);

        for (const QPluginInfoList& sf2 : sf2s)
            for (const PluginInfo& sf2_i : sf2)
                addPluginToTable(sf2_i, PLUGIN_SF2);

        for (const PluginInfo& sfz : sfzs)
            addPluginToTable(sfz, PLUGIN_SFZ);

        // ------------------------------------------------------------------------------------------------------------

        ui.tableWidget->setSortingEnabled(true);

        checkFilters();
        checkPlugin(ui.tableWidget->currentRow());
    }

    void checkPlugin(const int row)
    {
        if (row >= 0)
        {
            ui.b_add->setEnabled(true);

            PluginInfo plugin = asPluginInfo(ui.tableWidget->item(row, TABLEWIDGET_ITEM_NAME)->data(Qt::UserRole+1));

            const bool isSynth  = plugin.hints & PLUGIN_IS_SYNTH;
            const bool isEffect = plugin.audioIns > 0 && plugin.audioOuts > 0 && !isSynth;
            const bool isMidi   = plugin.audioIns == 0 &&
                                  plugin.audioOuts == 0 &&
                                  plugin.midiIns > 0 && plugin.midiOuts > 0;
            // const bool isKit    = plugin['type'] in (PLUGIN_SF2, PLUGIN_SFZ);
            // const bool isOther  = ! (isEffect || isSynth || isMidi || isKit);

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
            /**/ if (plugin.build == BINARY_NATIVE)
                parch = fTrNative;
            else if (plugin.build == BINARY_POSIX32)
                parch = "posix32";
            else if (plugin.build == BINARY_POSIX64)
                parch = "posix64";
            else if (plugin.build == BINARY_WIN32)
                parch = "win32";
            else if (plugin.build == BINARY_WIN64)
                parch = "win64";
            else if (plugin.build == BINARY_OTHER)
                parch = tr("Other");
            else if (plugin.build == BINARY_WIN32)
                parch = tr("Unknown");

            ui.l_format->setText(getPluginTypeAsString(plugin.type));

            ui.l_type->setText(ptype);
            ui.l_arch->setText(parch);
            ui.l_id->setText(QString::number(plugin.uniqueId));
            ui.l_ains->setText(QString::number(plugin.audioIns));
            ui.l_aouts->setText(QString::number(plugin.audioOuts));
            ui.l_cvins->setText(QString::number(plugin.cvIns));
            ui.l_cvouts->setText(QString::number(plugin.cvOuts));
            ui.l_mins->setText(QString::number(plugin.midiIns));
            ui.l_mouts->setText(QString::number(plugin.midiOuts));
            ui.l_pins->setText(QString::number(plugin.parametersIns));
            ui.l_pouts->setText(QString::number(plugin.parametersOuts));
            ui.l_gui->setText(plugin.hints & PLUGIN_HAS_CUSTOM_UI ? fTrYes : fTrNo);
            ui.l_idisp->setText(plugin.hints & PLUGIN_HAS_INLINE_DISPLAY ? fTrYes : fTrNo);
            ui.l_bridged->setText(plugin.hints & PLUGIN_IS_BRIDGE ? fTrYes : fTrNo);
            ui.l_synth->setText(isSynth ? fTrYes : fTrNo);
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
};

PluginListDialog::PluginListDialog(QWidget* const parent, const HostSettings& hostSettings)
    : QDialog(parent),
      self(Self::create(parent))
{
    self.ui.setupUi(this);
    self.hostSettings = hostSettings;

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up GUI

    self.ui.b_add->setEnabled(false);
    addAction(self.ui.act_focus_search);
    QObject::connect(self.ui.act_focus_search, &QAction::triggered,
                     this, &PluginListDialog::slot_focusSearchFieldAndSelectAll);

   #if BINARY_NATIVE == BINARY_POSIX32 || BINARY_NATIVE == BINARY_WIN32
    self.ui.ch_bridged->setText(tr("Bridged (64bit)"));
   #else
    self.ui.ch_bridged->setText(tr("Bridged (32bit)"));
   #endif

   #if !(defined(CARLA_OS_LINUX) || defined(CARLA_OS_MAC))
    self.ui.ch_bridged_wine->setChecked(false);
    self.ui.ch_bridged_wine->setEnabled(false);
   #endif

   #ifdef CARLA_OS_MAC
    setWindowModality(Qt::WindowModal);
   #else
    self.ui.ch_au->setChecked(false);
    self.ui.ch_au->setEnabled(false);
    self.ui.ch_au->setVisible(false);
   #endif

    self.ui.tab_info->tabBar()->hide();
    self.ui.tab_reqs->tabBar()->hide();
    // FIXME, why /2 needed?
    self.ui.tab_info->setMinimumWidth(self.ui.la_id->width()/2 +
                                      fontMetricsHorizontalAdvance(self.ui.l_id->fontMetrics(), "9999999999") + 6*3);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // ----------------------------------------------------------------------------------------------------------------
    // Load settings

    loadSettings();

    // ----------------------------------------------------------------------------------------------------------------
    // Disable bridges if not enabled in settings

#if 0
    // NOTE: We Assume win32 carla build will not run win64 plugins
    if (WINDOWS and not kIs64bit) or not host.showPluginBridges:
        self.ui.ch_native.setChecked(True)
        self.ui.ch_native.setEnabled(False)
        self.ui.ch_native.setVisible(True)
        self.ui.ch_bridged.setChecked(False)
        self.ui.ch_bridged.setEnabled(False)
        self.ui.ch_bridged.setVisible(False)
        self.ui.ch_bridged_wine.setChecked(False)
        self.ui.ch_bridged_wine.setEnabled(False)
        self.ui.ch_bridged_wine.setVisible(False)

    elif not host.showWineBridges:
        self.ui.ch_bridged_wine.setChecked(False)
        self.ui.ch_bridged_wine.setEnabled(False)
        self.ui.ch_bridged_wine.setVisible(False)
#endif

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up Icons

    if (hostSettings.useSystemIcons)
    {
#if 0
        self.ui.b_add.setIcon(getIcon('list-add', 16, 'svgz'))
        self.ui.b_cancel.setIcon(getIcon('dialog-cancel', 16, 'svgz'))
        self.ui.b_clear_filters.setIcon(getIcon('edit-clear', 16, 'svgz'))
        self.ui.b_refresh.setIcon(getIcon('view-refresh', 16, 'svgz'))
        QTableWidgetItem* const hhi = self.ui.tableWidget->horizontalHeaderItem(self.TABLEWIDGET_ITEM_FAVORITE);
        hhi.setIcon(getIcon('bookmarks', 16, 'svgz'))
#endif
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Set-up connections

    QObject::connect(this, &QDialog::finished, this, &PluginListDialog::slot_saveSettings);
    QObject::connect(self.ui.b_add, &QPushButton::clicked, this, &PluginListDialog::slot_addPlugin);
    QObject::connect(self.ui.b_cancel, &QPushButton::clicked, this, &QDialog::reject);

    QObject::connect(self.ui.b_refresh, &QPushButton::clicked, this, &PluginListDialog::slot_refreshPlugins);
    QObject::connect(self.ui.b_clear_filters, &QPushButton::clicked, this, &PluginListDialog::slot_clearFilters);
    QObject::connect(self.ui.lineEdit, &QLineEdit::textChanged, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.tableWidget, &QTableWidget::currentCellChanged,
                     this, &PluginListDialog::slot_checkPlugin);
    QObject::connect(self.ui.tableWidget, &QTableWidget::cellClicked,
                     this, &PluginListDialog::slot_cellClicked);
    QObject::connect(self.ui.tableWidget, &QTableWidget::cellDoubleClicked,
                     this, &PluginListDialog::slot_cellDoubleClicked);

    QObject::connect(self.ui.ch_internal, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_ladspa, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_dssi, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_lv2, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_vst, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_vst3, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_clap, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_au, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_jsfx, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_kits, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_effects, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_instruments, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_midi, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_other, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_native, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_bridged, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_bridged_wine, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_favorites, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_rtsafe, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_cv, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_gui, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_inline_display, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_stereo, &QCheckBox::clicked, this, &PluginListDialog::slot_checkFilters);
    QObject::connect(self.ui.ch_cat_all, &QCheckBox::clicked,
                     this, &PluginListDialog::slot_checkFiltersCategoryAll);
    QObject::connect(self.ui.ch_cat_delay, &QCheckBox::clicked,
                     this, &PluginListDialog::slot_checkFiltersCategorySpecific);
    QObject::connect(self.ui.ch_cat_distortion, &QCheckBox::clicked,
                     this, &PluginListDialog::slot_checkFiltersCategorySpecific);
    QObject::connect(self.ui.ch_cat_dynamics, &QCheckBox::clicked,
                     this, &PluginListDialog::slot_checkFiltersCategorySpecific);
    QObject::connect(self.ui.ch_cat_eq, &QCheckBox::clicked,
                     this, &PluginListDialog::slot_checkFiltersCategorySpecific);
    QObject::connect(self.ui.ch_cat_filter, &QCheckBox::clicked,
                     this, &PluginListDialog::slot_checkFiltersCategorySpecific);
    QObject::connect(self.ui.ch_cat_modulator, &QCheckBox::clicked,
                     this, &PluginListDialog::slot_checkFiltersCategorySpecific);
    QObject::connect(self.ui.ch_cat_synth, &QCheckBox::clicked,
                     this, &PluginListDialog::slot_checkFiltersCategorySpecific);
    QObject::connect(self.ui.ch_cat_utility, &QCheckBox::clicked,
                     this, &PluginListDialog::slot_checkFiltersCategorySpecific);
    QObject::connect(self.ui.ch_cat_other, &QCheckBox::clicked, this,
                     &PluginListDialog::slot_checkFiltersCategorySpecific);

    // ----------------------------------------------------------------------------------------------------------------
    // Post-connect setup

    self.reAddPlugins();
    slot_focusSearchFieldAndSelectAll();
}

PluginListDialog::~PluginListDialog()
{
    delete &self;
}

// --------------------------------------------------------------------------------------------------------------------
// public methods

const PluginInfo& PluginListDialog::getSelectedPluginInfo() const
{
    return self.fRetPlugin;
}

// --------------------------------------------------------------------------------------------------------------------
// protected methods

void PluginListDialog::showEvent(QShowEvent* const event)
{
    slot_focusSearchFieldAndSelectAll();
    QDialog::showEvent(event);
}

// --------------------------------------------------------------------------------------------------------------------
// private methods

void PluginListDialog::loadSettings()
{
    const QSafeSettings settings("falkTX", "CarlaDatabase2");
    self.fFavoritePlugins = settings.valueStringList("PluginDatabase/Favorites");
    self.fFavoritePluginsChanged = false;

    restoreGeometry(settings.valueByteArray("PluginDatabase/Geometry"));
    self.ui.ch_effects->setChecked(settings.valueBool("PluginDatabase/ShowEffects", true));
    self.ui.ch_instruments->setChecked(settings.valueBool("PluginDatabase/ShowInstruments", true));
    self.ui.ch_midi->setChecked(settings.valueBool("PluginDatabase/ShowMIDI", true));
    self.ui.ch_other->setChecked(settings.valueBool("PluginDatabase/ShowOther", true));
    self.ui.ch_internal->setChecked(settings.valueBool("PluginDatabase/ShowInternal", true));
    self.ui.ch_ladspa->setChecked(settings.valueBool("PluginDatabase/ShowLADSPA", true));
    self.ui.ch_dssi->setChecked(settings.valueBool("PluginDatabase/ShowDSSI", true));
    self.ui.ch_lv2->setChecked(settings.valueBool("PluginDatabase/ShowLV2", true));
    self.ui.ch_vst->setChecked(settings.valueBool("PluginDatabase/ShowVST2", true));
    self.ui.ch_vst3->setChecked(settings.valueBool("PluginDatabase/ShowVST3", true));
    self.ui.ch_clap->setChecked(settings.valueBool("PluginDatabase/ShowCLAP", true));
   #ifdef CARLA_OS_MAC
    self.ui.ch_au->setChecked(settings.valueBool("PluginDatabase/ShowAU", true));
   #endif
    self.ui.ch_jsfx->setChecked(settings.valueBool("PluginDatabase/ShowJSFX", true));
    self.ui.ch_kits->setChecked(settings.valueBool("PluginDatabase/ShowKits", true));
    self.ui.ch_native->setChecked(settings.valueBool("PluginDatabase/ShowNative", true));
    self.ui.ch_bridged->setChecked(settings.valueBool("PluginDatabase/ShowBridged", true));
    self.ui.ch_bridged_wine->setChecked(settings.valueBool("PluginDatabase/ShowBridgedWine", true));
    self.ui.ch_favorites->setChecked(settings.valueBool("PluginDatabase/ShowFavorites", false));
    self.ui.ch_rtsafe->setChecked(settings.valueBool("PluginDatabase/ShowRtSafe", false));
    self.ui.ch_cv->setChecked(settings.valueBool("PluginDatabase/ShowHasCV", false));
    self.ui.ch_gui->setChecked(settings.valueBool("PluginDatabase/ShowHasGUI", false));
    self.ui.ch_inline_display->setChecked(settings.valueBool("PluginDatabase/ShowHasInlineDisplay", false));
    self.ui.ch_stereo->setChecked(settings.valueBool("PluginDatabase/ShowStereoOnly", false));
    self.ui.lineEdit->setText(settings.valueString("PluginDatabase/SearchText", ""));

    const QString categories = settings.valueString("PluginDatabase/ShowCategory", "all");
    if (categories == "all" or categories.length() < 2)
    {
        self.ui.ch_cat_all->setChecked(true);
        self.ui.ch_cat_delay->setChecked(false);
        self.ui.ch_cat_distortion->setChecked(false);
        self.ui.ch_cat_dynamics->setChecked(false);
        self.ui.ch_cat_eq->setChecked(false);
        self.ui.ch_cat_filter->setChecked(false);
        self.ui.ch_cat_modulator->setChecked(false);
        self.ui.ch_cat_synth->setChecked(false);
        self.ui.ch_cat_utility->setChecked(false);
        self.ui.ch_cat_other->setChecked(false);
    }
    else
    {
        self.ui.ch_cat_all->setChecked(false);
        self.ui.ch_cat_delay->setChecked(categories.contains(":delay:"));
        self.ui.ch_cat_distortion->setChecked(categories.contains(":distortion:"));
        self.ui.ch_cat_dynamics->setChecked(categories.contains(":dynamics:"));
        self.ui.ch_cat_eq->setChecked(categories.contains(":eq:"));
        self.ui.ch_cat_filter->setChecked(categories.contains(":filter:"));
        self.ui.ch_cat_modulator->setChecked(categories.contains(":modulator:"));
        self.ui.ch_cat_synth->setChecked(categories.contains(":synth:"));
        self.ui.ch_cat_utility->setChecked(categories.contains(":utility:"));
        self.ui.ch_cat_other->setChecked(categories.contains(":other:"));
    }

    const QByteArray tableGeometry = settings.valueByteArray("PluginDatabase/TableGeometry_6");
    QHeaderView* const horizontalHeader = self.ui.tableWidget->horizontalHeader();
    if (! tableGeometry.isNull())
    {
        horizontalHeader->restoreState(tableGeometry);
    }
    else
    {
        self.ui.tableWidget->setColumnWidth(self.TABLEWIDGET_ITEM_NAME, 250);
        self.ui.tableWidget->setColumnWidth(self.TABLEWIDGET_ITEM_LABEL, 200);
        self.ui.tableWidget->setColumnWidth(self.TABLEWIDGET_ITEM_MAKER, 150);
        self.ui.tableWidget->sortByColumn(self.TABLEWIDGET_ITEM_NAME, Qt::AscendingOrder);
    }

    horizontalHeader->setSectionResizeMode(self.TABLEWIDGET_ITEM_FAVORITE, QHeaderView::Fixed);
    self.ui.tableWidget->setColumnWidth(self.TABLEWIDGET_ITEM_FAVORITE, 24);
    self.ui.tableWidget->setSortingEnabled(true);
}

// -----------------------------------------------------------------------------------------------------------------
// private slots

void PluginListDialog::slot_cellClicked(int row, int column)
{
    if (column != self.TABLEWIDGET_ITEM_FAVORITE)
        return;

    const QTableWidgetItem* const widget = self.ui.tableWidget->item(row, self.TABLEWIDGET_ITEM_FAVORITE);
    const PluginInfo plugin = asPluginInfo(self.ui.tableWidget->item(row, self.TABLEWIDGET_ITEM_NAME)->data(Qt::UserRole+1));
#if 0
    plugin = self._createFavoritePluginDict(plugin);
#endif

    if (widget->checkState() == Qt::Checked)
    {
#if 0
        if (not plugin in self.fFavoritePlugins)
        {
            self.fFavoritePlugins.append(plugin);
            self.fFavoritePluginsChanged = true;
        }
#endif
    }
    else
    {
#if 0
        try:
            self.fFavoritePlugins.remove(plugin);
            self.fFavoritePluginsChanged = True;
        except ValueError:
            pass
#endif
    }
}

void PluginListDialog::slot_cellDoubleClicked(int, int column)
{
    if (column != self.TABLEWIDGET_ITEM_FAVORITE)
        slot_addPlugin();
}

void PluginListDialog::slot_focusSearchFieldAndSelectAll()
{
    self.ui.lineEdit->setFocus();
    self.ui.lineEdit->selectAll();
}

void PluginListDialog::slot_addPlugin()
{
    if (self.ui.tableWidget->currentRow() >= 0)
    {
        self.fRetPlugin = asPluginInfo(self.ui.tableWidget->item(self.ui.tableWidget->currentRow(),
                                                                 self.TABLEWIDGET_ITEM_NAME)->data(Qt::UserRole+1));
        accept();
    }
    else
    {
        reject();
    }
}

void PluginListDialog::slot_checkPlugin(const int row)
{
    self.checkPlugin(row);
}

void PluginListDialog::slot_checkFilters()
{
    self.checkFilters();
}

void PluginListDialog::slot_checkFiltersCategoryAll(const bool clicked)
{
    const bool notClicked = !clicked;
    self.ui.ch_cat_delay->setChecked(notClicked);
    self.ui.ch_cat_distortion->setChecked(notClicked);
    self.ui.ch_cat_dynamics->setChecked(notClicked);
    self.ui.ch_cat_eq->setChecked(notClicked);
    self.ui.ch_cat_filter->setChecked(notClicked);
    self.ui.ch_cat_modulator->setChecked(notClicked);
    self.ui.ch_cat_synth->setChecked(notClicked);
    self.ui.ch_cat_utility->setChecked(notClicked);
    self.ui.ch_cat_other->setChecked(notClicked);
    self.checkFilters();
}

void PluginListDialog::slot_checkFiltersCategorySpecific(bool clicked)
{
    if (clicked)
    {
        self.ui.ch_cat_all->setChecked(false);
    }
    else if (! (self.ui.ch_cat_delay->isChecked() ||
                self.ui.ch_cat_distortion->isChecked() ||
                self.ui.ch_cat_dynamics->isChecked() ||
                self.ui.ch_cat_eq->isChecked() ||
                self.ui.ch_cat_filter->isChecked() ||
                self.ui.ch_cat_modulator->isChecked() ||
                self.ui.ch_cat_synth->isChecked() ||
                self.ui.ch_cat_utility->isChecked() ||
                self.ui.ch_cat_other->isChecked()))
    {
        self.ui.ch_cat_all->setChecked(true);
    }
    self.checkFilters();
}

void PluginListDialog::slot_refreshPlugins()
{
#if 0
    if (PluginRefreshW(this, self.hostSettings, self.hasLoadedLv2Plugins).exec())
        reAddPlugins();
#endif
}

void PluginListDialog::slot_clearFilters()
{
    blockSignals(true);

    self.ui.ch_internal->setChecked(true);
    self.ui.ch_ladspa->setChecked(true);
    self.ui.ch_dssi->setChecked(true);
    self.ui.ch_lv2->setChecked(true);
    self.ui.ch_vst->setChecked(true);
    self.ui.ch_vst3->setChecked(true);
    self.ui.ch_clap->setChecked(true);
    self.ui.ch_jsfx->setChecked(true);
    self.ui.ch_kits->setChecked(true);

    self.ui.ch_instruments->setChecked(true);
    self.ui.ch_effects->setChecked(true);
    self.ui.ch_midi->setChecked(true);
    self.ui.ch_other->setChecked(true);

    self.ui.ch_native->setChecked(true);
    self.ui.ch_bridged->setChecked(false);
    self.ui.ch_bridged_wine->setChecked(false);

    self.ui.ch_favorites->setChecked(false);
    self.ui.ch_rtsafe->setChecked(false);
    self.ui.ch_stereo->setChecked(false);
    self.ui.ch_cv->setChecked(false);
    self.ui.ch_gui->setChecked(false);
    self.ui.ch_inline_display->setChecked(false);

    if (self.ui.ch_au->isEnabled())
        self.ui.ch_au->setChecked(true);

    self.ui.ch_cat_all->setChecked(true);
    self.ui.ch_cat_delay->setChecked(false);
    self.ui.ch_cat_distortion->setChecked(false);
    self.ui.ch_cat_dynamics->setChecked(false);
    self.ui.ch_cat_eq->setChecked(false);
    self.ui.ch_cat_filter->setChecked(false);
    self.ui.ch_cat_modulator->setChecked(false);
    self.ui.ch_cat_synth->setChecked(false);
    self.ui.ch_cat_utility->setChecked(false);
    self.ui.ch_cat_other->setChecked(false);

    self.ui.lineEdit->clear();

    blockSignals(false);

    self.checkFilters();
}

// --------------------------------------------------------------------------------------------------------------------

void PluginListDialog::slot_saveSettings()
{
    QSafeSettings settings("falkTX", "CarlaDatabase2");
    settings.setValue("PluginDatabase/Geometry", saveGeometry());
    settings.setValue("PluginDatabase/TableGeometry_6", self.ui.tableWidget->horizontalHeader()->saveState());
    settings.setValue("PluginDatabase/ShowEffects", self.ui.ch_effects->isChecked());
    settings.setValue("PluginDatabase/ShowInstruments", self.ui.ch_instruments->isChecked());
    settings.setValue("PluginDatabase/ShowMIDI", self.ui.ch_midi->isChecked());
    settings.setValue("PluginDatabase/ShowOther", self.ui.ch_other->isChecked());
    settings.setValue("PluginDatabase/ShowInternal", self.ui.ch_internal->isChecked());
    settings.setValue("PluginDatabase/ShowLADSPA", self.ui.ch_ladspa->isChecked());
    settings.setValue("PluginDatabase/ShowDSSI", self.ui.ch_dssi->isChecked());
    settings.setValue("PluginDatabase/ShowLV2", self.ui.ch_lv2->isChecked());
    settings.setValue("PluginDatabase/ShowVST2", self.ui.ch_vst->isChecked());
    settings.setValue("PluginDatabase/ShowVST3", self.ui.ch_vst3->isChecked());
    settings.setValue("PluginDatabase/ShowCLAP", self.ui.ch_clap->isChecked());
    settings.setValue("PluginDatabase/ShowAU", self.ui.ch_au->isChecked());
    settings.setValue("PluginDatabase/ShowJSFX", self.ui.ch_jsfx->isChecked());
    settings.setValue("PluginDatabase/ShowKits", self.ui.ch_kits->isChecked());
    settings.setValue("PluginDatabase/ShowNative", self.ui.ch_native->isChecked());
    settings.setValue("PluginDatabase/ShowBridged", self.ui.ch_bridged->isChecked());
    settings.setValue("PluginDatabase/ShowBridgedWine", self.ui.ch_bridged_wine->isChecked());
    settings.setValue("PluginDatabase/ShowFavorites", self.ui.ch_favorites->isChecked());
    settings.setValue("PluginDatabase/ShowRtSafe", self.ui.ch_rtsafe->isChecked());
    settings.setValue("PluginDatabase/ShowHasCV", self.ui.ch_cv->isChecked());
    settings.setValue("PluginDatabase/ShowHasGUI", self.ui.ch_gui->isChecked());
    settings.setValue("PluginDatabase/ShowHasInlineDisplay", self.ui.ch_inline_display->isChecked());
    settings.setValue("PluginDatabase/ShowStereoOnly", self.ui.ch_stereo->isChecked());
    settings.setValue("PluginDatabase/SearchText", self.ui.lineEdit->text());

    if (self.ui.ch_cat_all->isChecked())
    {
        settings.setValue("PluginDatabase/ShowCategory", "all");
    }
    else
    {
        QCarlaString categories;
        if (self.ui.ch_cat_delay->isChecked())
            categories += ":delay";
        if (self.ui.ch_cat_distortion->isChecked())
            categories += ":distortion";
        if (self.ui.ch_cat_dynamics->isChecked())
            categories += ":dynamics";
        if (self.ui.ch_cat_eq->isChecked())
            categories += ":eq";
        if (self.ui.ch_cat_filter->isChecked())
            categories += ":filter";
        if (self.ui.ch_cat_modulator->isChecked())
            categories += ":modulator";
        if (self.ui.ch_cat_synth->isChecked())
            categories += ":synth";
        if (self.ui.ch_cat_utility->isChecked())
            categories += ":utility";
        if (self.ui.ch_cat_other->isChecked())
            categories += ":other";
        if (categories.isNotEmpty())
            categories += ":";
        settings.setValue("PluginDatabase/ShowCategory", categories);
    }

    if (self.fFavoritePluginsChanged)
        settings.setValue("PluginDatabase/Favorites", self.fFavoritePlugins);
}

// --------------------------------------------------------------------------------------------------------------------

const PluginListDialogResults*
carla_frontend_createAndExecPluginListDialog(void* const parent/*, const HostSettings& hostSettings*/)
{
    const HostSettings hostSettings = {};
    PluginListDialog gui(reinterpret_cast<QWidget*>(parent), hostSettings);

    if (gui.exec())
    {
        static PluginListDialogResults ret;
        static CarlaString category;
        static CarlaString filename;
        static CarlaString name;
        static CarlaString label;
        static CarlaString maker;

        const PluginInfo& plugin(gui.getSelectedPluginInfo());

        category = plugin.category.toUtf8();
        filename = plugin.filename.toUtf8();
        name = plugin.name.toUtf8();
        label = plugin.label.toUtf8();
        maker = plugin.maker.toUtf8();

        ret.API = plugin.API;
        ret.build = plugin.build;
        ret.type = plugin.type;
        ret.hints = plugin.hints;
        ret.category = category;
        ret.filename = filename;
        ret.name = name;
        ret.label = label;
        ret.maker = maker;
        ret.audioIns = plugin.audioIns;
        ret.audioOuts = plugin.audioOuts;
        ret.cvIns = plugin.cvIns;
        ret.cvOuts = plugin.cvOuts;
        ret.midiIns = plugin.midiIns;
        ret.midiOuts = plugin.midiOuts;
        ret.parametersIns = plugin.parametersIns;
        ret.parametersOuts = plugin.parametersOuts;

        return &ret;
    }

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
